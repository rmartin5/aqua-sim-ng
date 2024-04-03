/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 The City University of New York
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
 * Author: Dmitrii Dugaev <ddugaev@gradcenter.cuny.edu>
 */

#include "aqua-sim-mac-jmac.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-mac.h"
#include "aqua-sim-address.h"

#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/simulator.h"

#include <iostream>
#include <fstream>
#include <algorithm>

#include<stdio.h>
#include<stdlib.h>
#define SIZE 2048

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimJammingMac");
NS_OBJECT_ENSURE_REGISTERED(AquaSimJammingMac);


/* =========
Jamming MAC
============ */

AquaSimJammingMac::AquaSimJammingMac()
{
  m_rand = CreateObject<UniformRandomVariable> ();
  m_packetSize = 800; // bytes
  m_epoch_time = Seconds(10);
  m_guard_time = MilliSeconds(1);
  m_cs_delay = Seconds(1);        // this is a delay of a CS-reply sent from sink to sensor nodes
  m_data_delay = Seconds(20);

  // schedule the very first cycle
  Simulator::Schedule(Seconds(1), &AquaSimJammingMac::initCycle, this);
}

TypeId
AquaSimJammingMac::GetTypeId()
{
  static TypeId tid = TypeId("ns3::AquaSimJammingMac")
      .SetParent<AquaSimMac>()
      .AddConstructor<AquaSimJammingMac>()
      .AddAttribute("PacketSize", "Size of packet",
        IntegerValue(800),
        MakeIntegerAccessor (&AquaSimJammingMac::m_packetSize),
        MakeIntegerChecker<uint16_t> ())
      .AddAttribute ("EpochTime", "Interval between cc-request generations",
        TimeValue(Seconds(10)),
        MakeTimeAccessor(&AquaSimJammingMac::m_epoch_time),
        MakeTimeChecker())
      .AddAttribute ("GuardTime", "Interval in-between the transmissions in a train",
        TimeValue(MilliSeconds(100)),
        MakeTimeAccessor(&AquaSimJammingMac::m_guard_time),
        MakeTimeChecker())
      .AddTraceSource ("AreaTrace",
                       "Trace an area calculation event",
                        MakeTraceSourceAccessor (&AquaSimJammingMac::m_trace_area), "ns3::Packet::TracedCallback")
      .AddTraceSource ("EnergyTrace",
                       "Trace an energy calculation event",
                        MakeTraceSourceAccessor (&AquaSimJammingMac::m_trace_energy), "ns3::Packet::TracedCallback")
      .AddTraceSource ("ScheduledPktsTrace",
                       "Trace a packet in a schedule",
                        MakeTraceSourceAccessor (&AquaSimJammingMac::m_scheduled_pkts), "ns3::Packet::TracedCallback")
      .AddTraceSource ("RecvDataPktsTrace",
                       "Trace a reception event of a data packet",
                        MakeTraceSourceAccessor (&AquaSimJammingMac::m_recv_data_pkts), "ns3::Packet::TracedCallback")
    ;
  return tid;
}

int64_t
AquaSimJammingMac::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_rand->SetStream(stream);
  return 1;
}

void
AquaSimJammingMac::initCycle()
{
  // Start the very first Channel Competition (CC) phase
  StartCC();

  // set the very first timer at the sink-node to broadcast the schedule
  Simulator::Schedule(m_epoch_time, &AquaSimJammingMac::TriggerCsReply, this);
}

