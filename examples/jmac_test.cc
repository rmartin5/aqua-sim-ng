/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Jamming MAC (JMAC) tests
 *
 * Author: Dmitrii Dugaev <ddugaev@gradcenter.cuny.edu>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aqua-sim-ng-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/callback.h"

#include <iomanip>

/*
 * JMAC
 *
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("JammingMacTest");

// Return random coordinates in a circle with given radius and center, at given depth (y-coord)
Vector
getBottomCoords(double center_x, double center_z, double radius, double depth, Ptr<UniformRandomVariable> random_stream)
{
  Vector boundry = Vector(0,0,0);
  double x,z;
  do
  {
    // x = random_stream->GetValue (-radius, radius);
    // z = random_stream->GetValue (-radius, radius);
    x = random_stream->GetValue (0, 2*radius);
    z = random_stream->GetValue (0, 2*radius);
  }
  while (std::sqrt (x*x + z*z) > radius);

  boundry.x = x+center_x;
  boundry.z = z+center_z;
  boundry.y = depth; // the nodes are located at the bottom
  return boundry;
}

// tracebacks for the stats
double vulnerableArea = 0;
double totalEnergy = 0;
uint32_t totalScheduledPkts = 0;
uint32_t totalRecvDataPkts = 0;
uint32_t totalQueueSize = 0;
uint32_t queueCount = 0;
uint32_t totalDelay = 0;
uint32_t delayCount = 0;
uint32_t totalOrigPkts = 0;
uint32_t totalPhyTxPkts = 0;
uint32_t totalPhyRxPkts = 0;

void
traceVulnerableArea(double area)
{
  vulnerableArea += area;
}

void
traceEnergy(double energy)
{
  totalEnergy += energy;
}

void
traceScheduledPkts(uint32_t scheduledPkts)
{
  totalScheduledPkts += scheduledPkts;
}

void
traceQueueSize(uint32_t qSize)
{
  totalQueueSize += qSize;
  queueCount += 1;
}

void
traceE2EDelay(uint32_t delay_ms)
{
  totalDelay += delay_ms;
  delayCount += 1;
}

void
traceRecvDataPkts()
{
  totalRecvDataPkts += 1;
}

void
traceRoutingRx(Ptr<const Packet> packet)
{
  totalRecvDataPkts += 1;
}

void
traceOrigPkts(Ptr<const Packet> packet)
{
  totalOrigPkts += 1;
}

void
tracePhyTx(Ptr<Packet> packet, double noise)
{
  totalPhyTxPkts += 1;
}

void
tracePhyRx(Ptr<Packet> packet, double noise)
{
  totalPhyRxPkts += 1;
}

int
main (int argc, char *argv[])
{
  double simStop = 100; //seconds
  uint32_t seed_no;
  int nodes = 2;
  int sinks = 1;
  double m_dataRate = 24;
  uint32_t m_packetSize = 100;
  // location params, meters
  double radius = 100;
  double center_x = 100;
  double center_z = 100;
  double depth = 100;
  Time epochTime = Seconds(10);
  //double range = 20;
  std::string m_mac_protocol = "jmac";

  LogComponentEnable ("JammingMacTest", LOG_LEVEL_INFO);

  //to change on the fly
  CommandLine cmd;
  cmd.AddValue ("seed", "Seed for random generation", seed_no);
  cmd.AddValue ("simStop", "Length of simulation", simStop);
  cmd.AddValue ("nodes", "Amount of regular underwater nodes", nodes);
  cmd.AddValue ("sinks", "Amount of underwater sinks", sinks);
  cmd.AddValue ("psize", "Data packet size, bytes", m_packetSize);
  cmd.AddValue ("rate", "Data rate for CBR, bps", m_dataRate);
  cmd.AddValue ("radius", "Radius of random nodes at bottom, meters", radius);
  cmd.AddValue ("center_x", "Center of a circular bottom: x-coordinate, meters", center_x);
  cmd.AddValue ("center_z", "Center of a circular bottom: z-coordinate, meters", center_z);
  cmd.AddValue ("depth", "Depth of the bottom, meters", depth);
  cmd.AddValue ("epochTime", "Time in-between 2 CC-requests at a node", epochTime);
  cmd.AddValue ("mac", "Select between TR-MAC and TDMA", m_mac_protocol);
  cmd.Parse(argc,argv);

  std::cout << "-----------Initializing simulation-----------\n";

  // Initialize pseudo-random generator
  SeedManager::SetSeed (12345);
  SeedManager::SetRun (seed_no);
  Ptr<UniformRandomVariable> random_stream = CreateObject<UniformRandomVariable> ();

  NodeContainer nodesCon;
  NodeContainer sinksCon;
  nodesCon.Create(nodes);
  sinksCon.Create(sinks);

  PacketSocketHelper socketHelper;
  socketHelper.Install(nodesCon);
  socketHelper.Install(sinksCon);

  //establish layers using helper's pre-build settings
  AquaSimChannelHelper channel = AquaSimChannelHelper::Default();
  //channel.SetPropagation("ns3::AquaSimRangePropagation");
  AquaSimHelper asHelper = AquaSimHelper::Default();
  asHelper.SetChannel(channel.Create());

  if (m_mac_protocol == "jmac")
  {
    asHelper.SetMac("ns3::AquaSimJammingMac", "PacketSize", IntegerValue(m_packetSize), "EpochTime", TimeValue(epochTime));  }
  else if (m_mac_protocol == "aloha")
  {
    asHelper.SetMac("ns3::AquaSimAloha", "AckOn", IntegerValue(0));
  }
  else
  {
    NS_FATAL_ERROR ("Unkown MAC protocol name provided!");
  }


  asHelper.SetRouting("ns3::AquaSimRoutingDummy");

  /*
   * Set up mobility model for nodes and sinks
   */
  MobilityHelper mobility;
  NetDeviceContainer devices;
  Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator> ();
  Vector boundry = Vector(0,0,0);

  std::cout << "Creating Nodes\n";

  // Place nodes at the "bottom" - a random circle in (x,z)-plane; y - depth of the "bottom"
  for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
    {
      position->Add(getBottomCoords(center_x, center_z, radius, depth, random_stream));
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      devices.Add(asHelper.Create(*i, newDevice));
      //newDevice->GetPhy()->SetTransRange(range);
    }

  // Place all sinks at the center of the circle, at 0-meter depth (y=0)
  // TODO: place multiple sinks at different positions
  for (NodeContainer::Iterator i = sinksCon.Begin(); i != sinksCon.End(); i++)
    {
      boundry.x = center_x;
      boundry.z = center_z;
      boundry.y = 0; // sink is located at the surface
      position->Add(boundry);

      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      devices.Add(asHelper.Create(*i, newDevice));
      //newDevice->GetPhy()->SetTransRange(range);
    }

  mobility.SetPositionAllocator(position);
  mobility.Install(nodesCon);
  mobility.Install(sinksCon);

  // Debug node positions
  for (uint32_t i = 0; i < nodesCon.GetN(); i++)
  {    
    Ptr<MobilityModel> mob = nodesCon.Get(i)->GetObject<MobilityModel>();
    NS_LOG_DEBUG("Node " << i << " (x,y,z)-position: (" << mob->GetPosition().x << 
                  ",\t" << mob->GetPosition().y << ",\t" << mob->GetPosition().z << ")");
  }
  for (uint32_t i = 0; i < sinksCon.GetN(); i++)
  {    
    Ptr<MobilityModel> mob = sinksCon.Get(i)->GetObject<MobilityModel>();
    NS_LOG_DEBUG("Sink " << i << " (x,y,z)-position: (" << mob->GetPosition().x << 
                  ",\t" << mob->GetPosition().y << ",\t" << mob->GetPosition().z << ")");
  }

  // Application and sockets
  PacketSocketAddress socket;
  socket.SetAllDevices();
  socket.SetPhysicalAddress (devices.Get(nodes)->GetAddress()); //Set dest to first sink (nodes+1 device)
  socket.SetProtocol (0);

  OnOffHelper app ("ns3::PacketSocketFactory", Address (socket));

  char duration_on[300];
  char duration_off[300];

  sprintf(duration_on, "ns3::ExponentialRandomVariable[Mean=%f]", (m_packetSize * 8) / m_dataRate);
  sprintf(duration_off, "ns3::ExponentialRandomVariable[Mean=%f]", 1 / 100.0);  // lambda

  app.SetAttribute ("OnTime", StringValue (duration_on));
  app.SetAttribute ("OffTime", StringValue (duration_off));

  app.SetAttribute ("DataRate", DataRateValue (m_dataRate));
  app.SetAttribute ("PacketSize", UintegerValue (m_packetSize));

  ApplicationContainer apps = app.Install (nodesCon);
  apps.Start (Seconds (0.5));
  apps.Stop (Seconds (simStop + 1));

  Ptr<Node> sinkNode = sinksCon.Get(0);
  TypeId psfid = TypeId::LookupByName ("ns3::PacketSocketFactory");

  Ptr<Socket> sinkSocket = Socket::CreateSocket (sinkNode, psfid);
  sinkSocket->Bind (socket);

  if (m_mac_protocol == "jmac") {
    Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NetDevice/Mac/AreaTrace", MakeCallback (&traceVulnerableArea));
    Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NetDevice/Mac/EnergyTrace", MakeCallback (&traceEnergy));
    Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NetDevice/Mac/ScheduledPktsTrace", MakeCallback (&traceScheduledPkts));
    Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NetDevice/Mac/RecvDataPktsTrace", MakeCallback (&traceRecvDataPkts));
  }
  if (m_mac_protocol == "aloha") {
    Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NetDevice/Mac/RoutingRx", MakeCallback (&traceRoutingRx));
  }
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NetDevice/Mac/QueueSizeTrace", MakeCallback (&traceQueueSize));
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NetDevice/Mac/E2EDelayTrace", MakeCallback (&traceE2EDelay));
  Config::ConnectWithoutContext ("/NodeList/*/ApplicationList/*/$ns3::Application/Tx", MakeCallback (&traceOrigPkts));
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NetDevice/Phy/Tx", MakeCallback (&tracePhyTx));
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NetDevice/Phy/Rx", MakeCallback (&tracePhyRx));

