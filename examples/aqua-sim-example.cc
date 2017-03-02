/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 University of Washington
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
 */

/**
 *
 */

#include "aqua-sim-example.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/stats-module.h"
#include "ns3/applications-module.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("AquaSimExample");

Experiment::Experiment ()
  : m_numNodes (4),
    m_dataRate (80),
    m_depth (70),
    m_boundary (80),
    m_packetSize (32),
    m_bytesTotal (0),
    m_drMin (80),
    m_drMax (400),
    m_drStep (20),
    m_avgs (3),
    m_simTime (Seconds (500)),
    m_gnudatfile ("./output/aqua-sim-example.gpl"),
    m_asciitracefile ("./output/aqua-sim-example.asc"),
    m_bhCfgFile ("./output/aquasim-apps/dat/default.cfg")
{
}

void
Experiment::ResetData ()
{
  NS_LOG_DEBUG (Simulator::Now ().GetSeconds () << "  Resetting data");
  m_throughputs.push_back (m_bytesTotal * 8.0 / m_simTime.GetSeconds ());
  m_bytesTotal = 0;
}

/*
 *
 */
void
Experiment::IncreaseTraffic (uint32_t dr)
{
  double avgThroughput = 0.0;
  for (uint32_t i=0; i<m_avgs; i++)
    {
      avgThroughput += m_throughputs[i];
      //NS_LOG_DEBUG("localThroughput is: " << m_throughputs[i] << " for i: " << i);
    }
  avgThroughput /= m_avgs;
  m_data.Add (dr, avgThroughput);
//  std::cout<<"avgThroughput is:"<<avgThroughput<<"\n"<<std::endl;
  m_throughputs.clear ();

  Config::Set ("/NodeList/*/ApplicationList/*/$ns3::OnOffApplication/DataRate" , DataRateValue (dr + m_drStep));

  SeedManager::SetRun (SeedManager::GetRun () + 1);

  NS_LOG_DEBUG ("Average for traffic=" << dr << " over " << m_avgs << " runs: " << avgThroughput);
}

void
Experiment::ReceivePkt (Ptr<Socket> socket)
{
  Ptr<Packet> packet;

  while ((packet = socket->Recv ()))
    {
      m_bytesTotal += packet->GetSize ();
      //NS_LOG_DEBUG("localbyte is: " << m_bytesTotal);
    }
  packet = 0;
}

