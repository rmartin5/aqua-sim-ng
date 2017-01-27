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
    UWPTYPE_NDN };

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

} // namespace ns3

#endif /* AQUA_SIM_HEADER_MAC_H */