/*
 *  For channel trace driven simulation
 */
/*
  AquaSimTraceReader tReader;
  tReader.SetChannel(asHelper.GetChannel());
  if (tReader.ReadFile("channelTrace.txt")) NS_LOG_DEBUG("Trace Reader Success");
  else NS_LOG_DEBUG("Trace Reader Failure");
*/

  Packet::EnablePrinting (); //for debugging purposes
  std::cout << "-----------Running Simulation-----------\n";
  Simulator::Stop(Seconds(simStop));
  Simulator::Run();
  asHelper.GetChannel()->PrintCounters();
  Simulator::Destroy();

  // print the stats
  // std::cout << "VULNERABLE AREA: " << vulnerableArea << "\n";
  // std::cout << "TOTAL ENERGY: " << totalEnergy << "\n";

  std::ofstream results ("jmac_results.txt", std::ofstream::app);
  results << vulnerableArea << "\t" << totalEnergy << "\t"
  << totalOrigPkts << "\t"
  << totalRecvDataPkts << "\t"
  << totalScheduledPkts << "\t"
  << totalPhyTxPkts << "\t"
  << totalPhyRxPkts << "\t"
  << std::setprecision (4) << (double) totalQueueSize/queueCount << "\t"
  << std::setprecision (8) << (double) totalDelay/delayCount << "\t"
  << "\n";
  //

  std::cout << "fin.\n";
  return 0;
}
