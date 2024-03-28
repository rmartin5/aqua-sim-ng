/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Author: Dmitrii Dugaev <ddugaev@gradcenter.cuny.edu>
 * based on aqua-sim-mac-aloha model with no ACKs and backoffs
 */

#ifndef AQUA_SIM_MAC_LIBRA_H
#define AQUA_SIM_MAC_LIBRA_H

#include "aqua-sim-mac.h"
#include <math.h>


namespace ns3 {

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Routing MAC, based on aloha-mac with no ACKs and backoffs
 */
class AquaSimMacLibra : public AquaSimMac
{
public:
  AquaSimMacLibra();
  int64_t AssignStreams (int64_t stream);

  static TypeId GetTypeId(void);

  // to process the incoming packet
  virtual bool RecvProcess (Ptr<Packet>);
  void CallbackProcess ();
  void DropPacket (Ptr<Packet>);

  // Forward packet / frame from upper layers / network, keep track of the sender node
//  bool ForwardPacket (Ptr<Packet>, AquaSimAddress sender_addr, int hop_count, double reward);
  bool ForwardPacket (Ptr<Packet>, AquaSimAddress sender_addr);

  // Send down frame using broadcast-mac backoff logic
  bool SendDownFrame (Ptr<Packet>);

  void SendPacket();
  double GetBackoff();

  // Send down frame TODO: fix the order of send downs
  bool Send(Ptr<Packet>);

  // Resend frame from the send_buffer
  void ResendFrame();

  // Select next_hop_node for given destination, return its address
//  AquaSimAddress SelectNextHop (AquaSimAddress dst_addr);
  AquaSimAddress SelectNextHop (AquaSimAddress src_addr, AquaSimAddress dst_addr);

  // Filter duplicate broadcast service messages
  bool FilterDuplicateInit (AquaSimAddress src_addr);

  // Generate reward packet
  double GenerateReward (AquaSimAddress dst_addr);

  // Update path weight in forwarding table by corresponding reward value
//  bool UpdateWeight(AquaSimAddress address, AquaSimAddress next_hop_addr, double reward);
  bool UpdateWeight(AquaSimAddress src_addr, AquaSimAddress dst_addr, AquaSimAddress next_hop_addr, double reward);

  // Calculate rewards based on given distances
//  double CalculateWeight (double distance);
  double CalculateReward (double optimal_metric, double direct_distance, double current_distance);

  // Calculate and update distance list
  void UpdateDistance (double tx_power, double rx, AquaSimAddress dst_addr);

  // Calculate the distance between the src and dst nodes, based on Tx and Rx powers
  // For that, a very rough approximation model of Rayleigh is used, if frequency = 25 kHz!
  double CalculateDistance (double tx_power, double rx_power);
  // Get more precise distance from mobility model (while more accurate TX/RX formula hasn't been figured out)
  double CalculateDistance (AquaSimAddress dst_addr);

  // Calculate Tx power given the distance and expected Rx_threshold
  // The calculation is based on Rayleight model, used in the aqua-sim-propagation module:
  // Rx = Tx / (d^k * alpha^(d/1000)), k = 2, alpha = 4, (f = 25kHz)
  double CalculateTxPower (double distance);

  // Calculate the number of relays that can minimize the energy consumption
  // The return value is the ideal number of relays
//  int CalculateHopCount(double distance, int packet_size, int link_speed, double p_rx, double p_tx_max);
  uint8_t CalculateHopCount(double distance, int packet_size, double p_tx_max);

  // Check if the destination node is within the maximum transmission range of the source node or not
  bool IsWithinMaximumRange (AquaSimAddress dst_addr);

  // Popolate the in_range node list
  void DiscoverInRangeNodes ();

  // Initialized the distances between this node and all the other nodes
  void InitializeDistances();

  // to process the outgoing packet
  virtual bool TxProcess (Ptr<Packet>);

  // MAC states
  enum MacRoutingStatus {
	  DATA_TX,
	  DATA_RX,
	  IDLE
    };

protected:
  virtual void DoDispose();
  // Expiration handlers. Not used for now.
  void RewardExpirationHandler(std::map<AquaSimAddress, AquaSimAddress> dst_to_next_hop_map);


private:
  Ptr<UniformRandomVariable> m_rand;