void
AquaSimJammingMac::StartCC()
{
  if (m_sendQueue.size() != 0)
  {
    // Log epoch number (amount of cc --> cs --> data handshakes)
    // std::cout << "EPOCH No: " << m_epoch_no << "\n";
    // Start cc phase
    // Read aqua-sim header info from a data packet
    AquaSimHeader ash;
    m_sendQueue.front()->PeekHeader(ash);

    // Create a CC-request
    Ptr<Packet> cc_request = Create<Packet>();
    MacHeader mach;
    mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
    mach.SetDA(ash.GetDAddr());
    mach.SetDemuxPType(MacHeader::UWPTYPE_OTHER);
    JammingMacHeader jamh;
    jamh.SetPType(1);           // 1 - cc-request
    jamh.SetNodeId(m_device->GetNode()->GetId());
    // set coordinates
    Ptr<MobilityModel> mob = m_device->GetNode()->GetObject<MobilityModel>();
    Vector coords = Vector(mob->GetPosition().x, mob->GetPosition().y, mob->GetPosition().z);
    // std::cout << "SENT COORDS: " << coords << "\n";
    jamh.SetCoordinates(coords);

    cc_request->AddHeader(jamh);
    cc_request->AddHeader(mach);
    cc_request->AddHeader(ash);

    // Send CC-request towards the sink, using pure-ALOHA
    Simulator::Schedule(Seconds(GetBackoff()), &AquaSimJammingMac::SendPacketAloha, this, cc_request->Copy());
    m_epoch_no++;
  }

  // Sleep for a CS-reception phase and then send data packets in the DATA-send phase
  Simulator::Schedule(m_epoch_time + m_cs_delay, &AquaSimJammingMac::StartData, this);
}

void
AquaSimJammingMac::StartData()
{
  // Schedule packet transmissions based on the schedule
  if (m_current_schedule.GetNodeBit(m_device->GetNode()->GetId()) == 1)
  {
    AquaSimHeader ash;
    MacHeader mach;

    Time tx_delay = MilliSeconds(m_current_schedule.GetSchedule(m_device->GetNode()->GetId()));
    // Get data packet from queue
    Ptr<Packet> data_packet = m_sendQueue.front();
    m_sendQueue.pop_front();
    // Attach mac-headers to packet
    data_packet->RemoveHeader(ash);
    mach.SetSA(ash.GetSAddr());
    mach.SetDA(ash.GetDAddr());
    mach.SetDemuxPType(MacHeader::UWPTYPE_OTHER);
    JammingMacHeader data_h;
    data_h.SetPType(0);       // 0 - data packet
    data_h.SetNodeId(m_device->GetNode()->GetId());
    data_packet->AddHeader(data_h);
    data_packet->AddHeader(mach);
    data_packet->AddHeader(ash);
    
    Simulator::Schedule(tx_delay, &AquaSimJammingMac::SendDown, this, data_packet, ns3::NIDLE);

  }
  // Schedule the next CC-phase
  Simulator::Schedule(m_data_delay, &AquaSimJammingMac::StartCC, this);
}

void
AquaSimJammingMac::SendPacketAloha(Ptr<Packet> pkt)
{
  // // Check net_device status
  // // If it is IDLE, send packet down to PHY immediately
  // // otherwise, do random backoff again
  // // if (m_device->GetTransmissionStatus() == TransStatus::NIDLE)
  // if (m_device->GetTransmissionStatus() != TransStatus::SEND)
  // {
    // Call parent method to send packet down to PHY
    SendDown(pkt);
  // }
  // else
  // {
  //   // Do another backoff
  //   Simulator::Schedule(Seconds(GetBackoff()), &AquaSimJammingMac::SendPacketAloha, this, pkt->Copy());
  // }
}

// Return a random number in range
double AquaSimJammingMac::GetBackoff()
{
  return m_rand->GetValue(0.08*0, 0.08*100);
}

std::map<uint32_t, uint32_t>
AquaSimJammingMac::getPairsFromFile(uint32_t nNodes, char matrixString[])
{
  std::map<uint32_t, uint32_t> pairs;

  uint32_t node1, node2 = 0;  // nodes in a pair
  for (uint32_t i=0; i<(nNodes*nNodes); i++)
  {
    // std::cout << matrixString[i] << "\n";
    if (matrixString[i] == '1')
    {
      node1 = i / nNodes;
      node2 = i % nNodes;
      // std::cout << node1 << ", " << node2 << "\n";
      pairs.insert(std::make_pair(node1, node2));
    }
  }

  return pairs;
}

