/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 University of Connecticut
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
 * Author: Robert Martin <robert.martin@engr.uconn.edu>
 */

#ifndef AQUA_SIM_HEADER_MAC_H
#define AQUA_SIM_HEADER_MAC_H

//#include <string>
#include <iostream>

#include "ns3/header.h"
//#include "ns3/nstime.h"
#include "ns3/vector.h"

#include "aqua-sim-address.h"
//#include "aqua-sim-routing-buffer.h"
#include "aqua-sim-datastructure.h"

namespace ns3 {

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Generic MAC header with demux
 */
class MacHeader : public Header
{
public:
  enum AquaSimMacDemuxPktType{
    UWPTYPE_OTHER,
    UWPTYPE_LOC,
    UWPTYPE_SYNC,
    UWPTYPE_SYNC_BEACON,
    UWPTYPE_NDN,
    UWPTYPE_MAC_LIBRA}; // Add mac_libra mac type

  MacHeader();
  static TypeId GetTypeId(void);

  void SetSA(AquaSimAddress sa);
  void SetDA(AquaSimAddress da);
  void SetDemuxPType(uint8_t demuxPType);
  AquaSimAddress GetSA();
  AquaSimAddress GetDA();
  uint8_t GetDemuxPType();

  //inherited methods
  virtual uint32_t GetSerializedSize(void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId(void) const;

private:
  AquaSimAddress m_sa;
  AquaSimAddress m_da;
  uint8_t m_demuxPType;
};  // MacHeader

/**
 * \brief TMAC header
 *     (RMAC uses same header, due to similarity)
 */
class TMacHeader : public Header
{
public:
  enum PacketType {
    PT_OTHER,
    PT_DATA,
    PT_RTS,
    PT_CTS,
    PT_ND,
    PT_SACKND,
    PT_ACKDATA,
    PT_SYN
  } packet_type;

  TMacHeader();
  virtual ~TMacHeader();
  static TypeId GetTypeId(void);

  //Getters
  uint8_t GetPtype();
  uint32_t GetPktNum();
  uint32_t GetDataNum();
  uint8_t GetBlockNum();
  AquaSimAddress GetSenderAddr();
  AquaSimAddress GetRecvAddr();
  double GetST();
  double GetTS();
  double GetDuration();
  double GetInterval();
  double GetArrivalTime();

  //Setters
  void SetPtype(uint8_t ptype);
  void SetPktNum(uint32_t pktNum);
  void SetDataNum(uint32_t dataNum);
  void SetBlockNum(uint8_t blockNum);
  void SetSenderAddr(AquaSimAddress senderAddr);
  void SetRecvAddr(AquaSimAddress receiverAddr);
  void SetST(double st);
  void SetTS(double ts);
  void SetDuration(double durable);
  void SetInterval(double interval);
  void SetArrivalTime(double arrivalTime);

  //inherited methods
  virtual uint32_t GetSerializedSize(void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId(void) const;

private:
  uint8_t m_ptype;     //packet type
  uint32_t m_pktNum;    // sequence number
  uint32_t m_dataNum;
  uint8_t m_blockNum; // the block num, in real world, one bit is enough
  AquaSimAddress m_senderAddr;  //original sender's address
  AquaSimAddress m_recvAddr;
  double m_st;           // Timestamp when pkt is generated.
  double m_ts;
  double m_duration;
  double m_interval;
  double m_arrivalTime;
};  // class TMacHeader


 /**
  * \brief Aloha header
  */
class AlohaHeader : public Header
{
public:
  enum PacketType {
    DATA,
    ACK
  } packet_type;

  AlohaHeader();
  virtual ~AlohaHeader();
  static TypeId GetTypeId(void);

  static int GetSize();

  void SetSA(AquaSimAddress sa);
  void SetDA(AquaSimAddress da);
  void SetPType(uint8_t pType);
  AquaSimAddress GetSA();
  AquaSimAddress GetDA();
  uint8_t GetPType();	//Remove Set/Get pType and go directly to public variable??

  //inherited methods
  virtual uint32_t GetSerializedSize(void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId(void) const;
private:
  AquaSimAddress SA;
  AquaSimAddress DA;
  uint8_t m_pType;
};  // class AlohaHeader

 /**
  * \brief FAMA header
  */
class FamaHeader : public Header
{
public:
  enum PacketType {
    RTS,	//the previous forwarder thinks this is DATA-ACK
    CTS,
    FAMA_DATA,
    ND		//neighbor discovery. need know neighbors, so it can be used as next hop.
  } packet_type;

