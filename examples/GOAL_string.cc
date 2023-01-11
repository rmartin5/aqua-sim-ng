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
 //#include "ns3/energy-module.h"  //may not be needed here...
 #include "ns3/aqua-sim-ng-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/log.h"
 #include "ns3/callback.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ASGoalString");

class ASGoalString
{
public:
  void UpdatePosition(ConstantVelocityHelper helper);
  Box m_topo;
};  // class ASGoalString

void
ASGoalString::UpdatePosition(ConstantVelocityHelper helper)
{
  helper.UpdateWithBounds(m_topo);
}

int
main (int argc, char *argv[])
{
  //mobility pattern for bound checking???

  //Mac GOAL:
  int m_maxBurst = 10;
  int m_maxRetxTimes = 6;
  Time m_vbfMaxDelay = Seconds(2.0);
  //set bit_rate_  1.0e4 ;#10kbps
  //set encoding_efficiency_ 1

  double m_minSpeed = 0; // min speed of node
  double m_maxSpeed = 3; // max speed of node
  //double m_speed = 0.5; // speed of node
  //int m_routingControlPacketSize = 20; //bytes
        //unused variables
  Time m_positionUpdateInterval = Seconds(0.3); // the length of period to update node's position
  int m_packetSize = 300;

  int m_nodes = 6;
  double x = 1000;  // topography
  double y = 10;
  double z = 10;
  double m_simStop = 500; //sim time (seconds)
  double m_preStop = 90;  //time to prepare to stop
  //trace file "GOAL.tr"; // trace file
  //ifqlen		50	;# max queue length in if
  int m_dataInterval = 100; //10.0
  double m_range = 100; //range of each node in meters
  double m_width = 100; //vbf width
  //LL set mindelay_		50us
  //LL set delay_			25us


  CommandLine cmd;
  cmd.AddValue ("simStop", "Length of simulation", m_simStop);
  cmd.AddValue ("nodes", "Amount of regular underwater nodes", m_nodes);
  cmd.Parse(argc,argv);

  std::cout << "-----------Initializing simulation-----------\n";

  NodeContainer nodesCon;
  nodesCon.Create(m_nodes);

  PacketSocketHelper socketHelper;
  socketHelper.Install(nodesCon);

  //establish layers using helper's pre-build settings
  AquaSimChannelHelper channel = AquaSimChannelHelper::Default();
  AquaSimHelper asHelper = AquaSimHelper::Default();
  asHelper.SetChannel(channel.Create());
  asHelper.SetMac("ns3::AquaSimGoal",
                    "MaxBurst", IntegerValue(m_maxBurst),
                    "MaxRetxTimes", IntegerValue(m_maxRetxTimes),
                    "VBFMaxDelay", TimeValue(m_vbfMaxDelay));
  asHelper.SetRouting("ns3::AquaSimVBF",
                    "HopByHop", IntegerValue(0),
                    "EnableRouting", IntegerValue(0),
                    "Width", DoubleValue(m_width));
  asHelper.SetEnergyModel("ns3::AquaSimEnergyModel",
                    "RxPower", DoubleValue(0.75),
                    "TxPower", DoubleValue(2.0),
                    "InitialEnergy", DoubleValue(10000),
                    "IdlePower", DoubleValue(0.008));

  MobilityHelper mobility;
  NetDeviceContainer devices;
  Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator> ();

  ASGoalString goal;
  goal.m_topo = Box(0,x,0,y,0,z);

  //Trace TODO

  Vector boundry = Vector(0,0,0);
  std::cout << "Creating Nodes\n";

  for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      position->Add(boundry);
      newDevice = asHelper.Create(*i, newDevice);
      newDevice->GetPhy()->SetTransRange(m_range);
      devices.Add(newDevice);

      NS_LOG_DEBUG("Node: " << *i << " newDevice: " << newDevice << " Position: " <<
		     boundry.x << "," << boundry.y << "," << boundry.z);

      boundry.x += 100;
    }

  mobility.SetPositionAllocator(position);
  mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
  mobility.Install(nodesCon);
  //Set random velocities for all nodes
  for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
    {
      Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
      Ptr<ConstantVelocityMobilityModel> model = (*i)->GetObject<ConstantVelocityMobilityModel>();
      model->SetVelocity(Vector(rand->GetValue(m_minSpeed,m_maxSpeed),0,0));

      ConstantVelocityHelper m_helper = ConstantVelocityHelper(model->GetPosition(), model->GetVelocity());
      for (Time update = m_positionUpdateInterval; update.GetDouble() < m_simStop-Seconds(1).GetDouble(); update += m_positionUpdateInterval)
      {
        Simulator::Schedule(update, &ASGoalString::UpdatePosition, &goal, m_helper);
      }
    }
  PacketSocketAddress socket;
  socket.SetAllDevices();
  // socket.SetSingleDevice (devices.Get(0)->GetIfIndex());
  socket.SetPhysicalAddress (devices.Get(m_nodes-1)->GetAddress());
  socket.SetProtocol (0);

  OnOffHelper app ("ns3::PacketSocketFactory", Address (socket));
  app.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  app.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  app.SetAttribute ("DataRate", DataRateValue (m_dataInterval));
  app.SetAttribute ("PacketSize", UintegerValue (m_packetSize));

  ApplicationContainer apps = app.Install (nodesCon.Get(0));
  apps.Start (Seconds (0.5));
  apps.Stop (Seconds (m_simStop - m_preStop));

  Ptr<Node> sinkNode = nodesCon.Get(m_nodes-1);
  TypeId psfid = TypeId::LookupByName ("ns3::PacketSocketFactory");

  Ptr<Socket> sinkSocket = Socket::CreateSocket (sinkNode, psfid);
  sinkSocket->Bind (socket);

  Packet::EnablePrinting ();  //for debugging purposes
  std::cout << "-----------Running Simulation-----------\n";
  Simulator::Stop(Seconds(m_simStop));
  Simulator::Run();
  Simulator::Destroy();
  std::cout << "End.\n";
  return 0;
}
