/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 University of Connecticut
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Robert Martin <robert.martin@engr.uconn.edu>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/energy-module.h"  //may not be needed here...
#include "ns3/aqua-sim-ng-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/callback.h"
#include <math.h>

/*
 * Flood : 2xN architecture
 *
 * N ---->  N  -----> N -----> N*
 * |        |         |        |    ->(S)
 * N ---->  N  -----> N -----> N*
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ASBroadcastMac");

class LocalExperiment
{
public:
  LocalExperiment();
  void Run();
  void RecvPacket(Ptr<Socket> socket);

  double simStop; //seconds
  int nodes;
  int sinks;
  uint32_t m_dataRate;
  uint32_t m_packetSize;
};

LocalExperiment::LocalExperiment() :
  simStop(20), nodes(4), sinks(1),
  m_dataRate(180), m_packetSize(32)
{
}

void
LocalExperiment::RecvPacket(Ptr<Socket> socket)
{
  std::cout << "HIT;\n";
  Ptr<Packet> packet;
  while ((packet = socket->Recv ()))
  {
    std::cout << "Recv a packet of size " << packet->GetSize() << "\n";
    //m_bytesTotal += packet->GetSize ();
  }
}

void
LocalExperiment::Run()
{
  /*
   * **********
   * Node -> NetDevice -> AquaSimNetDeive -> etc.
   * Note: Nodelist keeps track of all nodes created.
   * ---Also need to look into id of nodes and assignment of this
   * ---need to look at assignment of address and making it unique per node.
   *
   *
   *  Ensure to use NS_LOG when testing in terminal. ie. ./waf --run broadcastMAC_example NS_LOG=Node=level_all or export 'NS_LOG=*=level_all|prefix_func'
   *  *********
   */


  std::cout << "-----------Initializing simulation-----------\n";

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
  Vector boundry = Vector(0,0,0);

  std::cout << "Creating Nodes\n";

  int c = 0;
  int nodeDistance = 2000;
  for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      position->Add(boundry);
      devices.Add(asHelper.Create(*i, newDevice));

      NS_LOG_DEBUG("Node: " << *i << " newDevice: " << newDevice << " Position:(" <<
		     boundry.x << "," << boundry.y << "," << boundry.z <<
		     ") freq:" << newDevice->GetPhy()->GetFrequency());
		     //<<
		     //" NDtypeid:" << newDevice->GetTypeId() <<
		     //" Ptypeid:" << newDevice->GetPhy()->GetTypeId());

      if(c%2) //row 1
      {
        boundry.y = 0;
        boundry.x += nodeDistance;
      }
      else  //row 2
      {
        boundry.y = nodeDistance;
      }
      c++;
    }

  for (NodeContainer::Iterator i = sinksCon.Begin(); i != sinksCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();

      if(c%2==1)
        {boundry.x += nodeDistance;}
      boundry.y = nodeDistance/2;
      position->Add(boundry);

      devices.Add(asHelper.Create(*i, newDevice));

      NS_LOG_DEBUG("Sink: " << *i << " newDevice: " << newDevice << " Position:(" <<
		     boundry.x << "," << boundry.y << "," << boundry.z << ")");
    }

  mobility.SetPositionAllocator(position);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(nodesCon);
  mobility.Install(sinksCon);

  PacketSocketAddress socket;
  socket.SetAllDevices();
  // socket.SetSingleDevice (devices.Get(0)->GetIfIndex());
  socket.SetPhysicalAddress (devices.Get(nodes+sinks-1)->GetAddress());
  socket.SetProtocol (0);


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
  sinkSocket->SetRecvCallback (MakeCallback (&LocalExperiment::RecvPacket, this));

/*
  ApplicationContainer serverApp;
  UdpServerHelper myServer (250);
  serverApp = myServer.Install (nodesCon.Get (0));
  serverApp.Start (Seconds (0.0));
  serverApp.Stop (Seconds (simStop + 1));
*/ //TODO implement application within this example...

  std::cout << "-----------Running Simulation-----------\n";
  Simulator::Stop(Seconds(simStop + 1));
  Simulator::Run();
  Simulator::Destroy(); //null all nodes too??
  asHelper.GetChannel()->PrintCounters();
  std::cout << "End.\n";
}

int
main (int argc, char *argv[])
{
  LocalExperiment exp;

  LogComponentEnable ("ASBroadcastMac", LOG_LEVEL_INFO);
  //to change on the fly
  CommandLine cmd;
  cmd.AddValue ("simStop", "Length of simulation", exp.simStop);
  cmd.AddValue ("nodes", "Amount of regular underwater nodes", exp.nodes);
  cmd.AddValue ("sinks", "Amount of underwater sinks", exp.sinks);
  cmd.Parse(argc,argv);

  exp.Run();
  return 0;
}
