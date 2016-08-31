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
  double simStop = 20; //seconds
  int nodes = 3;
  int sinks = 1;
  uint32_t m_dataRate = 180;
  uint32_t m_packetSize = 32;

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

  LogComponentEnable ("ASBroadcastMac", LOG_LEVEL_INFO);

  //to change on the fly
  CommandLine cmd;
  cmd.AddValue ("simStop", "Length of simulation", simStop);
  cmd.AddValue ("nodes", "Amount of regular underwater nodes", nodes);
  cmd.AddValue ("sinks", "Amount of underwater sinks", sinks);
  cmd.Parse(argc,argv);

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

  for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      position->Add(boundry);

      devices.Add(asHelper.Create(*i, newDevice));

      NS_LOG_DEBUG("Node: " << *i << " newDevice: " << newDevice << " Position: " <<
		     boundry.x << "," << boundry.y << "," << boundry.z <<
		     " freq:" << newDevice->GetPhy()->GetFrequency() << " addr:" <<
         AquaSimAddress::ConvertFrom(newDevice->GetAddress()).GetAsInt() );
		     //<<
		     //" NDtypeid:" << newDevice->GetTypeId() <<
		     //" Ptypeid:" << newDevice->GetPhy()->GetTypeId());

      boundry.x += 2000;
    }

  for (NodeContainer::Iterator i = sinksCon.Begin(); i != sinksCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      position->Add(boundry);

      devices.Add(asHelper.Create(*i, newDevice));

      NS_LOG_DEBUG("Sink: " << *i << " newDevice: " << newDevice << " Position: " <<
		     boundry.x << "," << boundry.y << "," << boundry.z);

      boundry.x += 2000;
    }


  mobility.SetPositionAllocator(position);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(nodesCon);
  mobility.Install(sinksCon);

  PacketSocketAddress socket;
  socket.SetAllDevices();
  // socket.SetSingleDevice (devices.Get(0)->GetIfIndex());
  socket.SetPhysicalAddress (devices.Get(0)->GetAddress());
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

  std::cout << "-----------Running Simulation-----------\n";
  Simulator::Stop(Seconds(simStop + 1));
  Simulator::Run();
  Simulator::Destroy(); //null all nodes too??

  std::cout << "fin.\n";
  return 0;
}
