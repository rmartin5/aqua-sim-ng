/*
 * broadcastMAC_example.cc
 *
 *  Created on: Nov 3, 2015
 *      Author: Robert Martin
 */
/*
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/energy-module.h"  //may not be needed here...
#include "ns3/aqua-sim-ng-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/callback.h"

/*
 * BroadCastMAC
 *
 * N ---->  N  -----> N -----> N* -----> S
 *
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ASBroadcastMac");

int
main (int argc, char *argv[])
{
  double simStop = 10; //seconds
  int nodes = 2;
  int sinks = 1;
  uint32_t m_dataRate = 150;
  uint32_t m_packetSize = 80;

  /*
   * **********
   * Node is the global combination here. May be helpful to review UAN and how it uses Node but my guess is this: Node stats it automatically adds created nodes to NodeList.
   * So then by going through Nodelist it can have access to all nodes created. This means I need to make sure that Node can access netdevice (again similar to UAN).
   * ---may not need to create a child class of Node for underwater reasons, instead may just need to ensure connection between node and net device here... may have to scale back net device
   * and instead add to node (attaching layers to node instead of net device??? not sure standard here)
   * ---Also need to look into id of nodes and assignment of this
   * ---need to look at assignment of address and making it unique per node.
   *
   *
   *  Ensure to use NS_LOG when testing in terminal. ie. ./waf --run broadcastMAC_example NS_LOG=Node=level_all or export 'NS_LOG=*=level_all|prefix_func'
   *
   *  Note: both index # is 0 while array number differs within m_device, this holds true for both local list and general node.cc list... Good to konw...
   *  *********
   */

  LogComponentEnable ("ASBroadcastMac", LOG_LEVEL_INFO);

  //to change on the fly
  CommandLine cmd;
  cmd.AddValue ("simStop", "Length of simulation", simStop);
  cmd.AddValue ("nodes", "Amount of regular underwater nodes", nodes);
  cmd.AddValue ("sinks", "Amount of underwater sinks", sinks);
  cmd.Parse(argc,argv);

  std::cout << "Initializing simulation\n";

  NodeContainer nodesCon;
  NodeContainer sinksCon;
  nodesCon.Create(nodes);
  sinksCon.Create(sinks);

  PacketSocketHelper socketHelper;
  socketHelper.Install(nodesCon);
  socketHelper.Install(sinksCon);

  //establish layers using helper's pre-build settings
  AquaSimChannelHelper channel = AquaSimChannelHelper::Default();
  AquaSimHelper asHelper = AquaSimHelper::Default();
  //AquaSimEnergyHelper energy;	//******this could instead be handled by node helper. ****/
  asHelper.SetChannel(channel.Create());

  /*
   * Preset up mobility model for nodes and sinks here
   */
  MobilityHelper mobility;
  NetDeviceContainer devices;
  Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator> ();

  //Static Y and Z dimension for now
  double m_YBoundry = 0;
  double m_ZBoundry = 0;
  double m_XBoundry = 0;

  std::cout << "Creating Nodes\n";

  for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      position->Add(Vector(m_XBoundry, m_YBoundry, m_ZBoundry));

      devices.Add(asHelper.Create(*i, newDevice));

      NS_LOG_DEBUG("Node: " << *i << " newDevice: " << newDevice << " Position: " <<
		     m_XBoundry << "," << m_YBoundry << "," << m_ZBoundry <<
		     " freq:" << newDevice->GetPhy()->GetFrequency());
		     //<<
		     //" NDtypeid:" << newDevice->GetTypeId() <<
		     //" Ptypeid:" << newDevice->GetPhy()->GetTypeId());

      m_XBoundry += 20;
    }

  for (NodeContainer::Iterator i = sinksCon.Begin(); i != sinksCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      position->Add(Vector(m_XBoundry, m_YBoundry, m_ZBoundry));

      devices.Add(asHelper.Create(*i, newDevice));

      NS_LOG_DEBUG("Sink: " << *i << " newDevice: " << newDevice << " Position: " <<
		     m_XBoundry << "," << m_YBoundry << "," << m_ZBoundry);

      m_XBoundry += 20;
    }


  mobility.SetPositionAllocator(position);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(nodesCon);
  mobility.Install(sinksCon);


  PacketSocketAddress socket;
  socket.SetAllDevices();
  // socket.SetSingleDevice (devices.Get(0)->GetIfIndex());
  //socket.SetPhysicalAddress (devices.Get(0)->GetAddress());
  socket.SetProtocol (0);

  std::cout << devices.Get(0)->GetAddress() << " &&& " << devices.Get(0)->GetIfIndex() << "\n";
  std::cout << devices.Get(1)->GetAddress() << " &&& " << devices.Get(1)->GetIfIndex() << "\n";


  OnOffHelper app ("ns3::PacketSocketFactory", Address (socket));
  app.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  app.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  app.SetAttribute ("DataRate", DataRateValue (m_dataRate));
  app.SetAttribute ("PacketSize", UintegerValue (m_packetSize));

  ApplicationContainer apps = app.Install (nodesCon);
  apps.Start (Seconds (0.5));
  apps.Stop (Seconds (simStop + 1));


  Ptr<Node> sinkNode = sinksCon.Get(0);
  TypeId psfid = TypeId::LookupByName ("ns3::PacketSocketFactory");

  Ptr<Socket> sinkSocket = Socket::CreateSocket (sinkNode, psfid);
  sinkSocket->Bind (socket);

/*
  ApplicationContainer serverApp;
  UdpServerHelper myServer (250);
  serverApp = myServer.Install (nodesCon.Get (0));
  serverApp.Start (Seconds (0.0));
  serverApp.Stop (Seconds (simStop + 1));
*/ //TODO implement application within this example...

  std::cout << "Running Simulation\n";
  Simulator::Stop(Seconds(simStop + 1));
  Simulator::Run();
  Simulator::Destroy(); //null all nodes too??

  std::cout << "fin.\n";
  return 0;
}


/*
 *
Within function calls
{
	Define starting protocols and values (each layer, energy, etc.)
		Each protocol may need to set its own values (this can be accomplished through a helper class i.e. phy x and y, \
		  node needs to set its positions, etc.)
		Must loop through each node assigning its location and other features (can be done mainly through passing to helper \
		  which then will assign appropriate values using defined functions)

	Set up layout (through mobility i believe)
	Set up trace (needs to be implemented but a TODO will work for now)


	Create traffic generator (ns3::OnOffHelper maybe)

	Start/stop time.
	Run.
	Null all nodes (set to 0).
	Simulation::Destroy().
}

main
{
	Create basics (through function call)
	Declare any basic variables and use helper to set up
	call 'Run' function with the given setup from helper (i.e. Helper h; h.SetLayer(x,y,z); h.Run(); OR x = h.Run(); txtOutput(x);)
}

Notes: Must use containers (i.e. NodeContainers). Can have two different containers (sinks + nodes of same class/type). Use prebuild\
   terms like 'Create(x)'. Use Simulator::Schedule to do events (sending or other techniques) and/or set callbacks to handle\
   sending/recving. Error checking is a must!

 *
 */