  FamaHeader();
  virtual ~FamaHeader();
  static TypeId GetTypeId(void);

  static int size();

  void SetSA(AquaSimAddress sa);
  void SetDA(AquaSimAddress da);
  void SetPType(uint8_t pType);
  AquaSimAddress GetSA();
  AquaSimAddress GetDA();
  uint8_t GetPType();	//Remove Set/Get pType and go directly to public variable??

  //inherited methods
  virtual uint32_t GetSerializedSize(void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId(void) const;
private:
  AquaSimAddress SA;
  AquaSimAddress DA;
  uint8_t m_pType;
};  // class FamaHeader


 /**
  * \brief COPE-MAC header
  */
class CopeHeader : public Header
{
public:
  enum PacketType {
    COPE_ND,
    COPE_ND_REPLY,
    MULTI_REV,
    MULTI_REV_ACK,
    MULTI_DATA_ACK
  } packet_type;

  CopeHeader();
  virtual ~CopeHeader();
  static TypeId GetTypeId(void);

  uint32_t size();

  void SetSA(AquaSimAddress sa);
  void SetDA(AquaSimAddress da);
  void SetPType(uint8_t pType);
  AquaSimAddress GetSA();
  AquaSimAddress GetDA();
  uint8_t GetPType();	//Remove Set/Get pType and go directly to public variable??

  //inherited methods
  virtual uint32_t GetSerializedSize(void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId(void) const;
private:
  int m_size;
  AquaSimAddress SA;
  AquaSimAddress DA;
  uint8_t m_pType;
};  // class CopeHeader


/**
 * \brief Slotted FAMA header
 */
class SFamaHeader : public Header
{
public:
  enum PacketType {
  SFAMA_RTS,
  	SFAMA_CTS,
  	SFAMA_DATA,
  	SFAMA_ACK
  } packet_type;

  SFamaHeader();
  virtual ~SFamaHeader();
  static TypeId GetTypeId(void);

  static int GetSize(enum PacketType pType);

  void SetPType(uint8_t pType);
  void SetSlotNum(uint16_t slotNum);
  uint8_t GetPType();	//Remove Set/Get pType and go directly to public variable??
  uint16_t GetSlotNum();

  //inherited methods
  virtual uint32_t GetSerializedSize(void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId(void) const;
private:
  //AquaSimAddress SA;
  //AquaSimAddress DA;
  uint8_t m_pType;
  uint16_t	m_slotNum;  //the number of slots required for transmitting the DATA packet

};  // class SFamaHeader

/**
 * \brief UWAN SYNC header
 */
class UwanSyncHeader : public Header
{
public:
  UwanSyncHeader();
  virtual ~UwanSyncHeader();
  static TypeId GetTypeId(void);

  double GetCyclePeriod();
  static int GetSize();
  void SetCyclePeriod(double cyclePeriod);

  //inherited methods
  virtual uint32_t GetSerializedSize(void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId(void) const;
private:
  double m_cyclePeriod;
};  // class UwanSyncHeader


/**
 * \brief Localization header
 */
class LocalizationHeader : public Header
{
public:
  LocalizationHeader();
  virtual ~LocalizationHeader();
  static TypeId GetTypeId(void);

  Vector GetNodePosition();
  double GetConfidence();
  void SetNodePosition(Vector nodePosition);
  void SetConfidence(double confidence);
  //components for AoA???

  //inherited methods
  virtual uint32_t GetSerializedSize(void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId(void) const;

private:
  Vector m_nodePosition;
  double m_confidence;
};  // class LocalizationHeader

/**
 * \brief JMAC header
 */
class JammingMacHeader : public Header
{
public:
	JammingMacHeader();
  virtual ~JammingMacHeader();
  static TypeId GetTypeId(void);

  void SetPType(uint8_t m_ptype);
  uint8_t GetPType();

  // Set "schedule" - delay before a packet is transmitted by given node_id
  void SetSchedule(uint8_t node_id, uint16_t delay_ms);
  uint16_t GetSchedule(uint8_t node_id);

  // Set/get node_id for cc-request
  void SetNodeId(uint8_t node_id);
  uint8_t GetNodeId();

