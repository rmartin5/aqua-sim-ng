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

#include "aqua-sim-mac-tdma.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-mac.h"
#include "aqua-sim-address.h"
#include "aqua-sim-time-tag.h"

#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/simulator.h"

#include <algorithm>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimTdmaMac");
NS_OBJECT_ENSURE_REGISTERED(AquaSimTdmaMac);


/* =========
TDMA-MAC
============ */

AquaSimTdmaMac::AquaSimTdmaMac()
{
  m_rand = CreateObject<UniformRandomVariable> ();

  // Tdma
  m_tdma_state = IDLE;
  m_tdma_slot_period = 10;
  m_tdma_slot_ms = MilliSeconds(800); // TODO: adjust it to accomodate the max possible Tx range, pkt size and channel speed
  m_tdma_guard_interval_ms = MilliSeconds(1);
  m_tdma_slot_number = 0;

}

TypeId
AquaSimTdmaMac::GetTypeId()
{
  static TypeId tid = TypeId("ns3::AquaSimTdmaMac")
      .SetParent<AquaSimMac>()
      .AddConstructor<AquaSimTdmaMac>()
    .AddAttribute ("TdmaSlotPeriod", "Number of slots in a single TDMA round", UintegerValue(10),
                   MakeUintegerAccessor (&AquaSimTdmaMac::m_tdma_slot_period), MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("TdmaSlotNumber", "TDMA Tx slot number of the node", UintegerValue(0),
                   MakeUintegerAccessor (&AquaSimTdmaMac::m_tdma_slot_number), MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("TdmaSlotDuration", "Duration of TDMA slot", TimeValue(MilliSeconds(800)),
                   MakeTimeAccessor (&AquaSimTdmaMac::m_tdma_slot_ms), MakeTimeChecker())
    .AddAttribute ("TdmaGuardTime", "Guard time in-between two slots", TimeValue(MilliSeconds(1)),
                   MakeTimeAccessor (&AquaSimTdmaMac::m_tdma_guard_interval_ms), MakeTimeChecker())
    ;
  return tid;
}

int64_t
AquaSimTdmaMac::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_rand->SetStream(stream);
  return 1;
}

bool
AquaSimTdmaMac::RecvProcess (Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this);

	AquaSimHeader ash;
  MacHeader mach;
  pkt->RemoveHeader(ash);
  pkt->RemoveHeader(mach);
	AquaSimAddress dst = mach.GetDA();
	AquaSimAddress src = mach.GetSA();

	if (ash.GetErrorFlag())
	{
		NS_LOG_DEBUG("TdmaMac:RecvProcess: received corrupt packet.");
		pkt=0;
		return false;
	}

  // read the next sender id
  if (AquaSimAddress::ConvertFrom(m_device->GetAddress()) == dst)
  {
    // trace the E2E delay
    AquaSimTimeTag timeTag;
    pkt->RemovePacketTag(timeTag);
    m_e2eDelayTrace((Simulator::Now() - timeTag.GetTime()).GetMilliSeconds());

    pkt->AddHeader(ash);
    SendUp(pkt);
  }

	return false;
}

bool
AquaSimTdmaMac::TxProcess(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this << pkt);

  // std::cout << m_device->GetNode()->GetId() << " " << m_tdma_slot_number << "\n";

  // Attach a timestamp tag to calculate E2E delay
  AquaSimTimeTag timeTag;
  timeTag.SetTime(Simulator::Now());
  pkt->AddPacketTag(timeTag);

  // Put incoming data packets to send-queue
  m_send_queue.push_back(pkt);
  m_queueSizeTrace(m_send_queue.size());

  // Check at which state the GCSMA MAC is now (IDLE, IFS or backoffs)
  if ((m_tdma_state == IDLE))
  {
    // Start TDMA transmission
    StartTdma();
  }

  return true;
}

void
AquaSimTdmaMac::StartTdma()
{
  // If queue is empty, set TDMA state back to IDLE and return
  if (m_send_queue.size() == 0)
  {
    m_tdma_state = IDLE;
    return;
  }

  // Set TDMA state to TX
  m_tdma_state = TX;

  // Get packet from queue
  Ptr<Packet> p = m_send_queue.front();
  m_send_queue.pop_front ();

  m_packet_start_ts = Simulator::Now(); // to calculate MAC send delay
  // Schedule packet for Tx within the own slot, according to TDMA schedule
  ScheduleNextSlotTx(p);
}

void
AquaSimTdmaMac::SendPacket (Ptr<Packet> packet)
{
  AquaSimHeader ash;
  MacHeader mach;

  packet->RemoveHeader(ash);
  mach.SetSA(ash.GetSAddr());
  mach.SetDA(ash.GetDAddr());
  mach.SetDemuxPType(MacHeader::UWPTYPE_OTHER);

  packet->AddHeader(mach);
  packet->AddHeader(ash);

  // Just send packet down to PHY
  SendDown(packet);
  // Go to next packet in the queue
  StartTdma();
}

void
AquaSimTdmaMac::ScheduleNextSlotTx(Ptr<Packet> packet)
{
  // Get current slot
  uint32_t current_slot = Simulator::Now().GetMicroSeconds() / (m_tdma_slot_ms + m_tdma_guard_interval_ms).GetMicroSeconds();

  // Iterate over a single TDMA-round to find next-slot transmission
  uint32_t next_slot = 0;
  for (uint32_t i=(current_slot+1); i<(current_slot+1+m_tdma_slot_period); i++)
  {
    if ((i%m_tdma_slot_period) == m_tdma_slot_number)
    {
      next_slot = i;
    }
  }
  if (next_slot == 0)
  {
    NS_FATAL_ERROR ("Couldn't find next TDMA slot!");
  }
  if (next_slot < current_slot)
  {
    NS_FATAL_ERROR ("Next slot is smaller than current slot!");
  }

  // Schedule the packet transmission to that slot
  Simulator::Schedule(next_slot * (m_tdma_slot_ms + m_tdma_guard_interval_ms) - Simulator::Now(), &AquaSimTdmaMac::SendPacket, this, packet->Copy());
}

void
AquaSimTdmaMac::DoDispose()
{
  NS_LOG_FUNCTION(this);
  AquaSimMac::DoDispose();
}
