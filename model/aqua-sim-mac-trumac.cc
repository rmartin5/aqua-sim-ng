/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 The City University of New York
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

#include "aqua-sim-mac-trumac.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-mac.h"
#include "aqua-sim-address.h"
#include "aqua-sim-time-tag.h"

#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/simulator.h"

#include <algorithm>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimTrumac");
NS_OBJECT_ENSURE_REGISTERED(AquaSimTrumac);


/* =========
TR-MAC
============ */

AquaSimTrumac::AquaSimTrumac()
{
  m_rand = CreateObject<UniformRandomVariable> ();
  m_packetSize = 800; // bytes
  m_guard_time = MilliSeconds(1);

  // schedule the very first TR-cycle, if node ID is the first one in the sequence
  Simulator::Schedule(Seconds(1), &AquaSimTrumac::initCycle, this);

  Simulator::Schedule(m_contention_timeout, &AquaSimTrumac::initDataContention, this);
}

TypeId
AquaSimTrumac::GetTypeId()
{
  static TypeId tid = TypeId("ns3::AquaSimTrumac")
      .SetParent<AquaSimMac>()
      .AddConstructor<AquaSimTrumac>()
      .AddAttribute("PacketSize", "Size of packet",
        IntegerValue(800),
        MakeIntegerAccessor (&AquaSimTrumac::m_packetSize),
        MakeIntegerChecker<uint16_t> ())
      .AddAttribute ("StartNodeId", "ID of a starting node in TR-cycle",
        UintegerValue(0),
        MakeUintegerAccessor(&AquaSimTrumac::m_start_node_id),
        MakeUintegerChecker<uint32_t> ())
      .AddAttribute ("TotalNodes", "Total number of nodes in a swarm",
        UintegerValue(0),
        MakeUintegerAccessor(&AquaSimTrumac::m_total_nodes),
        MakeUintegerChecker<uint32_t> ())
      .AddAttribute ("GuardTime", "Interval between reception and sending the next packet",
        TimeValue(MilliSeconds(1)),
        MakeTimeAccessor(&AquaSimTrumac::m_guard_time),
        MakeTimeChecker())
      .AddAttribute ("ContentionTimeout", "Start contention-based transmission if no traffic have been heard within the contention timeout",
        TimeValue(Seconds(5)),
        MakeTimeAccessor(&AquaSimTrumac::m_contention_timeout),
        MakeTimeChecker())
      .AddAttribute ("AlgoId", "Select between different next-hop selection algorithms",
        UintegerValue(0), // 0 - random; 1 - optimal (nearest-neighbor)
        MakeUintegerAccessor(&AquaSimTrumac::m_algo_id),
        MakeUintegerChecker<uint32_t> ())
    ;
  return tid;
}

int64_t
AquaSimTrumac::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_rand->SetStream(stream);
  return 1;
}

void
AquaSimTrumac::initCycle()
{
  if (m_total_nodes == 0)
  {
    NS_FATAL_ERROR ("Invalid total number of nodes!");
  }

  // if (m_device->GetNode()->GetId() == 0)
  // {
  //   Ptr<MobilityModel> senderModel = m_device->GetNode()->GetObject<MobilityModel>();
  //   std::cout << senderModel->GetPosition().x << "\n";
  // }

  // populate the global list of nodes' coordinates
  if (m_graph.size() == 0)
  {
    double dist = 0;
    for (uint32_t i = 0; i < m_total_nodes; i++)
    {
      for (uint32_t j = 0; j < m_total_nodes; j++)
      {
        // if (i < j)
        if (i != j)
        {
          Ptr<MobilityModel> senderModel = m_device->GetChannel()->GetDevice(i)->GetNode()->GetObject<MobilityModel>();
          Ptr<MobilityModel> recvModel = m_device->GetChannel()->GetDevice(j)->GetNode()->GetObject<MobilityModel>();
          dist = senderModel->GetDistanceFrom(recvModel);
          m_graph.insert(std::make_pair(std::make_pair(i,j), dist));
        }
      }
    }
    // store the sub-optimal greedy (nearest neighbor) schedule
    m_schedule = runNearestNeighborTSP();
  }
  // else
  // {
  //   // std::cout << m_graph.size() << "\n";
  //   std::vector<uint32_t> schedule = runNearestNeighborTSP();
  //   // for (uint32_t i=0; i<schedule.size(); i++)
  //   // {
  //   //   std::cout << schedule.at(i) << " ";
  //   // }
  //   // std::cout << "\n";

  //   // for (auto it = m_graph.begin(); it != m_graph.end(); it++)
  //   // {
  //   //   std::cout << it->first.first << "," << it->first.second << ": " << it->second << "\n";
  //   // }

  // }

  if (m_device->GetNode()->GetId() == m_start_node_id)
  {
    // std::cout << Simulator::Now().GetMilliSeconds() << " Starting TR-Cycle\n";

    // check if node ID is the starting one and send the very first packet/token, if yes
    m_heard_node_ids.clear();
    SendPacket();
  }
}

