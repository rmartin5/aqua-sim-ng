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

/*
* Vector Based Routing
*/
NS_OBJECT_ENSURE_REGISTERED(VBHeader);

VBHeader::VBHeader()
{
}

VBHeader::~VBHeader()
{
}

TypeId
VBHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::VBHeader")
    .SetParent<Header>()
    .AddConstructor<VBHeader>()
  ;
  return tid;
}

uint32_t
VBHeader::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_messType = i.ReadU8();
  m_pkNum = i.ReadU32();
  m_targetAddr = (AquaSimAddress) i.ReadU8();
  m_senderAddr = (AquaSimAddress) i.ReadU8();
  m_forwardAddr = (AquaSimAddress) i.ReadU8();
  m_dataType = i.ReadU8();
  m_originalSource.x = ( (double) i.ReadU16() ) / 1000.0;
  m_originalSource.y = ( (double) i.ReadU16() ) / 1000.0;
  m_originalSource.z = ( (double) i.ReadU16() ) / 1000.0;
  m_token = ((double) i.ReadU32()) / 1000.0;
  m_ts = ((double) i.ReadU32()) / 1000.0;
  m_range = ((double) i.ReadU32()) / 1000.0;

  //This is bloated.
  m_info.o.x = ( (double) i.ReadU16() ) / 1000.0;
  m_info.o.y = ( (double) i.ReadU16() ) / 1000.0;
  m_info.o.z = ( (double) i.ReadU16() ) / 1000.0;
  m_info.f.x = ( (double) i.ReadU16() ) / 1000.0;
  m_info.f.y = ( (double) i.ReadU16() ) / 1000.0;
  m_info.f.z = ( (double) i.ReadU16() ) / 1000.0;
  m_info.t.x = ( (double) i.ReadU16() ) / 1000.0;
  m_info.t.y = ( (double) i.ReadU16() ) / 1000.0;
  m_info.t.z = ( (double) i.ReadU16() ) / 1000.0;
  m_info.d.x = ( (double) i.ReadU16() ) / 1000.0;
  m_info.d.y = ( (double) i.ReadU16() ) / 1000.0;
  m_info.d.z = ( (double) i.ReadU16() ) / 1000.0;

  return GetSerializedSize();
}

uint32_t
VBHeader::GetSerializedSize(void) const
{
  //reserved bytes for header
  return (1+4+1+1+1+1+6+4+4+4 +24);
}

void
VBHeader::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8(m_messType);
  i.WriteU32(m_pkNum);
  i.WriteU8(m_targetAddr.GetAsInt());
  i.WriteU8(m_senderAddr.GetAsInt());
  i.WriteU8(m_forwardAddr.GetAsInt());
  i.WriteU8(m_dataType);

  //Messy...
  i.WriteU16 ((uint16_t)(m_originalSource.x*1000.0 +0.5));
  i.WriteU16 ((uint16_t)(m_originalSource.y*1000.0 +0.5));
  i.WriteU16 ((uint16_t)(m_originalSource.z*1000.0 +0.5));

  i.WriteU32((uint32_t)(m_token*1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_ts*1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_range*1000.0 + 0.5));

  //bloated.
  i.WriteU16 ((uint16_t)(m_info.o.x*1000.0 +0.5));
  i.WriteU16 ((uint16_t)(m_info.o.y*1000.0 +0.5));
  i.WriteU16 ((uint16_t)(m_info.o.z*1000.0 +0.5));
  i.WriteU16 ((uint16_t)(m_info.f.x*1000.0 +0.5));
  i.WriteU16 ((uint16_t)(m_info.f.y*1000.0 +0.5));
  i.WriteU16 ((uint16_t)(m_info.f.z*1000.0 +0.5));
  i.WriteU16 ((uint16_t)(m_info.t.x*1000.0 +0.5));
  i.WriteU16 ((uint16_t)(m_info.t.y*1000.0 +0.5));
  i.WriteU16 ((uint16_t)(m_info.t.z*1000.0 +0.5));
  i.WriteU16 ((uint16_t)(m_info.d.x*1000.0 +0.5));
  i.WriteU16 ((uint16_t)(m_info.d.y*1000.0 +0.5));
  i.WriteU16 ((uint16_t)(m_info.d.z*1000.0 +0.5));
}