uint16_t
AquaSimJammingMac::getDistanceDelayMs(uint32_t node1, uint32_t node2)
{
  // get the distance difference between node1 and node2
  double distance_1, distance_2 = 0;
  double diff = 0;
  // // Ptr<MobilityModel> mob = m_device->GetNode()->GetObject<MobilityModel>();
  // Ptr<MobilityModel> mob1 = m_device->GetChannel()->GetDevice(node1)->GetNode()->GetObject<MobilityModel>();
  // Ptr<MobilityModel> mob2 = m_device->GetChannel()->GetDevice(node1)->GetNode()->GetObject<MobilityModel>();
  // distance_diff = mob1->GetDistanceFrom()

	// Get the distance from the mobility model
  // Ptr<Object> sObject = m_device->GetNode();
  Ptr<Object> sObject1 = m_device->GetChannel()->GetDevice(node1)->GetNode();
  Ptr<MobilityModel> senderModel1 = sObject1->GetObject<MobilityModel> ();

  Ptr<Object> sObject2 = m_device->GetChannel()->GetDevice(node2)->GetNode();
  Ptr<MobilityModel> senderModel2 = sObject2->GetObject<MobilityModel> ();

  Ptr<Object> rObject = m_device->GetNode();
  Ptr<MobilityModel> recvModel = rObject->GetObject<MobilityModel> ();

  distance_1 = senderModel1->GetDistanceFrom(recvModel);
  distance_2 = senderModel2->GetDistanceFrom(recvModel);
  diff = distance_1 - distance_2;

  // std::cout << "Distance Diff: " << distance_diff << "\n";
  // std::cout << "Distance Delay: " << (int) distance_diff*1000/1500 << "\n";
  uint16_t d = (int) diff*1000/1500;
  if ((GetTxTime(m_packetSize).GetMilliSeconds() - d) < 0)
  {
    return 0;
  }
  else
  {
    return d;
  }
}

// calculate energy needed for a direct transmission
double
AquaSimJammingMac::calcEnergy(double l)
{
  /*
  Corresponding MATLAB code:

  l=distance;     % distance in km
  f=25;           % frequency in kHz
  k=2;            % spreading factor 1<=k<=2
  Ldata=100;      % packet size, in bytes
  R=10000;        % acoustic rate, in bps
  Prx=0.82;       % node power consumption for receiving and processing, in watts (WHOI modem is 0.82 for FSK)
  Ptmax=60;       % max transmission power, in watts (WHOI modem is 60 watts at 25khz)
  Trx=Ldata*8/R;  % seconds to receive a packet, make it equal to transmission delay
  Prmin=5.4764*10^(-8); % threshold for receiving a signal 
                      % 1.8255*10^(-8): 20W at 3km
                      % 5.4764*10^(-8): 60W at 3km
  Tpreamp=Trx;	% time to receive and decode the preamp so that 
                  % the nodes know what the destination of packet is
                  % in seconds
                  % 0.150 is the preample of UCONN OFDM modem
  C1=(0.011*f^2/(1+f^2)+44*f^2/(4100+f^2)+2.75/10000*f^2+0.003)*l;
  C2=Prmin*(l*1000)^k*8*Ldata/R;
  C3=Trx*Prx;     % energy consumptions for receiving
  result=C2*10^(C1)+C3; % energy consumption of a direct transmission  */

  l = l/1000.; // in kilometers

  double f = 25;
  uint8_t k = 2;
  uint32_t Ldata = 100;
  uint32_t R = 10000;
  double Prx = 0.82;
  // double Ptmax = 60;
  double Trx = Ldata*8/R;
  double Prmin = 5.4764 * std::pow(10, -8);
  // double Tpreamp = Trx;

  double C1 = (0.011*std::pow(f,2)/(1+std::pow(f,2))+44*std::pow(f,2)/(4100+std::pow(f,2))+2.75/10000*std::pow(f,2)+0.003)*l;
  double C2 = Prmin*std::pow(l*1000,k)*8*Ldata/R;
  double C3 = Trx*Prx;

  double result = C2*std::pow(10,C1)+C3;

  return result;
}