void
AquaSimTrumac::initDataContention()
{
  if ((Simulator::Now() - m_lastTxStamp) >= m_contention_timeout)
  {
    // std::cout << Simulator::Now().GetMilliSeconds() << ": " << m_device->GetNode()->GetId() << "\n";
    SendPacket();
    // Simulator::Schedule(m_contention_timeout, &AquaSimTrumac::initDataContention, this);
  }
  // else
  // {
  //   Simulator::Schedule(m_contention_timeout - (Simulator::Now() - m_lastTxStamp), &AquaSimTrumac::initDataContention, this);
  // }
  Simulator::Schedule(Seconds(m_rand->GetInteger(5, 10)), &AquaSimTrumac::initDataContention, this);
}

std::vector<uint32_t>
AquaSimTrumac::runNearestNeighborTSP()
{
  std::vector<uint32_t> optimalSchedule;
  std::vector<uint32_t> currentSchedule;
  double optimalSum = 100000000;
  double currentSum = 0;
  
  for (uint32_t n=0; n<m_total_nodes; n++)
  {
    currentSchedule.clear();
    currentSchedule.push_back(n);
    currentSum = 0;

    uint32_t nextNode = n;
    while (currentSchedule.size() < m_total_nodes)
    {
      double w = 1000000000;
      uint32_t currentBestNode = 0;
      for (auto it = m_graph.begin(); it != m_graph.end(); it++)
      {
        if (it->first.first == nextNode)
        {
          if (std::find(currentSchedule.begin(), currentSchedule.end(), it->first.second) == currentSchedule.end())
          {
            if (it->second < w)
            {
              w = it->second;
              currentBestNode = it->first.second;
            }
          }
        }
      }
      nextNode = currentBestNode;
      currentSchedule.push_back(nextNode);
      currentSum += w;
    }
    // check if this starting node is the best one
    if (currentSum < optimalSum)
    {
      optimalSum = currentSum;
      optimalSchedule.clear();
      optimalSchedule = currentSchedule;
    }
  }
  return optimalSchedule;
}

void
AquaSimTrumac::SendPacket()
{
  AquaSimHeader ash;
  MacHeader mach;
  // select next node_id
  uint8_t next_node_id = selectNextNode();
  // std::cout << +next_node_id << "\n";

  if (m_sendQueue.size() != 0)
  {
    // Get data packet from queue
    Ptr<Packet> data_packet = m_sendQueue.front();
    m_sendQueue.pop_front();

    // Attach mac-headers to packet
    data_packet->RemoveHeader(ash);
    mach.SetSA(ash.GetSAddr());
    mach.SetDA(ash.GetDAddr());
    mach.SetDemuxPType(MacHeader::UWPTYPE_OTHER);

    TrumacHeader trmac_h;
    trmac_h.SetPType(0);       // 0 - data packet
    trmac_h.SetNextNodeId(next_node_id);

    data_packet->AddHeader(trmac_h);
    data_packet->AddHeader(mach);
    data_packet->AddHeader(ash);

    m_lastTxStamp = Simulator::Now();

    // send packet to PHY
    SendDown(data_packet);
  }
  else
  {
    // Do not send empty token
    // // prepare to send an empty packet (pass a token)
    // Ptr<Packet> empty_packet = Create<Packet>();

    // ash.SetDirection(AquaSimHeader::DOWN);
    // ash.SetTxTime(MilliSeconds(8)); // TODO: define the token size
    // ash.SetSize(10); // TODO: define the token size

    // mach.SetSA(AquaSimAddress::ConvertFrom(GetAddress()));
    // mach.SetDA(AquaSimAddress::GetBroadcast());
    // mach.SetDemuxPType(MacHeader::UWPTYPE_OTHER);

    // TrumacHeader trmac_h;
    // trmac_h.SetPType(1);       // 1 - empty token
    // trmac_h.SetNextNodeId(next_node_id);

    // empty_packet->AddHeader(trmac_h);
    // empty_packet->AddHeader(mach);
    // empty_packet->AddHeader(ash);

    // // send token to PHY
    // SendDown(empty_packet);
  }
}

