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

#ifndef AQUA_SIM_MAC_JMAC_H
#define AQUA_SIM_MAC_JMAC_H

#include "aqua-sim-mac.h"
#include "aqua-sim-time-tag.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-mac.h"

namespace ns3 {

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Jamming MAC using basic backoff mechanism
 */
enum JammingMacScheduleType{
    TRAIN_SCHEDULE,    // schedule packet transmissions "side-by-side" so they will arrive to a sink as a "train" of packets
    RANDOM_SCHEDULE,   // randomly allocate packet transmissions
    AUCTION_SCHEDULE   // use Auction algorithm from the paper
};

class AquaSimJammingMac : public AquaSimMac
{
public:
  AquaSimJammingMac();
  int64_t AssignStreams (int64_t stream);

  static TypeId GetTypeId(void);

  // Handle a packet coming from channel
  virtual bool RecvProcess (Ptr<Packet>);
  // Handle a packet coming from upper layers
  virtual bool TxProcess (Ptr<Packet>);

  // Start Channel-Competition request
  void StartCC();
  // Start Data-send phase
  void StartData();
  // Send a packet using pure ALOHA channel access
  void SendPacketAloha(Ptr<Packet> p);
  // Return a backoff value for ALOHA
  double GetBackoff();
  // Assemble all received cc-requests and send a cs-reply
  void TriggerCsReply();
  // Process incoming CC-request - obtain node info and store it
  void ProcessCcRequest(uint32_t node_id, Vector coords);
  // Create a schedule based on the received requests
  // returns: <node_id>:<tx_delay_ms>
  std::map<uint32_t, uint16_t> CreateSchedule(JammingMacScheduleType schedule_type);
  // Return a cs-reply packet based on a given schedule
  Ptr<Packet> GenerateCsReply(std::map<uint32_t, uint16_t> schedule);

  // get pairs from txt file
  std::map<uint32_t, uint32_t> getPairsFromFile(uint32_t nNodes, char matrixString[]);
  // get distance-delay between two nodes in milliseconds
  uint16_t getDistanceDelayMs(uint32_t node1, uint32_t node2);
  // calculate vulnerable area for a pair of nodes
  void calcVulnerableArea(uint32_t node1, uint32_t node2);
  // calculate energy needed for a direct transmission
  double calcEnergy(double dist);
  void calcTotalEnergy(uint32_t node1, uint32_t node2);

  // initialize the cycle at the beginning
  void initCycle();

protected:
  virtual void DoDispose();

private:
  Ptr<UniformRandomVariable> m_rand;
  uint16_t m_packetSize;
  std::list<Ptr<Packet>> m_sendQueue;

  // Interval between cc-requests at the sender side (nodes)
  Time m_epoch_time;
  // Store the incoming cc-requests in a map: <node_id>:<coords>
  std::map<uint32_t, Vector> m_request_list;
  // Guard interval in-between packets in a train
  Time m_guard_time;
  // Keep track of a number of cc-->cs-->data phases
  uint32_t m_epoch_no = 0;

  // store vulnerable area values
  std::vector<double> m_vulnerable_list;
  std::vector<double> m_energy_list;

  // this is a delay of a CS-reply sent from sink to sensor-nodes
  Time m_cs_delay;
  // this is a delay to accomodate DATA-packets transmissions from sensor-nodes to a sink
  Time m_data_delay = Seconds(10);

  // store the last schedule received from the sink
  JammingMacHeader m_current_schedule;

  // tracebacks to count the stats
  TracedCallback<double> m_trace_area;
  TracedCallback<double> m_trace_energy;
  TracedCallback<uint32_t> m_scheduled_pkts;
  TracedCallback<> m_recv_data_pkts;

};  // class AquaSimJammingMac

} // namespace ns3

#endif /* AQUA_SIM_MAC_JMAC_H */