void
AquaSimJammingMac::calcTotalEnergy(uint32_t node1, uint32_t node2)
{
  // get the distance difference between node1 and node2
  double distance_1, distance_2 = 0;

	// Get the distance from the mobility model
  Ptr<Object> sObject1 = m_device->GetChannel()->GetDevice(node1)->GetNode();
  Ptr<MobilityModel> senderModel1 = sObject1->GetObject<MobilityModel> ();

  Ptr<Object> sObject2 = m_device->GetChannel()->GetDevice(node2)->GetNode();
  Ptr<MobilityModel> senderModel2 = sObject2->GetObject<MobilityModel> ();

  Ptr<Object> rObject = m_device->GetNode();
  Ptr<MobilityModel> recvModel = rObject->GetObject<MobilityModel> ();

  distance_1 = senderModel1->GetDistanceFrom(recvModel);
  distance_2 = senderModel2->GetDistanceFrom(recvModel);

  double totalEnergy = calcEnergy(distance_1) + calcEnergy(distance_2);
  
  m_energy_list.push_back(totalEnergy);
  m_trace_energy(totalEnergy);

  // calculate energy consumption as well
  double energy_sum = 0;
  for(uint32_t i=0; i < m_energy_list.size(); i++)
  {
    energy_sum = energy_sum + m_energy_list[i];
  }
  // std::cout << "ENERGY SUM: " << energy_sum << "\n";
}

// calculate vulnerable area for a pair of nodes
void
AquaSimJammingMac::calcVulnerableArea(uint32_t node1, uint32_t node2)
{
  // get the distance difference between node1 and node2
  double D1R, D2R, D12 = 0;

	// Get the distance from the mobility model
  Ptr<Object> sObject1 = m_device->GetChannel()->GetDevice(node1)->GetNode();
  Ptr<MobilityModel> senderModel1 = sObject1->GetObject<MobilityModel> ();

  Ptr<Object> sObject2 = m_device->GetChannel()->GetDevice(node2)->GetNode();
  Ptr<MobilityModel> senderModel2 = sObject2->GetObject<MobilityModel> ();

  Ptr<Object> rObject = m_device->GetNode();
  Ptr<MobilityModel> recvModel = rObject->GetObject<MobilityModel> ();

  D1R = senderModel1->GetDistanceFrom(recvModel);
  D2R = senderModel2->GetDistanceFrom(recvModel);

  D12 = senderModel1->GetDistanceFrom(senderModel2);

  double TH = D1R / 1500. + 0.08 - D2R/1500.;

  if (D1R > D2R)
  {
    m_trace_area(std::pow(500, 3));
    return;
  }

  // std::cout << "FOO: " << (double)hold_time_ms/1000. << "\n";
  double mu1 = 1500 * (0.08 - TH);
  double mu2 = 1500 * (0.08 + TH);

  double result1 = 3.14*mu1*(std::pow(D12,2) - std::pow(mu1,2))/8 * ( std::pow((2*500-mu1), 3) / (3*std::pow(D12,3)) + 2/3 - (2*500-mu1)/D12 );
  if (result1<0)
  {
    result1=0;
  }

  double result2 = 3.14*mu2*(std::pow(D12,2) - std::pow(mu2,2))/8 * ( std::pow((2*500-mu2), 3) / (3*std::pow(D12,3)) + 2/3 - (2*500-mu2)/D12 );
  if (result2<0)
  {
    result2=0;
  }

  // store the result in the vector
  m_vulnerable_list.push_back(result1 + result2);
  m_trace_area(result1 + result2);

  // print the intermediate average value
  double sum = 0;
  for(uint32_t i=0; i < m_vulnerable_list.size(); i++)
  {
    sum = sum + m_vulnerable_list[i];
  }
  // std::cout << "AREA MEAN: " << sum / m_vulnerable_list.size() << "\n";
  // std::cout << "AREA SUM: " << sum << "\n";
}