uint32_t
AquaSimTrumac::selectNextNode()
{
  uint32_t next_node = 0;

  if (m_algo_id == 0)
  {
    // if all the nodes have been overheard -> finish the TR-MAC cycle by selecting the starting node
    if (m_heard_node_ids.size() == m_total_nodes - 1)
    {
      return m_start_node_id;
    }
 
    // random selection
    next_node = m_rand->GetInteger(0, m_total_nodes-1);
    // while ((next_node == m_device->GetNode()->GetId()) || (next_node == m_start_node_id) || (std::find(m_heard_node_ids.begin(), m_heard_node_ids.end(), next_node) != m_heard_node_ids.end()))
    while ((next_node == m_device->GetNode()->GetId()) || (std::find(m_heard_node_ids.begin(), m_heard_node_ids.end(), next_node) != m_heard_node_ids.end()))
    {
      // select another one
      next_node = m_rand->GetInteger(0, m_total_nodes-1);
      // std::cout << m_device->GetNode()->GetId() << " " << next_node << " " << m_heard_node_ids.size() << '\n';
    }
    return next_node;
  }
  else if (m_algo_id == 1)
  {
    // sub-optimal TSP (greedy/nearest-neighbor selection TSP)
    for (uint32_t i=0; i<m_schedule.size(); i++)
    {
      if (m_schedule.at(i) == m_device->GetNode()->GetId())
      {
        if (i != m_schedule.size()-1)
        {
          next_node = m_schedule.at(i+1);
        }
        else
        {
          // the last node in the schedule --> go to the first node
          next_node = m_schedule.at(0);
        }
        break;
      }
    }
    return next_node;
  }
  else
  {
    NS_FATAL_ERROR ("Unknown algorithm!");
    return 0;
  }
}

bool
AquaSimTrumac::RecvProcess (Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this);

	AquaSimHeader ash;
  MacHeader mach;
  TrumacHeader trmac_h;
  pkt->RemoveHeader(ash);
  pkt->RemoveHeader(mach);
  pkt->RemoveHeader(trmac_h);
	AquaSimAddress dst = mach.GetDA();
	AquaSimAddress src = mach.GetSA();

  if ((Simulator::Now() - m_lastTxStamp) < m_contention_timeout)
  {
    // drop packet
  }

  m_lastTxStamp = Simulator::Now();

	if (ash.GetErrorFlag())
	{
		NS_LOG_DEBUG("TrMac:RecvProcess: received corrupt packet.");
		pkt=0;
		return false;
	}

  if (m_algo_id == 0)
  {
    // place the overheard node id to the list
    uint8_t heard_id = getIdbyAddress(src);
    if (heard_id == m_start_node_id)
    {
      m_heard_node_ids.clear();
    }
    if (std::find(m_heard_node_ids.begin(), m_heard_node_ids.end(), heard_id) == m_heard_node_ids.end())
    {
      m_heard_node_ids.push_back(heard_id);
    }
    // read the next sender id
    // std::cout << "RECV NEXT: " << +trmac_h.GetNextNodeId() << " " << m_device->GetNode()->GetId() << "\n";
    if (trmac_h.GetNextNodeId() == m_device->GetNode()->GetId())
    {
      if ((Simulator::Now() - m_lastRxTokenStamp) > Seconds(0.2))
      {
        if (m_device->GetNode()->GetId() == m_start_node_id)
        {
          // initCycle();
          Simulator::Schedule(m_guard_time, &AquaSimTrumac::initCycle, this);
        }
        else
        {
          // if the sender id matches with the receiver id, then send a packet/token
          // SendPacket();
          Simulator::Schedule(m_guard_time, &AquaSimTrumac::SendPacket, this);
        }
        m_lastRxTokenStamp = Simulator::Now();
      }
    }
  }
  else if (m_algo_id == 1)
  {
    if (trmac_h.GetNextNodeId() == m_device->GetNode()->GetId())
    {
      // if the sender id matches with the receiver id, then send a packet/token
      // SendPacket();
      Simulator::Schedule(m_guard_time, &AquaSimTrumac::SendPacket, this);
    }
  }


  if (AquaSimAddress::ConvertFrom(m_device->GetAddress()) == dst)
  {
    // trace the E2E delay
    AquaSimTimeTag timeTag;
    pkt->RemovePacketTag(timeTag);
    m_e2eDelayTrace((Simulator::Now() - timeTag.GetTime()).GetMilliSeconds());

    pkt->AddHeader(ash);
    SendUp(pkt);
  }

	pkt=0;
	return false;
}

uint32_t
AquaSimTrumac::getIdbyAddress(AquaSimAddress address)
{
	for (uint32_t i=0; i < m_device->GetChannel()->GetNDevices(); i++)
	{
		if (address == AquaSimAddress::ConvertFrom(m_device->GetChannel()->GetDevice(i)->GetAddress()))
		{
      return i;
		}
	}
  NS_FATAL_ERROR ("No node with such address!");
  return 0;
}

bool
AquaSimTrumac::TxProcess(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this << pkt);

  // Attach a timestamp tag to calculate E2E delay
  AquaSimTimeTag timeTag;
  timeTag.SetTime(Simulator::Now());
  pkt->AddPacketTag(timeTag);

  // Put incoming data packets to send-queue
  m_sendQueue.push_back(pkt);
  m_queueSizeTrace(m_sendQueue.size());

  return true;
}

void AquaSimTrumac::DoDispose()
{
  NS_LOG_FUNCTION(this);
  AquaSimMac::DoDispose();
}