void
VBHeader::Print(std::ostream &os) const
{
  //TODO messType print out needs to be a switch case using DEFINE names
  os << "Vector Based Routing Header is: messType=";
  switch(m_messType) {
    case INTEREST:          os << "INTEREST"; break;
    case AS_DATA:           os << "DATA"; break;
    case DATA_READY:        os << "DATA_READY";   break;
    case SOURCE_DISCOVERY:  os << "SOURCE_DISCOVERY"; break;
    case SOURCE_TIMEOUT:    os << "SOURCE_TIMEOUT"; break;
    case TARGET_DISCOVERY:  os << "TARGET_DISCOVERY";   break;
    case TARGET_REQUEST:    os << "TARGET_REQUEST"; break;
    case SOURCE_DENY:       os << "SOURCE_DENY"; break;
    case V_SHIFT:           os << "V_SHIFT";   break;
    case FLOODING:          os << "FLOODING"; break;
    case DATA_TERMINATION:  os << "DATA_TERMINATION"; break;
    case BACKPRESSURE:      os << "BACKPRESSURE";   break;
    case BACKFLOODING:      os << "BACKFLOODING";   break;
    case EXPENSION:         os << "EXPENSION"; break;
    case V_SHIFT_DATA:      os << "V_SHIFT_DATA"; break;
    case EXPENSION_DATA:    os << "EXPENSION_DATA";   break;
  }

  os << " pkNum=" << m_pkNum << " targetAddr=" << m_targetAddr << " senderAddr=" <<
   m_senderAddr << " forwardAddr=" << m_forwardAddr << " dataType=" <<
   m_dataType << " originalSource=" << m_originalSource.x << "," <<
   m_originalSource.y << "," << m_originalSource.z << " token=" << m_token <<
   " ts=" << m_ts << " range=" << m_range << "\n";

  os << "ExtraInfo= StartPoint(" << m_info.o.x << "," << m_info.o.y << "," <<
    m_info.o.z << ") ForwardPos(" << m_info.f.x << "," << m_info.f.y << "," <<
    m_info.f.z << ") EndPoint(" << m_info.t.x << "," << m_info.t.y << "," <<
    m_info.t.z << ") RecvToForwarder(" << m_info.d.x << "," << m_info.d.y << "," <<
    m_info.d.z << ")\n";
}

TypeId
VBHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}


void
VBHeader::SetMessType(uint8_t messType)
{
  m_messType = messType;
}
void
VBHeader::SetPkNum(uint32_t pkNum)
{
  m_pkNum = pkNum;
}
void
VBHeader::SetTargetAddr(AquaSimAddress targetAddr)
{
  m_targetAddr = targetAddr;
}
void
VBHeader::SetSenderAddr(AquaSimAddress senderAddr)
{
  m_senderAddr = senderAddr;
}
void
VBHeader::SetForwardAddr(AquaSimAddress forwardAddr)
{
  m_forwardAddr = forwardAddr;
}
void
VBHeader::SetDataType(uint8_t dataType)
{
  m_dataType = dataType;
}
void
VBHeader::SetOriginalSource(Vector3D originalSource)
{
  m_originalSource = originalSource;
}
void
VBHeader::SetToken(double token)
{
  m_token = token;
}
void
VBHeader::SetTs(double ts)
{
  m_ts = ts;
}
void
VBHeader::SetRange(double range)
{
  m_range = range;
}
void
VBHeader::SetExtraInfo(uw_extra_info info)
{
  m_info = info;
}

uint8_t
VBHeader::GetMessType()
{
  return m_messType;
}
uint32_t
VBHeader::GetPkNum()
{
  return m_pkNum;
}
AquaSimAddress
VBHeader::GetTargetAddr()
{
  return m_targetAddr;
}
AquaSimAddress
VBHeader::GetSenderAddr()
{
  return m_senderAddr;
}
AquaSimAddress
VBHeader::GetForwardAddr()
{
  return m_forwardAddr;
}
uint8_t
VBHeader::GetDataType()
{
  return m_dataType;
}
Vector3D
VBHeader::GetOriginalSource()
{
  return m_originalSource;
}
double
VBHeader::GetToken()
{
  return m_token;
}
double
VBHeader::GetTs()
{
  return m_ts;
}
double
VBHeader::GetRange()
{
  return m_range;
}
uw_extra_info
VBHeader::GetExtraInfo()
{
  return m_info;
}
