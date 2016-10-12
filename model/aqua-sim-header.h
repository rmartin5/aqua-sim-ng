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

#ifndef AQUA_SIM_HEADER_H
#define AQUA_SIM_HEADER_H

#include <string>
#include <iostream>

#include "ns3/address.h"
#include "ns3/header.h"
#include "ns3/nstime.h"

#include "aqua-sim-address.h"

namespace ns3 {

class Packet;

struct addr_t {
  AquaSimAddress addr;
  int32_t port;   /* Currently not used */
};

 /**
  * \ingroup aqua-sim-ng
  *
  * \brief Generic Aqua-Sim header
  */
class AquaSimHeader : public Header
{
public:
  /// packet direction within device's stack
  enum dir_t { DOWN = 0, NONE = 1, UP = 2 };

  AquaSimHeader();
  virtual ~AquaSimHeader();
  static TypeId GetTypeId(void);

  //Protocol specific access to the header

  /***TODO This is clunky and needs to be revamped to match real world headers***/
  //Could probably combine direction and packet status using std::bitset to utilize uint8_t
      //icmpv6-header shows a helpful example of header flag handling too

  //Getters
  Time GetTxTime();  // tx time for this packet in sec
  uint32_t GetSize();	// simulated packet size
  uint8_t GetDirection();/* setting all direction of pkts to be none as default */
  //uint8_t GetAddrType();	//type of next hop addr... not in use currently
  AquaSimAddress GetNextHop();	// next hop for this packet
  uint16_t GetNumForwards();	// how many times this pkt was forwarded
  AquaSimAddress GetSAddr();
  AquaSimAddress GetDAddr();
  int32_t GetSPort();
  int32_t GetDPort();
  bool GetErrorFlag();
  uint16_t GetUId();
  Time GetTimeStamp();

  //Setters
  void SetTxTime(Time time);
  void SetSize(uint16_t size);
  void SetDirection(uint8_t direction);
  void SetNextHop(AquaSimAddress nextHop);
  void SetNumForwards(uint16_t numForwards);
  void SetSAddr(AquaSimAddress sAddr);
  void SetDAddr(AquaSimAddress dAddr);
  void SetSPort(int32_t sPort);	//TODO should be removed...
  void SetDPort(int32_t dPort);
  void SetErrorFlag(bool error);
  void SetUId(uint16_t uId);
  void SetTimeStamp(Time timestamp);

  //inherited by Header class
  virtual TypeId GetInstanceTypeId(void) const;
  virtual void Print(std::ostream &os) const;
  virtual void Serialize(Buffer::Iterator start) const;
  virtual uint32_t Deserialize(Buffer::Iterator start);
  virtual uint32_t GetSerializedSize(void) const;

private:
  //uint32_t m_data;
  Time m_txTime;
  uint8_t m_direction;  // direction: 0=down, 1=none, 2=up
  //uint8_t m_addrType;
  AquaSimAddress m_nextHop;
  uint16_t m_numForwards;
  addr_t m_src;
  addr_t m_dst;
  uint8_t m_errorFlag;
  uint16_t m_uId;
  uint16_t m_size; //figmented size of packet...
  Time m_timestamp;

}; //class AquaSimHeader

/**
 * \brief Packet stamp used by lower layers
 */
class AquaSimPacketStamp : public Header
{
public:
  enum PacketStatus { RECEPTION = 0, COLLISION = 1, INVALID = 2};

  AquaSimPacketStamp();
  static TypeId GetTypeId(void);

  double GetTxRange();
  double GetPt();
  double GetPr();
  double GetFreq();
  double GetNoise();
  //std::string GetModName();
  uint8_t GetPacketStatus();	// default is INVALID


  void SetTxRange(double txRange);
  void SetPt(double pt);
  void SetPr(double pr);
  void SetFreq(double freq);
  void SetNoise(double noise);
  //void SetModName(std::string modName);
  void SetPacketStatus(uint8_t status);

  bool CheckConflict();  //check if parameters conflict

  //inherited by Header class
  virtual TypeId GetInstanceTypeId(void) const;
  virtual void Print(std::ostream &os) const;
  virtual void Serialize(Buffer::Iterator start) const;
  virtual uint32_t Deserialize(Buffer::Iterator start);
  virtual uint32_t GetSerializedSize(void) const;

private:
  /**
  * only one between m_ptLevel and m_txRange can should be set
  * run CheckConflict() to avoid conflict
  */

  double m_pt;		//transmission power, set according to Pt_level_
  double m_pr;		//rx power, set by channel/propagation module
  double m_txRange;	//transmission range
  double m_freq;	//central frequency
  double m_noise;	//background noise at the receiver side
  //std::string m_modName;
  uint8_t m_status;  // 0=reception, 1=collision, 2=invalid

}; //class AquaSimPacketStamp

}  // namespace ns3

#endif /* AQUA_SIM_HEADER_H */
