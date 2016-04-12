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

#include "aqua-sim-header.h"
#include "aqua-sim-address.h"
#include "ns3/header.h"
#include "ns3/buffer.h"
#include "ns3/log.h"
#include "ns3/address-utils.h"

#include <iostream>
#include <bitset>


using namespace ns3;

#include "ns3/address.h"

//Aqua Sim Header

NS_LOG_COMPONENT_DEFINE("AquaSimHeader");
NS_OBJECT_ENSURE_REGISTERED(AquaSimHeader);

AquaSimHeader::AquaSimHeader(void) :
    m_txTime(0), m_direction(DOWN),
    m_numForwards(0), m_errorFlag(0), m_uId(-1),
    m_timestamp(0)
{
  m_nextHop = AquaSimAddress(-1);
  m_src.addr = AquaSimAddress(-1);
  m_dst.addr = AquaSimAddress(-1);
  //m_src.port(0);
  //m_dst.port(0);

  NS_LOG_FUNCTION(this);
}

AquaSimHeader::~AquaSimHeader(void)
{
  NS_LOG_FUNCTION(this);
}

TypeId
AquaSimHeader::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimHeader")
    .SetParent<Header>()
    .AddConstructor<AquaSimHeader>()
  ;
  return tid;
}

TypeId
AquaSimHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

uint32_t
AquaSimHeader::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_txTime = Seconds ( ( (double) i.ReadU32 ()) / 1000.0 );
  m_size = i.ReadU16();
  m_direction = i.ReadU8();
  m_numForwards = i.ReadU16();
  m_nextHop = (AquaSimAddress) i.ReadU8();
  m_src.addr = (AquaSimAddress) i.ReadU8();
  m_dst.addr = (AquaSimAddress) i.ReadU8();
  m_errorFlag = i.ReadU8();	//wasted space due to only needing 1 bit
  m_uId = i.ReadU16();
  m_timestamp = Seconds ( ( (double) i.ReadU32 ()) / 1000.0 );

  return GetSerializedSize();
}

uint32_t
AquaSimHeader::GetSerializedSize(void) const
{
  /*Currently can be of arbitrary size
  (say 2 bytes for static header fields + 4 bytes for data)
  this can/SHOULD be changed dependent on protocol constrains.
  example can be seen @ main-packet-header.cc*/

  //reserved bytes for header
  return (4+2+1+2+1+1+1+1+2+4);
}

void
AquaSimHeader::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU32((uint32_t)(m_txTime.GetSeconds() * 1000.0));
  i.WriteU16(m_size);
  i.WriteU8(m_direction);
  i.WriteU16(m_numForwards);
  i.WriteU8(m_nextHop.GetAsInt());
  i.WriteU8(m_src.addr.GetAsInt());
  i.WriteU8(m_dst.addr.GetAsInt());
  i.WriteU8(m_errorFlag);
  i.WriteU16(m_uId);
  //src/dst port
  i.WriteU32((uint32_t)(m_timestamp.GetSeconds()*1000.0 + 0.5));
}

void
AquaSimHeader::Print(std::ostream &os) const
{
  os << "Packet header is  ";
  os << "TxTime=" << m_txTime << " Size=" << m_size << " Direction=";
  switch (m_direction){
    case DOWN:  os << "DOWN"; break;
    case NONE:  os << "NONE"; break;
    case UP:    os << "UP";   break;
  }
  os << " NumForwards=" << m_numForwards << " Error=";
  if (m_errorFlag == 0) {os << "False";}
  else  {os << "True"; }

  os << " UniqueID=" << m_uId;
  os << " Timestamp=" << m_timestamp;
  os << " SenderAddr=" << m_src.addr << " DestAddr=" << m_dst.addr << " NextHop=" << m_nextHop << "\n";
}

Time
AquaSimHeader::GetTxTime(void)
{
  return m_txTime;
}

uint32_t
AquaSimHeader::GetSize(void)
{
  return m_size;
  //return GetSerializedSize();
}

uint8_t
AquaSimHeader::GetDirection(void)
{
  return m_direction;
}

AquaSimAddress
AquaSimHeader::GetNextHop(void)
{
  return m_nextHop;
}

uint16_t
AquaSimHeader::GetNumForwards(void)
{
  return m_numForwards;
}

AquaSimAddress
AquaSimHeader::GetSAddr(void)
{
  return m_src.addr;
}

AquaSimAddress
AquaSimHeader::GetDAddr(void)
{
  return m_dst.addr;
}

int32_t
AquaSimHeader::GetSPort()
{
  return (m_src.port);
}

int32_t
AquaSimHeader::GetDPort()
{
  return (m_dst.port);
}

bool
AquaSimHeader::GetErrorFlag(void)
{
  return m_errorFlag;
}