Gnuplot2dDataset
Experiment::Run (AquaSimHelper &ash)
{
  NodeContainer nc = NodeContainer ();
  NodeContainer sink = NodeContainer ();
  nc.Create (m_numNodes);
  sink.Create (1);

  PacketSocketHelper socketHelper;
  socketHelper.Install (nc);
  socketHelper.Install (sink);

  //establish layers using helper's pre-build settings
  AquaSimChannelHelper channel = AquaSimChannelHelper::Default();

  //AquaSimEnergyHelper energy;	//******this could instead be handled by node helper. ****/
  ash.SetChannel(channel.Create());

  //Create net device and nodes
  NetDeviceContainer devices;
  NetDeviceContainer sinkdev;

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator> ();
  Ptr<UniformRandomVariable> urv = CreateObject<UniformRandomVariable> ();
  position->Add (Vector (m_boundary / 2.0, m_boundary / 2.0, m_depth));
      double rsum = 0;

      std::cout << "Creating Nodes\n";
      double minr = 2 * m_boundary;
      for (uint32_t i = 0; i < m_numNodes; i++)
        {
          double x = urv->GetValue (0, m_boundary);
          double y = urv->GetValue (0, m_boundary);
          double newr = std::sqrt ((x - m_boundary / 2.0) * (x - m_boundary / 2.0)
                              + (y - m_boundary / 2.0) * (y - m_boundary / 2.0));
          rsum += newr;
          minr = std::min (minr, newr);
          position->Add (Vector (x, y, m_depth));

        }
      NS_LOG_DEBUG ("Mean range from gateway: " << rsum / m_numNodes
                                                << "    min. range " << minr);

      mobility.SetPositionAllocator (position);//stores m_numNodes+1 position information
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install (sink);//allocate position for sink first and then nc

      NS_LOG_DEBUG ("Position of sink: "
                    << sink.Get (0)->GetObject<MobilityModel> ()->GetPosition ());//aggregation using father class
      mobility.Install (nc);
      for(uint32_t i=0;i < m_numNodes;i++){
    	  NS_LOG_DEBUG("Position of node "
    			  << i << ": "<<nc.Get(i)->GetObject<MobilityModel>()->GetPosition());
      }

//  std::cout << "Creating Nodes\n";

  for (NodeContainer::Iterator i = nc.Begin(); i != nc.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();

      devices.Add(ash.Create(*i, newDevice));

    }

  for (NodeContainer::Iterator i = sink.Begin(); i != sink.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();

      sinkdev.Add(ash.Create(*i, newDevice));

    }


  PacketSocketAddress socket;
  socket.SetAllDevices();
//  socket.SetSingleDevice (devices.Get(0)->GetIfIndex());
  socket.SetPhysicalAddress (devices.Get (0)->GetAddress());
  socket.SetProtocol (1);

    /*
     * PacketSocketFactory can be used as an interface in a node in order for the node to
     * generate PacketSockets that can connect to net devices
     */
    OnOffHelper app ("ns3::PacketSocketFactory", Address (socket));
    app.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    app.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    app.SetAttribute ("DataRate", DataRateValue (m_dataRate));
    app.SetAttribute ("PacketSize", UintegerValue (m_packetSize));

    ApplicationContainer apps = app.Install (nc);
    apps.Start (Seconds (0.5));
    Time nextEvent = Seconds (0.5);


    for (uint32_t dr = m_drMin; dr <= m_drMax; dr += m_drStep)
      {

        for (uint32_t an = 0; an < m_avgs; an++)
          {
            nextEvent += m_simTime;
            Simulator::Schedule (nextEvent, &Experiment::ResetData, this);
          }
        Simulator::Schedule (nextEvent, &Experiment::IncreaseTraffic, this, dr);
      }
    apps.Stop (nextEvent + m_simTime);

    Ptr<Node> sinkNode = sink.Get (0);
    TypeId psfid = TypeId::LookupByName ("ns3::PacketSocketFactory");
    if (sinkNode->GetObject<SocketFactory> (psfid) == 0)
      {
        Ptr<PacketSocketFactory> psf = CreateObject<PacketSocketFactory> ();
        sinkNode->AggregateObject (psf);
      }

    Ptr<Socket> sinkSocket = Socket::CreateSocket (sinkNode, psfid);
    sinkSocket->Bind (socket);

    /*
     * Notify application when new data is available to be read.
     */
    sinkSocket->SetRecvCallback (MakeCallback (&Experiment::ReceivePkt,this));

    m_bytesTotal = 0;

/*    std::ofstream ascii (m_asciitracefile.c_str ());
    if (!ascii.is_open ())
      {
        NS_FATAL_ERROR ("Could not open ascii trace file: "
                        << m_asciitracefile);
      }
    ash.EnableAsciiAll (ascii);
*/
    Simulator::Stop(Seconds(50000));
    Simulator::Run ();
    sinkNode = 0;
    sinkSocket = 0;
    for (uint32_t i=0; i < nc.GetN (); i++)
      {
        nc.Get (i) = 0;
      }
    for (uint32_t i=0; i < sink.GetN (); i++)
      {
        sink.Get (i) = 0;
      }

    for (uint32_t i=0; i < devices.GetN (); i++)
      {
        devices.Get (i) = 0;
      }

    Simulator::Destroy ();
    return m_data;
}

int
main (int argc, char **argv)
{

  LogComponentEnable ("AquaSimExample", LOG_LEVEL_ALL);
//  LogComponentEnable("OnOffApplication",LOG_LEVEL_ALL);

  Experiment exp;

  std::string gnudatfile ("aquasimgnuout.dat");

  CommandLine cmd;
  cmd.AddValue ("NumNodes", "Number of transmitting nodes", exp.m_numNodes);
  cmd.AddValue ("Depth", "Depth of transmitting and sink nodes", exp.m_depth);
  cmd.AddValue ("RegionSize", "Size of boundary in meters", exp.m_boundary);
  cmd.AddValue ("PacketSize", "Generated packet size in bytes", exp.m_packetSize);
  cmd.AddValue ("DataRate", "DataRate in bps", exp.m_dataRate);
  cmd.AddValue ("DataRateMin", "Min DataRate to simulate", exp.m_drMin);
  cmd.AddValue ("DataRateMax", "Max DataRate to simulate", exp.m_drMax);
  cmd.AddValue ("Averages", "Number of topologies to test for each datarate point", exp.m_avgs);
  cmd.AddValue ("GnuFile", "Name for GNU Plot output", exp.m_gnudatfile);
  cmd.Parse (argc, argv);

  AquaSimHelper ash= AquaSimHelper::Default();
  Gnuplot gp;
  Gnuplot2dDataset ds;
  ds = exp.Run (ash);

  gp.AddDataset (ds);

  std::ofstream of (exp.m_gnudatfile.c_str ());
  if (!of.is_open ())
    {
      NS_FATAL_ERROR ("Can not open GNU Plot outfile: " << exp.m_gnudatfile);
    }
  gp.GenerateOutput (of);

  std::cout << "Simulation end...\n";
  return 0;
}

