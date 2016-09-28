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
#include <sstream>

#include <iostream>
#include <fstream>
/*
 *  Tree like topography
 *
 *  Protocol description can be found at:
 *  Robert Martin, Sanguthevar Rajasekaran, "Data Centric Approach to
 *    Analyzing Security Threats in Underwater Sensor Networks,"
 *    in Proceedings of IEEE/MTS OCEANS, Monterey, California, USA, 2016 (to appear).
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("DdosSim");

static void
TrafficTracer (Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval)
{
  *stream->GetStream () << oldval << " " << newval << std::endl;
}

static void
TraceTraffic (std::string trafficTrFileName)
{
  AsciiTraceHelper ascii;
  if (trafficTrFileName.compare ("") == 0)
    {
      NS_LOG_DEBUG ("No trace file for traffic provided");
      return;
    }
  else
    {
      Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (trafficTrFileName.c_str ());
      Config::ConnectWithoutContext ("/NodeList/1/ns3::AquaSimNetDevice/AquaSimRouting/TrafficPkts",MakeBoundCallback (&TrafficTracer, stream));
    }
}

int
main (int argc, char *argv[])
{
  std::cout << "-----------Initializing simulation-----------\n";

  double simStop=7200; //seconds
  double preStop=60; //seconds
  int nodes=15;
  int attackers=3;
  //int sinks;
  uint32_t m_dataRate=45;
  uint32_t m_packetSize=32;
  //NOTE dataRate = 360bps(45Bps)
  //NOTE pktSize = (interest) 320 b (40 Bytes) / (data) 1 kb (128 Bytes)
  int totalDataSegments=1500;
  int traceFreq=30;

  std::string trafficTrFileName = "ddos.tr";

  LogComponentEnable ("DdosSim", LOG_LEVEL_INFO);
  //to change on the fly
  CommandLine cmd;
  cmd.AddValue ("simStop", "Length of simulation", simStop);
  cmd.AddValue ("nodes", "Amount of regular underwater nodes", nodes);
  //cmd.AddValue ("sinks", "Amount of underwater sinks", sinks);
  cmd.Parse(argc,argv);

  NodeContainer nodesCon;
  NodeContainer attackerCon;
  nodesCon.Create(nodes);
  attackerCon.Create(attackers);

  PacketSocketHelper socketHelper;
  socketHelper.Install(nodesCon);
  socketHelper.Install(attackerCon);

  //establish layers using helper's pre-build settings
  AquaSimChannelHelper channel = AquaSimChannelHelper::Default();
  AquaSimHelper asHelper = AquaSimHelper::Default();
  Ptr<AquaSimChannel> c = channel.Create();
  asHelper.SetChannel(c);
  asHelper.SetRouting("ns3::AquaSimDDOS");

  /*
   * Preset up mobility model for nodes and sinks here
   */
  MobilityHelper mobility;
  NetDeviceContainer devices;
  Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator> ();

  std::cout << "-----------Data/Path Setup-----------\n";

  //assumably paths are handled seperately in NDN and therefore simulated as already known
  std::vector<std::string> allKnownPaths, crabKnownPath, sharkKnownPath, whaleKnownPath,
                              crabSharkPath, sharkWhalePath;
  allKnownPaths.push_back("/videos/crab/");
  allKnownPaths.push_back("/videos/shark/");
  allKnownPaths.push_back("/videos/whale/");

  crabKnownPath.push_back("/videos/crab/");
  sharkKnownPath.push_back("/videos/shark/");
  whaleKnownPath.push_back("/videos/whale/");
  crabSharkPath.push_back("/videos/crab/");
  crabSharkPath.push_back("/videos/shark/");
  sharkWhalePath.push_back("/videos/shark/");
  sharkWhalePath.push_back("/videos/whale/");

  std::vector<std::string> data0, data1;
  int segmentOffset = totalDataSegments / 2;
  for (int i =0; i<segmentOffset; i++)
  {
    std::stringstream seg0, seg1;
    seg0 << "v" << i;
    seg1 << "v" << i+segmentOffset;
    data0.push_back(seg0.str());
    data1.push_back(seg1.str());
  }

  std::cout << "-----------Creating Nodes-----------\n";

  /*
  *     This should be automated...
  */

  //sink
  Ptr<AquaSimNetDevice> newDevice0 = CreateObject<AquaSimNetDevice>();
  position->Add(Vector(6000,10000,0));
  Ptr<AquaSimNetDevice> sink = asHelper.Create(nodesCon.Get(0), newDevice0);
  sink->SetSinkStatus();
  sink->GetRouting()->AssignInternalDataPath(allKnownPaths);  //slightly awkward but works for this.
  sink->SetIfIndex(0);
  devices.Add(sink);

  //row 1
  Ptr<AquaSimNetDevice> newDevice1 = CreateObject<AquaSimNetDevice>();
  position->Add(Vector(4000,8000,0));
  Ptr<AquaSimNetDevice> dev1 = asHelper.Create(nodesCon.Get(1), newDevice1);
  dev1->GetRouting()->AssignInternalDataPath(crabSharkPath);
  dev1->SetIfIndex(1);
  devices.Add(dev1);

  Ptr<AquaSimNetDevice> newDevice2 = CreateObject<AquaSimNetDevice>();
  position->Add(Vector(8000,8000,0));
  Ptr<AquaSimNetDevice> dev2 = asHelper.Create(nodesCon.Get(2), newDevice2);
  dev2->GetRouting()->AssignInternalDataPath(sharkWhalePath);
  dev2->SetIfIndex(1);
  devices.Add(dev2);

  //row 2
  Ptr<AquaSimNetDevice> newDevice3 = CreateObject<AquaSimNetDevice>();
  position->Add(Vector(2000,6000,0));
  Ptr<AquaSimNetDevice> dev3 = asHelper.Create(nodesCon.Get(3), newDevice3);
  dev3->GetRouting()->AssignInternalDataPath(crabKnownPath);
  dev3->SetIfIndex(2);
  devices.Add(dev3);

  Ptr<AquaSimNetDevice> newDevice4 = CreateObject<AquaSimNetDevice>();
  position->Add(Vector(6000,6000,0));
  Ptr<AquaSimNetDevice> dev4 = asHelper.Create(nodesCon.Get(4), newDevice4);
  dev4->GetRouting()->AssignInternalDataPath(sharkKnownPath);
  dev4->SetIfIndex(2);
  devices.Add(dev4);

  Ptr<AquaSimNetDevice> newDevice5 = CreateObject<AquaSimNetDevice>();
  position->Add(Vector(10000,6000,0));
  Ptr<AquaSimNetDevice> dev5 = asHelper.Create(nodesCon.Get(5), newDevice5);
  dev5->GetRouting()->AssignInternalDataPath(whaleKnownPath);
  dev5->SetIfIndex(2);
  devices.Add(dev5);

  //row 3
  Ptr<AquaSimNetDevice> newDevice6 = CreateObject<AquaSimNetDevice>();
  position->Add(Vector(1000,3000,0));
  Ptr<AquaSimNetDevice> dev6 = asHelper.Create(nodesCon.Get(6), newDevice6);
  dev6->GetRouting()->AssignInternalDataPath(crabKnownPath);
  dev6->SetIfIndex(3);
  devices.Add(dev6);

  Ptr<AquaSimNetDevice> newDevice7 = CreateObject<AquaSimNetDevice>();
  position->Add(Vector(6000,3000,0));
  Ptr<AquaSimNetDevice> dev7 = asHelper.Create(nodesCon.Get(7), newDevice7);
  dev7->GetRouting()->AssignInternalDataPath(sharkKnownPath);
  dev7->SetIfIndex(3);
  devices.Add(dev7);

  Ptr<AquaSimNetDevice> newDevice8 = CreateObject<AquaSimNetDevice>();
  position->Add(Vector(11000,3000,0));
  Ptr<AquaSimNetDevice> dev8 = asHelper.Create(nodesCon.Get(8), newDevice8);
  dev8->GetRouting()->AssignInternalDataPath(whaleKnownPath);
  dev8->SetIfIndex(3);
  devices.Add(dev8);

  //data sources:
  Ptr<AquaSimNetDevice> newDevice9 = CreateObject<AquaSimNetDevice>();
  position->Add(Vector(0,0,0));
  Ptr<AquaSimNetDevice> dev9 = asHelper.Create(nodesCon.Get(9), newDevice9);
  dev9->GetRouting()->AssignInternalDataPath(crabKnownPath);
  dev9->GetRouting()->AssignInternalData(data0);
  dev9->SetIfIndex(4);
  devices.Add(dev9);

  Ptr<AquaSimNetDevice> newDevice10 = CreateObject<AquaSimNetDevice>();
  position->Add(Vector(2000,0,0));
  Ptr<AquaSimNetDevice> dev10 = asHelper.Create(nodesCon.Get(10), newDevice10);
  dev10->GetRouting()->AssignInternalDataPath(crabKnownPath);
  dev10->GetRouting()->AssignInternalData(data1);
  dev10->SetIfIndex(4);
  devices.Add(dev10);

  Ptr<AquaSimNetDevice> newDevice11 = CreateObject<AquaSimNetDevice>();
  position->Add(Vector(5000,0,0));
  Ptr<AquaSimNetDevice> dev11 = asHelper.Create(nodesCon.Get(11), newDevice11);
  dev11->GetRouting()->AssignInternalDataPath(sharkKnownPath);
  dev11->GetRouting()->AssignInternalData(data0);
  dev11->SetIfIndex(4);
  devices.Add(dev11);

  Ptr<AquaSimNetDevice> newDevice12 = CreateObject<AquaSimNetDevice>();
  position->Add(Vector(7000,0,0));
  Ptr<AquaSimNetDevice> dev12 = asHelper.Create(nodesCon.Get(12), newDevice12);
  dev12->GetRouting()->AssignInternalDataPath(sharkKnownPath);
  dev12->GetRouting()->AssignInternalData(data1);
  dev12->SetIfIndex(4);
  devices.Add(dev12);

  Ptr<AquaSimNetDevice> newDevice13 = CreateObject<AquaSimNetDevice>();
  position->Add(Vector(10000,0,0));
  Ptr<AquaSimNetDevice> dev13 = asHelper.Create(nodesCon.Get(13), newDevice13);
  dev13->GetRouting()->AssignInternalDataPath(whaleKnownPath);
  dev13->GetRouting()->AssignInternalData(data0);
  dev13->SetIfIndex(4);
  devices.Add(dev13);

  Ptr<AquaSimNetDevice> newDevice14 = CreateObject<AquaSimNetDevice>();
  position->Add(Vector(12000,0,0));
  Ptr<AquaSimNetDevice> dev14 = asHelper.Create(nodesCon.Get(14), newDevice14);
  dev14->GetRouting()->AssignInternalDataPath(whaleKnownPath);
  dev14->GetRouting()->AssignInternalData(data1);
  dev14->SetIfIndex(4);
  devices.Add(dev14);


  //attacker
  asHelper.SetRouting("ns3::AquaSimDDOS", "Attacker", BooleanValue(true));
  MobilityHelper attMobility;
  Ptr<ListPositionAllocator> attPosition = CreateObject<ListPositionAllocator> ();

  Ptr<AquaSimNetDevice> attackDevice0 = CreateObject<AquaSimNetDevice>();
  attPosition->Add(Vector(1000,5000,0));
  Ptr<AquaSimNetDevice> attacker0 = asHelper.Create(attackerCon.Get(0), attackDevice0);
  //attacker->SetSinkStatus();
  attacker0->GetRouting()->AssignInternalDataPath(allKnownPaths);
  attacker0->SetIfIndex(2);
  devices.Add(attacker0);

  Ptr<AquaSimNetDevice> attackDevice1 = CreateObject<AquaSimNetDevice>();
  attPosition->Add(Vector(6000,5000,0));
  Ptr<AquaSimNetDevice> attacker1 = asHelper.Create(attackerCon.Get(1), attackDevice1);
  //attacker->SetSinkStatus();
  attacker1->GetRouting()->AssignInternalDataPath(allKnownPaths);
  attacker1->SetIfIndex(2);
  devices.Add(attacker1);

  Ptr<AquaSimNetDevice> attackDevice2 = CreateObject<AquaSimNetDevice>();
  attPosition->Add(Vector(11000,5000,0));
  Ptr<AquaSimNetDevice> attacker2 = asHelper.Create(attackerCon.Get(2), attackDevice2);
  //attacker->SetSinkStatus();
  attacker2->GetRouting()->AssignInternalDataPath(allKnownPaths);
  attacker2->SetIfIndex(2);
  devices.Add(attacker2);

  int id = 0;
  for (NetDeviceContainer::Iterator i = devices.Begin(); i != devices.End(); i++)
  {
    if (id >= nodes) continue;

    Vector v = position->GetNext();
    NS_LOG_DEBUG("Node(" << id << ") Position:(" << v.x << "," << v.y << "," << v.z << ")");
    id++;
  }

  mobility.SetPositionAllocator(position);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(nodesCon);
  attMobility.SetPositionAllocator(attPosition);
  //attMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  attMobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                               "Mode", StringValue ("Time"),
                               "Time", StringValue ("500s"),
                               "Speed", StringValue ("ns3::UniformRandomVariable[Min=0.5|Max=2.0]"),
                               "Bounds", StringValue ("0|12000|0|10000")); //,
                               //"Direction", StringValue ("ns3::UniformRandomVariable[Min=4.7|Max=1.5]"));

  attMobility.Install(attackerCon);
  // NOTE Mobile Attacker Speed = 1 to 4 knots (0.5 to 2.0 m/s).

  PacketSocketAddress socket;
  socket.SetAllDevices();
  // socket.SetSingleDevice (devices.Get(0)->GetIfIndex());
  socket.SetPhysicalAddress (devices.Get(nodes-1)->GetAddress());
  socket.SetProtocol (0);


  OnOffHelper app ("ns3::PacketSocketFactory", Address (socket));
  app.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.25]"));
  app.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.75]"));
  app.SetAttribute ("DataRate", DataRateValue (m_dataRate));
  app.SetAttribute ("PacketSize", UintegerValue (m_packetSize));
  app.SetAttribute("MaxBytes", UintegerValue (0));
  //change above to create interesting interest types (pkt spikes / heavy attacks)

  /*XXX should be changed and handled by tracers... and FileHandler API.
  FILE * pFile;
  pFile = fopen ("outputFile.txt", "a");
  if (pFile!=NULL)
  {
    fputs ("Simulation Started.",pFile);
    fclose (pFile);
  }*/

  ApplicationContainer apps = app.Install (nodesCon.Get(0));
  apps.Start (Seconds (0.5));
  apps.Stop (Seconds (simStop - preStop));

  //look into this note with a quick check...
  /* NOTE besides the main traffic generator, others are having no effect besides creating additional traffic
      Since i am not seeing any change in how many pkt the sink is receiving, meaning either
      there is no recv socket set up correctly (most likely not the case),
      SinkStatus not set for node (may be true but outside of 1 sink, attackers should behave similar to sinks),
      OR protocol is behaving normally and suppressing duplicates (MOST LIKELY but needs testing.)
  */

  ApplicationContainer apps2 = app.Install (attackerCon.Get(0));
  apps2.Start (Seconds (0.5));
  apps2.Stop (Seconds (simStop - preStop));
  ApplicationContainer apps3 = app.Install (attackerCon.Get(1));
  apps3.Start (Seconds (0.5));
  apps3.Stop (Seconds (simStop - preStop));
  ApplicationContainer apps4 = app.Install (attackerCon.Get(2));
  apps4.Start (Seconds (0.5));
  apps4.Stop (Seconds (simStop - preStop));


  Ptr<Node> sinkNode = nodesCon.Get(0);
  TypeId psfid = TypeId::LookupByName ("ns3::PacketSocketFactory");

  Ptr<Socket> sinkSocket = Socket::CreateSocket (sinkNode, psfid);
  sinkSocket->Bind (socket);
  //sinkSocket->SetRecvCallback (MakeCallback (&LocalExperiment::RecvPacket, this));

/*
  ApplicationContainer serverApp;
  UdpServerHelper myServer (250);
  serverApp = myServer.Install (nodesCon.Get (0));
  serverApp.Start (Seconds (0.0));
  serverApp.Stop (Seconds (simStop + 1));
*/ //TODO implement application within this example...

  Packet::EnablePrinting ();  //for debugging purposes
  std::cout << "-----------Running Simulation-----------\n";

  Simulator::Schedule (Seconds (0.00001), &TraceTraffic, trafficTrFileName);
  //XXX should be changed and handled by tracers... and FileHandler API.

  Simulator::Stop(Seconds(simStop +1));
  for (int i = traceFreq; i < simStop; i += traceFreq )
  {
    Simulator::Schedule(Seconds(i),&AquaSimChannel::FilePrintCounters,c,i,15);
  }
  //Simulator::Schedule(Seconds(100),&AquaSimChannel::FilePrintCounters,c,100,15);
  Simulator::Run();
  Simulator::Destroy(); //null all nodes too??
  std::cout << "-----------Printing Simulation Results-----------\n";

  asHelper.GetChannel()->PrintCounters();
  asHelper.GetChannel()->FilePrintCounters(simStop,15);

  std::cout << "End.\n";

  return 0;
}
