/*
 * libra_tests
 *
 *  Created on: Aug 16, 2019
 *      Author: dmitry
 * 
 * Added kinematic mobility model
 */


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aqua-sim-ng-module.h"
#include "ns3/aqua-sim-propagation.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/callback.h"

#include <random>
#include <math.h>
#include <iomanip> // setprecision
#include <sstream> // stringstream

/*
 * MAC-Libra NxN grid random destination topology tests
 *
 *
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MAC_libra_grid_test");

int
main (int argc, char *argv[])
{
  double simStop = 100; //seconds
//  double simStop = 2; //seconds

  int n_nodes = 10;
//  int sinks = 1;
//  uint32_t m_dataRate = 80000; // bps
  double m_dataRate = 80000; // bps

  double m_packetSize = 50; // bytes
  double range = 1500;	// meters

  // Poisson traffic parameters
  double lambda = 0.1;

  // Grid parameters
  int max_x = 100; // meters
//  int max_y = 10000; // meters
//  double distance = 10; // meters

  // Max Tx power
  double max_tx_power = 60; // Watts

  // Number of intermediate nodes (only for some experiments!!!)
  int n_intermediate_nodes = 0;

  //to change on the fly
  CommandLine cmd;
  cmd.AddValue ("simStop", "Length of simulation", simStop);
  cmd.AddValue ("lambda", "Packet arrival rate", lambda);
  cmd.AddValue ("packet_size", "Packet size", m_packetSize);
  cmd.AddValue ("grid_size", "Grid size, in km", max_x);
  cmd.AddValue ("n_nodes", "Number of nodes", n_nodes);
  cmd.AddValue ("range", "Transmission range", range);
  cmd.AddValue ("tx_power", "Max transmission power", max_tx_power);
  cmd.AddValue ("intermediate_nodes", "Number of intermediate nodes", n_intermediate_nodes);


  cmd.Parse(argc,argv);

  // Random integer selection-related parameters
  std::random_device rd;     // only used once to initialise (seed) engine
  std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
  std::uniform_int_distribution<int> uni_distance(0, max_x); // guaranteed unbiased
  std::uniform_int_distribution<int> uni_nodes(0, n_nodes - 1); // guaranteed unbiased


  std::cout << "-----------Initializing simulation-----------\n";

  NodeContainer nodesCon;
//  NodeContainer sinksCon;
  nodesCon.Create(n_nodes);
//  sinksCon.Create(sinks);

  PacketSocketHelper socketHelper;
  socketHelper.Install(nodesCon);
//  socketHelper.Install(sinksCon);

  //establish layers using helper's pre-build settings
  AquaSimChannelHelper channel = AquaSimChannelHelper::Default();
  channel.SetPropagation("ns3::AquaSimRangePropagation");
  AquaSimHelper asHelper = AquaSimHelper::Default();
  asHelper.SetChannel(channel.Create());

  asHelper.SetMac("ns3::AquaSimMacLibra", "max_range", DoubleValue(range), "max_tx_power", DoubleValue(max_tx_power),
  		  "packet_size", IntegerValue(m_packetSize), "intermediate_nodes", IntegerValue(n_intermediate_nodes));

//    asHelper.SetMac("ns3::AquaSimSFama", "packet_size", DoubleValue(m_packetSize));
//    asHelper.SetMac("ns3::AquaSimBroadcastMac");
//  asHelper.SetMac("ns3::AquaSimAloha");

  asHelper.SetRouting("ns3::AquaSimRoutingDummy");

  // Define the Tx power
  asHelper.SetPhy("ns3::AquaSimPhyCmn", "PT", DoubleValue(max_tx_power));


  /*
   * Set up mobility model for nodes and sinks
   */
  MobilityHelper mobility;
  NetDeviceContainer devices;
  Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator> ();
  Vector boundry = Vector(0,0,0);

  std::cout << "Creating Nodes\n";

  for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();

      // Select random (x, y) position
      boundry.x = uni_distance(rng);
      boundry.y = uni_distance(rng);

      position->Add(boundry);
      devices.Add(asHelper.Create(*i, newDevice));

//      NS_LOG_DEBUG("Node:" << newDevice->GetAddress() << " position(x):" << boundry.x);
//      std::cout << "Node:" << newDevice->GetAddress() << " position(x):" << boundry.x <<
//    		  " position(y):" << boundry.y << "\n";
      newDevice->GetPhy()->SetTransRange(range);
//      newDevice->GetPhy()->SetTxPower(0.001);
    }

  mobility.SetPositionAllocator(position);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(nodesCon);
//  mobility.Install(sinksCon);

  int j = 0;
  char duration_on[300];
  char duration_off[300];

  // Set application to each node
  for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
  {
	  AquaSimApplicationHelper app ("ns3::PacketSocketFactory", n_nodes);

	  sprintf(duration_on, "ns3::ExponentialRandomVariable[Mean=%f]", (m_packetSize * 8) / m_dataRate);
	  sprintf(duration_off, "ns3::ExponentialRandomVariable[Mean=%f]", 1 / lambda);
//	  std::cout << "Duration On: " << duration_on << "\n";
//	  std::cout << "Duration Off: " << duration_off << "\n";
	  app.SetAttribute ("OnTime", StringValue (duration_on));
	  app.SetAttribute ("OffTime", StringValue (duration_off));

	  app.SetAttribute ("DataRate", DataRateValue (m_dataRate));
	  app.SetAttribute ("PacketSize", UintegerValue (m_packetSize));

	  ApplicationContainer apps = app.Install (nodesCon.Get(j));

	  // apps.Start (Seconds (0.5));
	  // apps.Stop (Seconds (simStop + 1));

      // Stop the application 30 seconds earlier - for more accurate throughput calculations
      apps.Start (Seconds (1));
	  apps.Stop (Seconds (simStop + 1));

	  j++;
  }

  Packet::EnablePrinting (); //for debugging purposes
  std::cout << "-----------Running Simulation-----------\n";
  Simulator::Stop(Seconds(simStop + 30));

  // Enable ASCII trace files
  Packet::EnablePrinting ();  //for debugging purposes
  char buff[1000];
  // Naming convention: lambda-number_of_nodes-n_intermediate_nodes-seed
  std::stringstream stream;
  stream << std::fixed << std::setprecision(2) << lambda;
  std::string lambda_string = stream.str();
  snprintf(buff, sizeof(buff), "libra-density-trace-%s-%d-%d.asc", lambda_string.c_str(), n_nodes, n_intermediate_nodes);
  std::string asciiTraceFile = buff;

  // std::string asciiTraceFile = "libra-trace.asc";
  // asciiTraceFile.
  std::ofstream ascii (asciiTraceFile.c_str());
  if (!ascii.is_open()) {
    NS_FATAL_ERROR("Could not open trace file.");
  }
  asHelper.EnableAsciiAll(ascii);

  Simulator::Run();

  asHelper.GetChannel()->PrintCounters();

  Simulator::Destroy();

  std::cout << "fin.\n";
  return 0;
}
