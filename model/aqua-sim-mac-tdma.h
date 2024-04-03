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

#ifndef AQUA_SIM_MAC_TDMA_H
#define AQUA_SIM_MAC_TDMA_H

#include "aqua-sim-mac.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-mac.h"

namespace ns3 {

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Classic TDMA MAC implementation
 */

class AquaSimTdmaMac : public AquaSimMac
{
public:

  typedef enum {
    IDLE, TX
  } TdmaState;

  AquaSimTdmaMac();
  int64_t AssignStreams (int64_t stream);

  static TypeId GetTypeId(void);

  // Handle a packet coming from channel
  virtual bool RecvProcess (Ptr<Packet>);
  // Handle a packet coming from upper layers
  virtual bool TxProcess (Ptr<Packet>);

  void SendPacket (Ptr<Packet> packet);
  // TDMA methods
  void StartTdma();
  // schedule the transmission in the next available slot
  void ScheduleNextSlotTx(Ptr<Packet> packet);

protected:
  virtual void DoDispose();

private:
  Ptr<UniformRandomVariable> m_rand;
  uint16_t m_packetSize;

  // TDMA-related variables
  TdmaState m_tdma_state;
  Time m_packet_start_ts = Seconds(0);
  std::list<Ptr<Packet>> m_send_queue;

  uint16_t m_tdma_slot_period;  // how many slots in 1 TDMA round
  Time m_tdma_slot_ms;
  Time m_tdma_guard_interval_ms;
  uint32_t m_tdma_slot_number;  // store the TDMA slot Tx number of this node

};  // class AquaSimTdmaMac

} // namespace ns3

#endif /* AQUA_SIM_MAC_TDMA_H */
