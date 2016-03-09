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

#include "aqua-sim-address.h"

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

}  // namespace ns3

#endif /* AQUA_SIM_HEADER_ROUTING_H */
