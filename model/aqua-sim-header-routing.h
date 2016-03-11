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

#ifndef AQUA_SIM_HEADER_ROUTING_H
#define AQUA_SIM_HEADER_ROUTING_H

//#include <string>
#include <iostream>

#include "ns3/header.h"
//#include "ns3/nstime.h"
#include "ns3/vector.h"

#include "aqua-sim-address.h"
#include "aqua-sim-routing-buffer.h"
#include "aqua-sim-datastructure.h"

namespace ns3 {

/*
 *  Dynamic routing
 */
class DRoutingHeader : public Header
{
public:
  DRoutingHeader();
  virtual ~DRoutingHeader();
  static TypeId GetTypeId();

  AquaSimAddress GetPktSrc();
  uint16_t GetPktLen();
  uint8_t GetPktSeqNum();
  uint32_t GetEntryNum();
  void SetPktSrc(AquaSimAddress pktSrc);
  void SetPktLen(uint16_t pktLen);
  void SetPktSeqNum(uint8_t pktSeqNum);
  void SetEntryNum(uint32_t entryNum);

  //inherited methods
  virtual uint32_t GetSerializedSize(void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId(void) const;

private:
  AquaSimAddress  m_pktSrc; // Node which originated this packet
  uint16_t m_pktLen; // Packet length (in bytes)
  uint8_t  m_pktSeqNum; // Packet sequence number
  uint32_t m_entryNum; // Packet length (in bytes)
  //add by jun-------routing table
}; // class DRoutingHeader

/*
 *  Vector Based Routing
 */
class VBHeader : public Header
{
public:
  VBHeader();
  virtual ~VBHeader();
  static TypeId GetTypeId();

  //Setters
  void SetMessType(uint8_t messType);
  void SetPkNum(uint32_t pkNum);
  void SetTargetAddr(AquaSimAddress targetAddr);
  void SetSenderAddr(AquaSimAddress senderAddr);
  void SetForwardAddr(AquaSimAddress forwardAddr);
  void SetDataType(uint8_t dataType);
  void SetOriginalSource(Vector3D originalSource);
  void SetToken(double token);
  void SetTs(double ts);
  void SetRange(double range);
  void SetExtraInfo(uw_extra_info info);

  //Getters
  uint8_t GetMessType();
  uint32_t GetPkNum();
  AquaSimAddress GetTargetAddr();
  AquaSimAddress GetSenderAddr();
  AquaSimAddress GetForwardAddr();
  uint8_t GetDataType();
  Vector3D GetOriginalSource();
  double GetToken();
  double GetTs();
  double GetRange();
  uw_extra_info GetExtraInfo();

  //inherited methods
  virtual uint32_t GetSerializedSize(void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId(void) const;

private:
  uint8_t m_messType;  //message type
  uint32_t m_pkNum;   //packet sequence num
  AquaSimAddress m_targetAddr; // the target addr of this data
  AquaSimAddress m_senderAddr;  //original sender addr
  AquaSimAddress m_forwardAddr;// the forwarder addr
  // AquaSimAddress next_nodes[MAX_NEIGHBORS];
  //int num_next;
  uint8_t m_dataType; //what is this for?

  Vector3D m_originalSource;
  struct uw_extra_info m_info;

  double m_token;
  double m_ts;      // Timestamp when pkt is generated.
  double m_range;    // target range
  //int report_rate;               // For simple diffusion only.
  //int attr[MAX_ATTRIBUTE];
};  // class VBHeader

}  // namespace ns3

#endif /* AQUA_SIM_HEADER_ROUTING_H */
