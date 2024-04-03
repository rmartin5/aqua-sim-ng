/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * MAC tests across TRUMAC, ALOHA and TDMA protocols
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
 * TRUMAC
 *
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TrumacTest");


// tracebacks for the stats
uint32_t totalRecvDataPkts = 0;
uint32_t totalQueueSize = 0;
uint32_t queueCount = 0;
uint32_t totalDelay = 0;
uint32_t delayCount = 0;
uint32_t totalOrigPkts = 0;
uint32_t totalPhyTxPkts = 0;
uint32_t totalPhyRxPkts = 0;
uint32_t totalPhyRxColls = 0;


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
traceRecvDataPkts(Ptr<const Packet>)
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

void
tracePhyRxColl()
{
  totalPhyRxColls += 1;
}

int
main (int argc, char *argv[])
{
  double simStop = 100; //seconds
  uint32_t seed_no;
  int nodes = 2;
  double dist = 100;
  double m_dataRate = 24;
  uint32_t m_packetSize = 40;
  uint32_t m_algo_id = 0;

  std::string m_mac_protocol = "trumac";

  LogComponentEnable ("TrumacTest", LOG_LEVEL_INFO);

  //to change on the fly
  CommandLine cmd;
  cmd.AddValue ("seed", "Seed for random generation", seed_no);
  cmd.AddValue ("simStop", "Length of simulation", simStop);
  cmd.AddValue ("nodes", "Amount of regular underwater nodes", nodes);
  cmd.AddValue ("psize", "Data packet size, bytes", m_packetSize);
  cmd.AddValue ("rate", "Data rate for CBR, bps", m_dataRate);
  cmd.AddValue ("mac", "Select between TR-MAC and TDMA", m_mac_protocol);
  cmd.AddValue ("dist", "Distance between two nodes, in meters", dist);
  cmd.AddValue ("algo", "Switch between random selection and sub-optimal TSP-cucle", m_algo_id);
  cmd.Parse(argc,argv);

  std::cout << "-----------Initializing simulation-----------\n";

  // Initialize pseudo-random generator
  SeedManager::SetSeed (12345);
  SeedManager::SetRun (seed_no);
  Ptr<UniformRandomVariable> random_stream = CreateObject<UniformRandomVariable> ();

  NodeContainer nodesCon;
  nodesCon.Create(nodes);

  PacketSocketHelper socketHelper;
  socketHelper.Install(nodesCon);

  //establish layers using helper's pre-build settings
  AquaSimChannelHelper channel = AquaSimChannelHelper::Default();
  channel.SetPropagation("ns3::AquaSimRangePropagation");
  AquaSimHelper asHelper = AquaSimHelper::Default();
  asHelper.SetChannel(channel.Create());

  if (m_mac_protocol == "trumac")
  {
    asHelper.SetMac("ns3::AquaSimTrumac", "PacketSize", IntegerValue(m_packetSize), "StartNodeId", UintegerValue(0), 
                                        "TotalNodes", UintegerValue(nodes), "AlgoId", UintegerValue(m_algo_id),
                                        "GuardTime", TimeValue(MilliSeconds(1)));
  }
  else if (m_mac_protocol == "tdma")
  {
    asHelper.SetMac("ns3::AquaSimTdmaMac", "TdmaSlotPeriod", UintegerValue(nodes), "TdmaSlotDuration", TimeValue(Seconds(0.6)));
  }
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

  std::cout << "Creating Nodes\n";
  for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      devices.Add(asHelper.Create(*i, newDevice));
      newDevice->GetPhy()->SetTransRange(1500);
      if (m_mac_protocol == "tdma")
      {
        newDevice->GetMac()->SetAttribute("TdmaSlotNumber", UintegerValue(newDevice->GetNode()->GetId()));
      }
    }

  // Allocate nodes randomly within a circle
  // mobility.SetPositionAllocator("ns3::UniformDiscPositionAllocator", "X", DoubleValue(0),
  //                                     "Y", DoubleValue(0), "rho", DoubleValue(dist/2)); // TODO: change to Max.Tx.Range/2
  // mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  ObjectFactory pos;
  pos.SetTypeId("ns3::UniformDiscPositionAllocator");
  pos.Set("X", DoubleValue(0));
  pos.Set("Y", DoubleValue(0));
  pos.Set("rho", DoubleValue(dist/2));
  Ptr<PositionAllocator> positionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                              "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=0.5]"),
                              "Pause", StringValue ("ns3::UniformRandomVariable[Min=0.1|Max=3.0]"),
                              "PositionAllocator", PointerValue (positionAlloc));

  mobility.Install(nodesCon);

  // // Allocate nodes in a fixed grid
  // mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
  //                                   "MinX", DoubleValue (0.0),
  //                                   "MinY", DoubleValue (0.0),
  //                                   "DeltaX", DoubleValue (dist),
  //                                   "DeltaY", DoubleValue (dist),
  //                                   "GridWidth", UintegerValue (sqrt(nodes)),
  //                                   // "GridWidth", UintegerValue (2), // for 3-node triangle test
  //                                   "LayoutType", StringValue ("RowFirst"));
  // mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  // mobility.Install(nodesCon);

  // Debug node positions
  for (uint32_t i = 0; i < nodesCon.GetN(); i++)
  {    
    Ptr<MobilityModel> mob = nodesCon.Get(i)->GetObject<MobilityModel>();
    NS_LOG_DEBUG("Node " << i << " (x,y,z)-position: (" << mob->GetPosition().x << 
                  ",\t" << mob->GetPosition().y << ",\t" << mob->GetPosition().z << ")");
  }

  int j = 0;
  char duration_on[300];
  char duration_off[300];
  for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
  {
	  AquaSimApplicationHelper app ("ns3::PacketSocketFactory", nodes);

    // app.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    // app.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    // app.SetAttribute ("DataRate", DataRateValue (m_dataRate));
    // app.SetAttribute ("PacketSize", UintegerValue (m_packetSize));
  
	  sprintf(duration_on, "ns3::ExponentialRandomVariable[Mean=%f]", (m_packetSize * 8) / m_dataRate);
	  sprintf(duration_off, "ns3::ExponentialRandomVariable[Mean=%f]", 1 / 100.0);  // lambda
 
 	  app.SetAttribute ("OnTime", StringValue (duration_on));
	  app.SetAttribute ("OffTime", StringValue (duration_off));

    app.SetAttribute ("DataRate", DataRateValue (m_dataRate));
    app.SetAttribute ("PacketSize", UintegerValue (m_packetSize));

    ApplicationContainer apps = app.Install (nodesCon.Get(j));

    // // start traffic only on a single node
    // if (j == 0)
    // {
      apps.Start (Seconds (0.5));
      apps.Stop (Seconds (simStop + 1));
    // }
    // else
    // {
    //   apps.Start (Seconds (100000));
    //   apps.Stop (Seconds (100000));
    // }

    // // generate 1000 packets side-by-side
    // apps.Start (Seconds (0.001));
    // apps.Stop (Seconds (0.08 * 1001 + 0.001));

    j++;
  }

  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NetDevice/Mac/RoutingRx", MakeCallback (&traceRecvDataPkts));
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NetDevice/Mac/QueueSizeTrace", MakeCallback (&traceQueueSize));
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NetDevice/Mac/E2EDelayTrace", MakeCallback (&traceE2EDelay));
  Config::ConnectWithoutContext ("/NodeList/*/ApplicationList/*/$ns3::Application/Tx", MakeCallback (&traceOrigPkts));
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NetDevice/Phy/Tx", MakeCallback (&tracePhyTx));
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NetDevice/Phy/Rx", MakeCallback (&tracePhyRx));
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NetDevice/Phy/RxColl", MakeCallback (&tracePhyRxColl));

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
  // Simulator::Stop(Seconds(simStop + 0.5));
  Simulator::Stop(Seconds(simStop));
  Simulator::Run();
  asHelper.GetChannel()->PrintCounters();
  Simulator::Destroy();

  // print the stats
  std::ofstream results ("results_trumac.txt", std::ofstream::app);
  results << totalOrigPkts << "\t"
  << totalRecvDataPkts << "\t"
  << totalPhyTxPkts << "\t"
  << totalPhyRxPkts << "\t"
  << totalPhyRxColls << "\t"
  << std::setprecision (4) << (double) totalQueueSize/queueCount << "\t"
  << std::setprecision (8) << (double) totalDelay/delayCount << "\t"
  << "\n";
  //

  std::cout << "fin.\n";
  return 0;
}
