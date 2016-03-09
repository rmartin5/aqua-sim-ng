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

#include "aqua-sim-header-routing.h"

#include "ns3/log.h"
#include "ns3/buffer.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("DRoutingHeader");
NS_OBJECT_ENSURE_REGISTERED(DRoutingHeader);

DRoutingHeader::DRoutingHeader()
{
}

DRoutingHeader::~DRoutingHeader()
{
}

TypeId
DRoutingHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::DRoutingHeader")
    .SetParent<Header>()
    .AddConstructor<DRoutingHeader>()
  ;
  return tid;
}

uint32_t
DRoutingHeader::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_pktSrc = (AquaSimAddress) i.ReadU8();
  m_pktLen = i.ReadU16();
  m_pktSeqNum = i.ReadU8();
  m_entryNum = i.ReadU32();

  return GetSerializedSize();
}

uint32_t
DRoutingHeader::GetSerializedSize(void) const
{
  //reserved bytes for header
  return (1+3+1+4);
}

void
DRoutingHeader::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8(m_pktSrc.GetAsInt());
  i.WriteU16(m_pktLen);
  i.WriteU8(m_pktSeqNum);
  i.WriteU32(m_entryNum);
}

void
DRoutingHeader::Print(std::ostream &os) const
{
  os << "Dynamic Routing Header is: PktSrc=" << m_pktSrc << " PktLen=" <<
    m_pktLen << " PktSeqNum=" << m_pktSeqNum << " EntryNum=" << m_entryNum << "\n";
}

TypeId
DRoutingHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

AquaSimAddress
DRoutingHeader::GetPktSrc()
{
  return m_pktSrc;
}
uint16_t
DRoutingHeader::GetPktLen()
{
  return m_pktLen;
}
uint8_t
DRoutingHeader::GetPktSeqNum()
{
  return m_pktSeqNum;
}
uint32_t
DRoutingHeader::GetEntryNum()
{
  return m_entryNum;
}

void
DRoutingHeader::SetPktSrc(AquaSimAddress pktSrc)
{
  m_pktSrc = pktSrc;
}
void
DRoutingHeader::SetPktLen(uint16_t pktLen)
{
  m_pktLen = pktLen;
}
void
DRoutingHeader::SetPktSeqNum(uint8_t pktSeqNum)
{
  m_pktSeqNum = pktSeqNum;
}
void
DRoutingHeader::SetEntryNum(uint32_t entryNum)
{
  m_entryNum = entryNum;
}