void
AquaSimJammingMac::TriggerCsReply()
{
  if (m_request_list.size() != 0)
  {
    // Create a schedule based on a list of the requesting nodes
    std::map<uint32_t, uint16_t> schedule = CreateSchedule(TRAIN_SCHEDULE);
    // std::map<uint32_t, uint16_t> schedule = CreateSchedule(AUCTION_SCHEDULE);
    // std::map<uint32_t, uint16_t> schedule = CreateSchedule(RANDOM_SCHEDULE);

    // Generate and broadcats the schedule (CS-reply) back
    Ptr<Packet> cs_reply = GenerateCsReply(schedule);
    // Broadcast the packet using Aloha
    // Simulator::Schedule(Seconds(GetBackoff()), &AquaSimJammingMac::SendPacketAloha, this, cs_reply->Copy());
    SendPacketAloha(cs_reply);

    m_scheduled_pkts(m_request_list.size());

    // Clear the cc-request list
    m_request_list.clear();
  }

  // set the next timer at the sink-node to broadcast the schedule
  Simulator::Schedule(m_cs_delay + m_data_delay + m_epoch_time, &AquaSimJammingMac::TriggerCsReply, this);
}

std::map<uint32_t, uint16_t>
AquaSimJammingMac::CreateSchedule(JammingMacScheduleType schedule_type)
{
  std::map<uint32_t, uint16_t> schedule;
  if (schedule_type == TRAIN_SCHEDULE)
  {
    // if we have an odd number of requests, we need to remove one since the auction algorithm takes only even number of coordinates
    if ((m_request_list.size() % 2) != 0)
    {
      m_request_list.erase(std::prev(m_request_list.end()));
    }
    // std::cout << "LIST SIZE: " << m_request_list.size() << "\n";

    // Iterate over all the cc-requests and schedule their transmissions side-by-side
    uint16_t delay_ms = 0;
    uint16_t pkt_tx_time_ms = GetTxTime(m_packetSize).GetMilliSeconds();
    // std::cout << "LIST SIZE: " << m_request_list.size() << "\n";
    uint32_t n, node1, node2 = 0;

    n = 0;
    for (auto const& x : m_request_list)
    {
      schedule.insert(std::make_pair(x.first, delay_ms));
      delay_ms += (pkt_tx_time_ms + m_guard_time.GetMilliSeconds());

      if (n == 0)
      {
        node1 = x.first;
        // std::cout << "node 1: " << x.first << "\n";
        n++;
      }
      else
      {
        node2 = x.first;
        n = 0;
        // std::cout << "node 2: " << x.first << "\n";
        // calc vulnerable area
        calcVulnerableArea(node1, node2);
      }
      // calc energy
      calcTotalEnergy(node1, node2);

    }
  }
  else if (schedule_type == RANDOM_SCHEDULE)
  {

    // if we have an odd number of requests, we need to remove one since the auction algorithm takes only even number of coordinates
    if ((m_request_list.size() % 2) != 0)
    {
      m_request_list.erase(std::prev(m_request_list.end()));
    }
    // std::cout << "LIST SIZE: " << m_request_list.size() << "\n";

    // Iterate over all the cc-requests and schedule their transmissions side-by-side
    uint16_t delay_ms = 0;
    uint16_t pkt_tx_time_ms = GetTxTime(m_packetSize).GetMilliSeconds();
    // std::cout << "LIST SIZE: " << m_request_list.size() << "\n";
    uint32_t n, node1, node2 = 0;

    // generate random node sequence
    // std::map<uint32_t, uint32_t> node_seq;
    std::list<uint32_t> node_seq;
    std::vector<uint32_t> requested_node_ids;
    for (auto const& x : m_request_list)
    {
      requested_node_ids.push_back(x.first);
    }

    uint32_t node_index = m_rand->GetInteger(0, requested_node_ids.size()-1);
    // node_seq.insert(std::make_pair(node_index, 0));
    node_seq.push_back(requested_node_ids.at(node_index));
    for (uint32_t i = 0; i < requested_node_ids.size()-1; i++)
    {
      // while (node_seq.find(node_index) != node_seq.end())
      while (std::find(node_seq.begin(), node_seq.end(), requested_node_ids.at(node_index)) != node_seq.end())
      {
        node_index = m_rand->GetInteger(0, requested_node_ids.size()-1);
      }
      // node_seq.insert(std::make_pair(node_index, node_index));
      node_seq.push_back(requested_node_ids.at(node_index));
    }

    n = 0;
    // for (auto const& x : m_request_list)
    // std::cout << "Schedule:\n";
    for (auto const& x : node_seq)
    {
      // schedule.insert(std::make_pair(x.first, delay_ms));
      schedule.insert(std::make_pair(x, delay_ms));
      delay_ms += (pkt_tx_time_ms + m_guard_time.GetMilliSeconds());

      if (n == 0)
      {
        node1 = x;
        // std::cout << "node 1: " << x.first << "\n";
        n++;
      }
      else
      {
        node2 = x;
        n = 0;
        // std::cout << "node 2: " << x.first << "\n";
        // std::cout << "node 1: " << node1 << ", node 2: " << node2 << "\n";
        // calc vulnerable area
        calcVulnerableArea(node1, node2);
      }
      // calc energy
      calcTotalEnergy(node1, node2);
    }


  }
  else if (schedule_type == AUCTION_SCHEDULE)
  {
    NS_FATAL_ERROR ("Auction schedule is currently not supported. Please refer to Matlab code.");
    // // if we have an odd number of requests, we need to remove one since the auction algorithm takes only even number of coordinates
    // if ((m_request_list.size() % 2) != 0)
    // {
    //   m_request_list.erase(std::prev(m_request_list.end()));
    // }
    // // std::cout << "LIST SIZE: " << m_request_list.size() << "\n";

    // // prepare the input coords.txt file for matlab binary
    // std::ofstream input_file("/home/ubuntu/simulation2/coords.txt");

    // input_file << m_request_list.size() << "\n";
    // std::map<uint32_t, uint32_t> node_id_map;     // maintain a map between real node ids and the ones returned by the assignemnt matrix
    // uint32_t matrix_node_id = 0;
    // for (auto const& entry : m_request_list)
    // {
    //   input_file << entry.second.x << "\n";
    //   input_file << entry.second.z << "\n";
    //   input_file << entry.second.y << "\n";
    //   node_id_map.insert(std::make_pair(matrix_node_id, entry.first));       // matrix_node_id : real_node_id
    //   matrix_node_id++;
    // }
    // input_file.close();

    // // trigger the binary file from Matlab
    // std::string str = "/home/ubuntu/simulation2/main > schedule.txt";
    // const char *command = str.c_str();
    // std::system(command);

    // // read schedule from file
    // char read_el[SIZE];
    // FILE *fp=fopen("schedule.txt", "r");

    // if(fp == NULL){
    //     printf("File Opening Error!!");

    // }
    // fgets(read_el, SIZE, fp);
    // fclose(fp);
    // // std::cout << "SCHEDULE: " << read_el << "\n";

    // std::map<uint32_t, uint32_t> pairs = getPairsFromFile(m_request_list.size(), read_el);

    // // prepare a schedule based on given pairs
    // uint16_t delay_ms = 0;
    // uint16_t pkt_tx_time_ms = GetTxTime(m_packetSize).GetMilliSeconds();
    // // std::cout << "PAIR SIZE: " << pairs.size() << "\n";
    // for (auto const& x : pairs)
    // {
    //   // schedule.insert(std::make_pair(x.first, delay_ms));
    //   // delay_ms += (pkt_tx_time_ms + m_guard_time.GetMilliSeconds());
    //   uint16_t dist_delay_ms = getDistanceDelayMs(node_id_map.find(x.first)->second, node_id_map.find(x.second)->second);

    //   // std::cout << "NODE_1: " << node_id_map.find(x.first)->second << "\n";
    //   schedule.insert(std::make_pair(node_id_map.find(x.first)->second, delay_ms));
    //   delay_ms += (pkt_tx_time_ms - dist_delay_ms + m_guard_time.GetMilliSeconds());

    //   // std::cout << "NODE_2: " << node_id_map.find(x.second)->second << "\n";
    //   schedule.insert(std::make_pair(node_id_map.find(x.second)->second, delay_ms));
    //   // delay_ms += (pkt_tx_time_ms - dist_delay_ms + m_guard_time.GetMilliSeconds());
    //   delay_ms += (pkt_tx_time_ms + m_guard_time.GetMilliSeconds()); // add just a tx_time of a packet in-between the pairs

    //   // calc vulnerable area
    //   calcVulnerableArea(node_id_map.find(x.first)->second,  node_id_map.find(x.second)->second);

    //   // calc energy
    //   calcTotalEnergy(node_id_map.find(x.first)->second,  node_id_map.find(x.second)->second);
    // }
  }
  else
  {
    NS_FATAL_ERROR ("Invalid schedule type!");
  }
  return schedule;
}