  // Forwarding table to select next-hop nodes, in a format: {dst_addr : [next_hop_addr: weight]}
//  std::map<AquaSimAddress, std::map<AquaSimAddress, double>> m_forwarding_table;
  // Map of (src, dst) pair and the corresponding weight
  std::map<std::map<AquaSimAddress, AquaSimAddress>, std::map<AquaSimAddress, double>> m_forwarding_table;

  // Store the packets in send_queue until path discovery procedure is successful or failed, in a format:
  // {dst_addr: packet_queue}
  std::map<AquaSimAddress, std::queue<Ptr<Packet>>> m_send_queue;

  // Store the packets which have been already triggered to be sent, but failed due to device not idle status
  std::queue<Ptr<Packet>> m_send_buffer;

  // Store reward expiration timestamps, needed when the expire event is triggered by the scheduler,
  // or when the reward message is received,
  // in a format: {{dst_addr : next_hop_addr} : last_recevied_reward_timestamp}
  std::map<std::map<AquaSimAddress, AquaSimAddress>, Time> m_reward_expirations;

  // Store INIT expiration timestamps, needed when the expire event is triggered by the scheduler,
  // or when the INIT message is received,
  // in a format: {dst_addr: last_recevied_init_timestamp}
  std::map<AquaSimAddress, Time> m_init_expirations;

  // Store list of already received INITs, with the last received timestamp to handle duplicate frames
  std::map<AquaSimAddress, Time> m_init_list;

  // Store list of {dst_addr, sender_addr} pairs to periodically generate the reward packets back to sender
  std::map<std::map<AquaSimAddress, AquaSimAddress>, Time> m_reward_delays;

  // ACK / Reward delay on receiver side
  Time m_reward_delay = Seconds(1);
  // Reward wait timeout, in seconds
  Time m_reward_timeout = Seconds(30) + m_reward_delay;

  // INIT wait timeout, in seconds
  Time m_init_timeout = Seconds(10);

  // Store the distances to each destination
  std::map<AquaSimAddress, double> m_distances;

  // Default negative reward value
  double m_negative_reward = -1;

  // Maximum hop-count for packets (to avoid loops)
  int m_max_hop_count = 10;

  // Max transmission range
  double m_max_range = 1500; // meters

  // Expected Rx_threshold, in Watts
  // double m_rx_threshold = 3.27 * pow(10, -8);
  // RX_threshold calculated for TX power = 60 Watts at distance of 1250 meters
  // double m_rx_threshold = 6.62568 * pow(10, -6);
  double m_rx_threshold = 5.4764*pow(10,(-8.0));
  // RX_threshold calculated for TX power = 60 Watts at distance of 1500 meters
  // double m_rx_threshold = 3.23778 * pow(10, -6);

  // Max tx_power
  double m_max_tx_power = 20; // Watts

  // Hop count threshold - send the DATA packet directly to the destination, if the hop count value exceeds the threshold
  int m_hop_count_threshold = 3;

  // Minimum possible reward
  double m_min_reward = 0.001;

  // Data transmission time interval
  // Time duration of the DATA_TX / RX states before transition to IDLE
  // I.e., how much data should be transmitted within a single RTS/CTS handshake
  Time m_data_timeout = Seconds(10);

  // Distance calculation error, meters
  // double m_dist_error = 0.1;
  double m_dist_error = 0.001;

  // Data packet size from upper layers
  int m_packet_size = 50;

  // Header id counter
  uint32_t m_header_id = 0;

  // Store number of nodes in a network
  uint16_t m_nnodes = 0;

  // Store a list of destination address, which are within maximum transmission range
  std::vector<AquaSimAddress> m_inrange_addresses;

  // Also, make sure that the given node has at least one neighbor in the maximum transmission range (
  //  (for the edge-case, when the max tx range is too short)
  bool m_isInRange = true;

  // Fix the number of intermediate nodes (for currernt experiments!!!)
  uint32_t m_nintermediate_nodes = 0;

  // Flag to indicate whether the initial distances are initialized
  bool m_distances_initialized = false;


};  // class AquaSimMacLibra

} // namespace ns3

#endif /* AQUA_SIM_MAC_LIBRA_H */