  // Set node-bit in the schedule
  void SetNodeBit(uint8_t node_id);
  uint8_t GetNodeBit(uint8_t node_id) const;
  // Return an amount of nodes in the schedule (node_list)
  uint8_t GetNodesAmount() const;

  // Set/get coordinates from request/reply
  void SetCoordinates(Vector coords);
  Vector GetCoordinates();

  //inherited methods
  virtual uint32_t GetSerializedSize(void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId(void) const;

private:
  // Packet type: 0 - DATA, 1 - CC-request, 2 - CS-reply
  uint8_t m_ptype;
  uint8_t m_node_id;
  // Node list for CS-header
  uint64_t m_node_list = 0;
  // Schedule (delay) in milliseconds for given nodes
  std::map<uint8_t, uint16_t> m_delays_ms;
  // store coordinates for cc-request
  uint32_t m_x_coord;
  uint32_t m_y_coord;
  uint32_t m_z_coord;
};  // class JammingMacHeader

/**
 * \brief TRUMAC header
 */
class TrumacHeader : public Header
{
public:
	TrumacHeader();
  virtual ~TrumacHeader();
  static TypeId GetTypeId(void);

  void SetPType(uint8_t m_ptype);
  uint8_t GetPType();

  void SetNextNodeId(uint8_t node_id);
  uint8_t GetNextNodeId();

  //inherited methods
  virtual uint32_t GetSerializedSize(void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId(void) const;

private:
  // Packet type: 0 - DATA, 1 - Token
  uint8_t m_ptype;
  uint8_t m_next_sender_id;
};  // class TrumacHeader

/**
 * \brief mac-libra header
 */
class MacLibraHeader : public Header
{
public:
	MacLibraHeader();
  virtual ~MacLibraHeader();
  static TypeId GetTypeId(void);

  void SetPType(uint8_t m_ptype);
  int GetPType();
  void SetId(uint32_t m_header_id);
  void SetHopCount(uint8_t m_hop_count);
  void IncrementHopCount();

  void SetSrcAddr (AquaSimAddress src_addr);
  void SetDstAddr (AquaSimAddress dst_addr);
  void SetSenderAddr (AquaSimAddress sender_addr);

  AquaSimAddress GetSrcAddr ();
  AquaSimAddress GetDstAddr ();
  AquaSimAddress GetSenderAddr ();

  void SetReward (double reward_value);
  double GetReward ();

  int GetHopCount ();

  void SetRxPower (double rx_power);
  void SetTxPower (double tx_power);
  void SetDirectDistance (double direct_distance);
  void SetNextHopDistance (double next_hop_distance);
  void SetOptimalDistance (double m_optimal_distance);
  void SetMaxHopsNumber (uint8_t m_max_hops_number);

  double GetRxPower ();
  double GetTxPower ();
  double GetDirectDistance ();
  double GetNextHopDistance ();
  double GetOptimalDistance ();
  uint8_t GetMaxHopsNumber ();

  //inherited methods
  virtual uint32_t GetSerializedSize(void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId(void) const;

private:
  // Packet type: 0 - data_packet, 1 - RREQ, 2 - RREP, 3 - ACK, 4 - INIT, 5 - RTS, 6 - CTS, 7 - DIRECT_REWARD
  uint8_t m_ptype;
  // Unique header ID
  uint32_t m_header_id;
  // Hop count
  uint8_t m_hop_count = 0;
  // Source address
  AquaSimAddress m_src_addr;
  // Destination address
  AquaSimAddress m_dst_addr;
  // Sender address to send back the reward to
  AquaSimAddress m_sender_addr;
  // Reward value
  uint32_t m_reward = 0;
  // Message ID the ACK is replying to
  uint32_t m_ack_message_id = 0;

  // Tx power for sending frame
  uint64_t m_tx_power;

  // Rx power for receiving frame
  uint64_t m_rx_power = 0;

  // Direct distance from source to destination
  uint32_t m_direct_distance = 0;

  // Next hop distance, needed for the propagation model
  uint32_t m_next_hop_distance = 0;

  // Optimal distance, calculated by the source
  uint32_t m_optimal_distance = 0;

  // Max number of hops, calculated by the source
  uint8_t m_max_hops_number = 0;

  // Store a multipler to convert from double to uint32/64 and back
  double m_multiplier_64 = 10000000000000;
  double m_multiplier_32 = 10000;

};  // class MacLibraHeader

} // namespace ns3

#endif /* AQUA_SIM_HEADER_MAC_H */