Ptr<Packet>
AquaSimJammingMac::GenerateCsReply(std::map<uint32_t, uint16_t> schedule)
{
  // Create a packet and allocate the corresponding headers
  AquaSimHeader ash;
  MacHeader mach;
  JammingMacHeader jamh;

  // Src/dst addresses
  AquaSimAddress src = AquaSimAddress::ConvertFrom(GetAddress());   // sink-node address
  AquaSimAddress dst = AquaSimAddress::GetBroadcast();              // broadcast cs-reply to all the nodes

  Ptr<Packet> cs_reply = Create<Packet>();
  ash.SetSAddr(src);
  ash.SetDAddr(dst);
  ash.SetDirection(AquaSimHeader::DOWN);
  mach.SetSA(src);
  mach.SetDA(dst);
  mach.SetDemuxPType(MacHeader::UWPTYPE_OTHER);
  // Set schedule
  JammingMacHeader cs_reply_h;
  cs_reply_h.SetPType(2);           // 2 - cs-reply
  cs_reply_h.SetNodeId(m_device->GetNode()->GetId());
  // Allocate the schedule
  for (auto const& x : schedule)
  {
    cs_reply_h.SetSchedule(x.first, x.second);  
  }
  cs_reply->AddHeader(cs_reply_h);
  cs_reply->AddHeader(mach);
  cs_reply->AddHeader(ash);
  return cs_reply;
}

