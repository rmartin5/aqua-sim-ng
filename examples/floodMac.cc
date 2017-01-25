/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Moshin Raza Jafri <mohsin.jafri@unive.it>
 */


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/energy-module.h"
#include "ns3/aqua-sim-ng-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/callback.h"
#include <math.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FloodingMac");

class LocalExperiment
{
public:
  LocalExperiment();
  void Run();
  void RecvPacket(Ptr<Socket> socket);
  void UpdatePosition(ConstantVelocityHelper helper);
  Box m_topo;
  double simStop; //seconds
  int nodes;
  int sinks;
  uint32_t m_dataRate;
  uint32_t m_packetSize;
  double m_minSpeed; // min speed of node
  double m_maxSpeed; // max speed of node
};

void
LocalExperiment::UpdatePosition(ConstantVelocityHelper helper)
{
  helper.UpdateWithBounds(m_topo);
}

LocalExperiment::LocalExperiment() :
  simStop(100), nodes(800), sinks(1),
  m_dataRate(100), m_packetSize(32)
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
  asHelper.SetChannel(channel.Create());
  asHelper.SetEnergyModel("ns3::AquaSimEnergyModel",
                        "RxPower", DoubleValue(0.1),
                        "TxPower", DoubleValue(2.0),
                        "InitialEnergy", DoubleValue(50),
                        "IdlePower", DoubleValue(0.01));
  asHelper.SetMac("ns3::AquaSimBroadcastMac");
  asHelper.SetRouting("ns3::AquaSimDDBR");

  MobilityHelper mobility;
  MobilityHelper mobilitysink;
  NetDeviceContainer devices;
  Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator> ();
  RngSeedManager::SetSeed(1);
  RngSeedManager::SetRun (4);

  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();

  LocalExperiment flood;
  flood.m_topo = Box(0,500,0,500,0,500);
  // Vector boundry = Vector(125,125,10);
  Vector boundry = Vector((rand->GetValue(10,490)),(rand->GetValue(10,490)),10);
  int nodedis = 75;
  std::cout << "Creating Nodes\n";

  for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      // newDevice->SetTransmitDistance(100);
      position->Add(boundry);
      devices.Add(asHelper.Create(*i, newDevice));
      NS_LOG_DEBUG("Node: " << *i << " newDevice: " << newDevice << " Position:(" <<
		     boundry.x << "," << boundry.y << "," << boundry.z <<
          ") freq:" << newDevice->GetPhy()->GetFrequency()<<" id:"<<newDevice->GetAddress()<<" new id:"<<
           AquaSimAddress::ConvertFrom(newDevice->GetAddress()).GetAsInt() );

      boundry.x = rand->GetValue(20,490);
      boundry.y = rand->GetValue(20,490);
      boundry.z = rand->GetValue(20,490);
    }

  for (NodeContainer::Iterator i = sinksCon.Begin(); i != sinksCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      // newDevice->SetTransmitDistance(100);
      boundry.x += nodedis;
      boundry.y += nodedis;
      boundry.z = 500;
      position->Add(boundry);
      devices.Add(asHelper.Create(*i, newDevice));
      NS_LOG_DEBUG("Sink: " << *i << " newDevice: " << newDevice << " Position:(" <<
		     boundry.x << "," << boundry.y << "," << boundry.z << ")");

    }

  mobility.SetPositionAllocator(position);
  mobilitysink.SetPositionAllocator(position);
  mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (0, 500, 0, 500)),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1]"),
                             "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.01]"));

// mobility.SetMobilityModel ("ns3::GaussMarkovMobilityModel",
//                             "Bounds", BoxValue (Box (0, 245, 0, 245, 0, 245)),
//                             "TimeStep", TimeValue (Seconds (0.00001)),
//                             "Alpha", DoubleValue (0.85),
//                             "MeanVelocity", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=3.0]"),
//                             "MeanDirection", StringValue ("ns3::UniformRandomVariable[Min=0|Max=6.283185307]"),
//                             "MeanPitch", StringValue ("ns3::UniformRandomVariable[Min=0.05|Max=0.05]"),
//                             "NormalVelocity", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.0|Bound=0.0]"),
//                             "NormalDirection", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.2|Bound=0.4]"),
//                             "NormalPitch", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.02|Bound=0.04]"));

  mobilitysink.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (0, 500, 0, 500)),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=0.01]"),
                             "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.01]"));

  mobility.Install(nodesCon);
  mobilitysink.Install(sinksCon);

  PacketSocketAddress socket;
  socket.SetAllDevices();
  // socket.SetSingleDevice (devices.Get(0)->GetIfIndex());
  socket.SetPhysicalAddress (devices.Get(nodes+sinks-1)->GetAddress());
  // socket.SetProtocol (0);

  OnOffHelper app ("ns3::PacketSocketFactory", Address (socket));
  app.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  app.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  app.SetAttribute ("DataRate", DataRateValue (m_dataRate));
  app.SetAttribute ("PacketSize", UintegerValue (m_packetSize));
  // app.SetAttribute ("MaxBytes", UintegerValue (0));

  ApplicationContainer apps = app.Install (nodesCon.Get(0));
  apps.Start (Seconds (0.5));
  apps.Stop (Seconds (simStop + 1));

  Ptr<Node> sinkNode = sinksCon.Get(0);
  TypeId psfid = TypeId::LookupByName ("ns3::PacketSocketFactory");

  Ptr<Socket> sinkSocket = Socket::CreateSocket (sinkNode, psfid);
  sinkSocket->Bind (socket);
  // sinkSocket->SetRecvCallback (MakeCallback (&LocalExperiment::RecvPacket, this));

/*
  ApplicationContainer serverApp;
  UdpServerHelper myServer (250);
  serverApp = myServer.Install (nodesCon.Get (0));
  serverApp.Start (Seconds (0.0));
  serverApp.Stop (Seconds (simStop + 1));
*/ //TODO implement application within this example...

  Packet::EnablePrinting ();  //for debugging purposes
  std::cout << "-----------Running Simulation-----------\n";
  Simulator::Stop(Seconds(simStop + 1));
  Simulator::Run();
  Simulator::Destroy(); //null all nodes too??
  std::cout << "-----------Printing Simulation Results-----------\n";
  asHelper.GetChannel()->PrintCounters();
  std::cout << "End.\n";
}

int
main (int argc, char *argv[])
{
  LocalExperiment exp;
  LogComponentEnable ("FloodingMac", LOG_LEVEL_INFO);
  CommandLine cmd;
  cmd.AddValue ("simStop", "Length of simulation", exp.simStop);
  cmd.AddValue ("nodes", "Amount of regular underwater nodes", exp.nodes);
  cmd.AddValue ("sinks", "Amount of underwater sinks", exp.sinks);
  cmd.Parse(argc,argv);
  exp.Run();
  return 0;
}
