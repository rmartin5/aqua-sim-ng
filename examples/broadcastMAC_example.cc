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
  for (int i = 0; i < nodes; i++)
    {
      //select i-th node within nodesCon

      //attach mobility model to node and set vector location
      //create new as-netdevice
      //

      //add device ( asHelper.Create(this node, as-netdevice)
	  //NOTE: std::vector<Ptr<AquaSimNetDevice> > m_deviceList; in channel
	  //	AND void AddDevice (Ptr<AquaSimNetDevice> device);

    }

  for (int i = 0; i < sinks; i++)
    {

    }
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