void
AquaSimJammingMac::ProcessCcRequest(uint32_t node_id, Vector coords)
{
  // Store the node info in a request list
  if (m_request_list.find(node_id) == m_request_list.end())
  {
    // Insert a new entry in a map
    m_request_list.insert(std::make_pair(node_id, coords));
  }
}

bool
AquaSimJammingMac::RecvProcess (Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this);

	AquaSimHeader ash;
  MacHeader mach;
  JammingMacHeader jamh;
  pkt->RemoveHeader(ash);
  pkt->RemoveHeader(mach);
  pkt->RemoveHeader(jamh);
	AquaSimAddress dst = mach.GetDA();
	AquaSimAddress src = mach.GetSA();


	if (ash.GetErrorFlag())
	{
		NS_LOG_DEBUG("JammingMac:RecvProcess: received corrupt packet.");
		pkt=0;
		return false;
	}

  // std::cout << "Node ID: " << m_device->GetNode()->GetId() << "\n";
  // std::cout << "SRC ADDR: " << mach.GetSA() << "\n";
  // std::cout << "DST ADDR: " << mach.GetDA() << "\n";

  // If the destination unicast, then process either cc-request or data-packet
  // TODO: assert that this is actually a sink
	if (dst == AquaSimAddress::ConvertFrom(m_device->GetAddress()))
	{
    // Check the received packet type
    // If the packet is CC-request (type == 1)
    if (jamh.GetPType() == 1)
    {
      // std::cout << "Time_ms: " << Simulator::Now().GetMilliSeconds() << "\tSink:" << m_device->GetNode()->GetId() << "\tCC-request received from Node " << +jamh.GetNodeId() << "\n";
      // Handle CC-request on a sink
      // if (!m_sink_timer.IsRunning())
      // {
      //   // If the timer is not running, start it and set the expiration time
      //   m_sink_timer.SetFunction(&AquaSimJammingMac::TriggerCsReply, this);
      //   m_sink_timer.SetDelay(m_cc_accumulation_time);
      //   m_sink_timer.Schedule();
      // }

      // Accumulate all incoming cc-requests while the timer runs
      // Vector coords = Vector(0, 0, 0);
      Vector coords = jamh.GetCoordinates();
      // std::cout << "RECVD COORDS: " << coords << "\n";
      ProcessCcRequest(jamh.GetNodeId(), coords);
    }
    else if (jamh.GetPType() == 0)
    {
      // std::cout << "Time_ms: " << Simulator::Now().GetMilliSeconds() << "\tSink:" << m_device->GetNode()->GetId() << "\tData-packet received from Node " << +jamh.GetNodeId() << "\n";

      // trace the E2E delay
      AquaSimTimeTag timeTag;
      pkt->RemovePacketTag(timeTag);
      m_e2eDelayTrace((Simulator::Now() - timeTag.GetTime()).GetMilliSeconds());

      // std::cout << (Simulator::Now() - timeTag.GetTime()).GetMilliSeconds() << "\n";

      // Attach aqua-sim-header and send packet up
      pkt->AddHeader(ash);
      SendUp(pkt);
      // trace a data packet reception event
      m_recv_data_pkts();
    }
	}
  // If the destination is broadcast, then the packet is cs-reply, process it accordingly
  else if (dst == AquaSimAddress::GetBroadcast())
  {
    if (jamh.GetPType() == 2)
    {
      // std::cout << "Time_ms: " << Simulator::Now().GetMilliSeconds() << "\tNode:" << m_device->GetNode()->GetId() << "\tCS-reply received from Node " << +jamh.GetNodeId() << "\n";

      // save the schedule and then send data packets during the DATA-send phase
      m_current_schedule = jamh;

      // // Schedule packet transmissions based on the schedule
      // if (jamh.GetNodeBit(m_device->GetNode()->GetId()) == 1)
      // {
      //   Time tx_delay = MilliSeconds(jamh.GetSchedule(m_device->GetNode()->GetId()));
      //   // Get data packet from queue
      //   Ptr<Packet> data_packet = m_sendQueue.front();
      //   m_sendQueue.pop_front();
      //   // Attach mac-headers to packet
      //   data_packet->RemoveHeader(ash);
      //   mach.SetSA(ash.GetSAddr());
      //   mach.SetDA(ash.GetDAddr());
      //   mach.SetDemuxPType(MacHeader::UWPTYPE_OTHER);
      //   JammingMacHeader data_h;
      //   data_h.SetPType(0);       // 0 - data packet
      //   data_h.SetNodeId(m_device->GetNode()->GetId());
      //   data_packet->AddHeader(data_h);
      //   data_packet->AddHeader(mach);
      //   data_packet->AddHeader(ash);
        
      //   Simulator::Schedule(tx_delay, &AquaSimJammingMac::SendDown, this, data_packet, ns3::NIDLE);
      // }
    }
    else
    {
      NS_FATAL_ERROR ("The broadcast-packet is not CS-reply!");
    }
  }
  else
  {
    // TODO:
    // log the transmission, overheard from the other nodes to another destination
  }

	pkt=0;
	return false;
}

bool
AquaSimJammingMac::TxProcess(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this << pkt);

  // Attach a timestamp tag to calculate E2E delay
  AquaSimTimeTag timeTag;
  timeTag.SetTime(Simulator::Now());
  pkt->AddPacketTag(timeTag);

  // Put incoming data packets to send-queue
  m_sendQueue.push_back(pkt);

  // Trace the current queue size
  m_queueSizeTrace(m_sendQueue.size());

  // // Start the very first Channel Competition (CC) phase
  // if (!m_firstCcInit)
  // {
  //   StartCC();
  //   m_firstCcInit = true;
  // }

  return true;
}

void AquaSimJammingMac::DoDispose()
{
  NS_LOG_FUNCTION(this);
  AquaSimMac::DoDispose();
}
