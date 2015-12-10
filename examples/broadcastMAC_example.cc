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

/*
 * BroadCastMAC
 *
 * N ---->  N  -----> N -----> N* -----> S
 *
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("aqua-sim-broadcast-mac");

int
main (int argc, char *argv[])
{
  double simStop = 600; //seconds
  int nodes = 4;
  int sinks = 1;

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

  for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      position->Add(Vector(m_XBoundry, m_YBoundry, m_ZBoundry));

      newDevice = asHelper.Create(*i, newDevice);

      m_XBoundry += 20;
    }

  for (NodeContainer::Iterator i = sinksCon.Begin(); i != sinksCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      position->Add(Vector(m_XBoundry, m_YBoundry, m_ZBoundry));

      newDevice = asHelper.Create(*i, newDevice);

      m_XBoundry += 20;
    }


  mobility.SetPositionAllocator(position);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(nodesCon);
  mobility.Install(sinksCon);

  /*
  PacketSocketAddress socket;
  socket.SetSingleDevice (0);
  socket.SetPhysicalAddress (Address ());
  socket.SetProtocol (0);

  OnOffHelper app ("ns3::PacketSocketFactory", Address (socket));
  app.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  app.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  app.SetAttribute ("DataRate", DataRateValue (m_dataRate));
  app.SetAttribute ("PacketSize", UintegerValue (m_packetSize));

  ApplicationContainer apps = app.Install (nodesCon);
  apps.Start (Seconds (0.5));
  Time nextEvent = Seconds (0.5);
  */

  ApplicationContainer serverApp;
  UdpServerHelper myServer (9);
  serverApp = myServer.Install (nodesCon.Get (0));
  serverApp.Start (Seconds (0.0));
  serverApp.Stop (Seconds (simStop + 1));


  Simulator::Stop(Seconds(simStop + 1));
  Simulator::Run();
  Simulator::Destroy(); //null all nodes too??

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
