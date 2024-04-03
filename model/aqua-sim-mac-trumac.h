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

#ifndef AQUA_SIM_MAC_TRUMAC_H
#define AQUA_SIM_MAC_TRMUAC_H

#include "aqua-sim-mac.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-mac.h"

namespace ns3 {

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Token-Ring MAC (TR-MAC) implementation
 */

class AquaSimTrumac : public AquaSimMac
{
public:
  AquaSimTrumac();
  int64_t AssignStreams (int64_t stream);

  static TypeId GetTypeId(void);

  // Handle a packet coming from channel
  virtual bool RecvProcess (Ptr<Packet>);
  // Handle a packet coming from upper layers
  virtual bool TxProcess (Ptr<Packet>);

  void initCycle();
  // check the sendQueue and send a packet, if the queue is not empty. Send an empty token otherwise.
  void SendPacket();

  // select next transmission node, based on the list of nodes already overheard and a total number of nodes in a swarm
  uint32_t selectNextNode();

  uint32_t getIdbyAddress(AquaSimAddress address);

  // run nearest-neightbor TSP algorithm, return sub-optimal sequence of nodes (schedule)
  std::vector<uint32_t> runNearestNeighborTSP();

  // periodically initiate a contention-based data transmission, if no transmissions have been heard within a certain time interval
  void initDataContention();

protected:
  virtual void DoDispose();

private:
  Ptr<UniformRandomVariable> m_rand;
  uint16_t m_packetSize;
  std::list<Ptr<Packet>> m_sendQueue;

  // guard interval between packet reception and a packet transmission of the next packet
  Time m_guard_time;

  // start contention-based transmission if no transmissions have happened within this timeout
  Time m_contention_timeout;

  // starting node id
  uint32_t m_start_node_id;

  // total number of nodes in a swarm
  uint32_t m_total_nodes;

  uint32_t m_algo_id;

  // store list of all nodes' coordinates
  std::map<std::pair<uint32_t, uint32_t>, double> m_graph;
  std::vector<uint32_t> m_schedule;

  // keep track of all the node ids, overheard during the TR-cycle
  std::vector<uint8_t> m_heard_node_ids;

  // keep track of the last overheard transmission
  Time m_lastTxStamp = Seconds(0);

  Time m_lastRxTokenStamp = Seconds(0);

};  // class AquaSimTrumac

} // namespace ns3

#endif /* AQUA_SIM_MAC_TRUMAC_H */