uint16_t
AquaSimHeader::GetUId(void)
{
  return m_uId;
}

Time
AquaSimHeader::GetTimeStamp()
{
  return m_timestamp;
}

void
AquaSimHeader::SetTxTime(Time time)
{
  m_txTime = time;
}

void
AquaSimHeader::SetSize(uint16_t size)
{
  m_size = size;
}

void
AquaSimHeader::SetDirection(uint8_t direction)
{
  m_direction = direction;
}

void
AquaSimHeader::SetNextHop(AquaSimAddress nextHop)
{
  m_nextHop = nextHop;
}

void
AquaSimHeader::SetNumForwards(uint16_t numForwards)
{
  m_numForwards = numForwards;
}

void
AquaSimHeader::SetSAddr(AquaSimAddress sAddr)
{
  m_src.addr = sAddr;
}

void
AquaSimHeader::SetDAddr(AquaSimAddress dAddr)
{
  m_dst.addr = dAddr;
}

void
AquaSimHeader::SetSPort(int32_t sPort)
{
  m_src.port = sPort;
}

void
AquaSimHeader::SetDPort(int32_t dPort)
{
  m_dst.port = dPort;
}

void
AquaSimHeader::SetErrorFlag(bool error)
{
  m_errorFlag = error;
}

void
AquaSimHeader::SetUId(uint16_t uId)
{
  NS_LOG_FUNCTION(this << "this is not unique and must be removed/implemented");
  m_uId = uId;
}

void
AquaSimHeader::SetTimeStamp(Time timestamp)
{
  m_timestamp = timestamp;
}


//PacketStampHeader

AquaSimPacketStamp::AquaSimPacketStamp() :
  m_pt(-1), m_pr(-1), m_txRange(-1),
  m_freq(-1), m_noise(0), m_status(INVALID)
{
}

TypeId
AquaSimPacketStamp::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimPacketStamp")
    .SetParent<Header>()
    .AddConstructor<AquaSimPacketStamp>()
  ;
  return tid;
}

TypeId
AquaSimPacketStamp::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

uint32_t
AquaSimPacketStamp::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_pt = (double) i.ReadU32() / 10000.0;
  m_pr = (double) i.ReadU32() / 10000.0;
  m_txRange = (double) i.ReadU32() / 1000.0;
  m_freq = (double) i.ReadU32() / 1000.0;
  m_noise = (double) i.ReadU32() / 1000.0;
  m_status = i.ReadU8();

  return GetSerializedSize();
}

uint32_t
AquaSimPacketStamp::GetSerializedSize(void) const
{
  //reserved bytes for header
  return (21);
}

void
AquaSimPacketStamp::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU32((uint32_t) (m_pt * 10000.0));
  i.WriteU32((uint32_t) (m_pr * 10000.0));
  i.WriteU32((uint32_t) (m_txRange * 1000.0));
  i.WriteU32((uint32_t) (m_freq * 1000.0));
  i.WriteU32((uint32_t) (m_noise * 1000.0));
  i.WriteU8(m_status);
}

void
AquaSimPacketStamp::Print(std::ostream &os) const
{
  os << "PacketStamp: Pt(" << m_pt << ") Pr(" << m_pr << ") TxRange(" <<
    m_txRange << ") Freq(" << m_freq << ") Noise(" << m_noise << ") PacketStatus(";
  switch (m_status) {
    case RECEPTION: os << "RECEPTION"; break;
    case COLLISION: os << "COLLISION"; break;
    case INVALID:   os << "INVALID";   break;
  }
  os << ")\n";
}

double
AquaSimPacketStamp::GetTxRange()
{
  return m_txRange;
}
double
AquaSimPacketStamp::GetPt()
{
  return m_pt;
}
double
AquaSimPacketStamp::GetPr()
{
  return m_pr;
}
double
AquaSimPacketStamp::GetFreq()
{
  return m_freq;
}
double
AquaSimPacketStamp::GetNoise()
{
  return m_noise;
}
uint8_t
AquaSimPacketStamp::GetPacketStatus()
{
  return m_status;
}

void
AquaSimPacketStamp::SetTxRange(double txRange)
{
  m_txRange = txRange;
}
void
AquaSimPacketStamp::SetPt(double pt)
{
  m_pt = pt;
}
void
AquaSimPacketStamp::SetPr(double pr)
{
  m_pr = pr;
}
void
AquaSimPacketStamp::SetFreq(double freq)
{
  m_freq = freq;
}
void
AquaSimPacketStamp::SetNoise(double noise)
{
  m_noise = noise;
}
void
AquaSimPacketStamp::SetPacketStatus(uint8_t status)
{
  m_status = status;
}

bool
AquaSimPacketStamp::CheckConflict()
{
  return (m_txRange > 0 && m_pt < 0) || (m_txRange < 0 && m_pt > 0);
}
