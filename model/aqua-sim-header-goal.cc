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

#include "aqua-sim-header-goal.h"

#include "ns3/log.h"
#include "ns3/address-utils.h"
#include "ns3/buffer.h"

using namespace ns3;


NS_LOG_COMPONENT_DEFINE("AquaSimGoalHeader");
NS_OBJECT_ENSURE_REGISTERED(AquaSimGoalReqHeader);

AquaSimGoalReqHeader::AquaSimGoalReqHeader()
{
}
AquaSimGoalReqHeader::~AquaSimGoalReqHeader()
{
}

TypeId
AquaSimGoalReqHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::AquaSimGoalReqHeader")
    .SetParent<Header>()
    .AddConstructor<AquaSimGoalReqHeader>()
  ;
  return tid;
}

uint32_t
AquaSimGoalReqHeader::size(BackoffType type)
{
  int hdrSize = 10*2 + 4*8;

  switch( type ) {
    case VBF:
      hdrSize += 96;
      break;
    case HH_VBF:
      hdrSize += 144;
      break;
    default:
      ;
  }

  return hdrSize/8+1;
}
void
AquaSimGoalReqHeader::SetSA(AquaSimAddress sa)
{
  SA = sa;
}
void
AquaSimGoalReqHeader::SetRA(AquaSimAddress ra)
{
  RA = ra;
}
void
AquaSimGoalReqHeader::SetDA(AquaSimAddress da)
{
  DA = da;
}
void
AquaSimGoalReqHeader::SetSendTime(Time sendtime)
{
  m_SendTime = sendtime;
}
void
AquaSimGoalReqHeader::SetTxTime(Time txtime)
{
  m_TxTime = txtime;
}
void
AquaSimGoalReqHeader::SetReqID(uint8_t reqid)
{
  m_ReqID = reqid;
}
void
AquaSimGoalReqHeader::SetSenderPos(Vector senderPos)
{
  SenderPos = senderPos;
}
void
AquaSimGoalReqHeader::SetSinkPos(Vector sinkPos)
{
  SinkPos = sinkPos;
}
void
AquaSimGoalReqHeader::SetSourcePos(Vector sourcePos)
{
  SourcePos = sourcePos;
}

AquaSimAddress
AquaSimGoalReqHeader::GetSA()
{
  return SA;
}
AquaSimAddress
AquaSimGoalReqHeader::GetRA()
{
  return RA;
}
AquaSimAddress
AquaSimGoalReqHeader::GetDA()
{
  return DA;
}
Time
AquaSimGoalReqHeader::GetSendTime()
{
  return m_SendTime;
}
Time
AquaSimGoalReqHeader::GetTxTime()
{
  return m_TxTime;
}
uint8_t
AquaSimGoalReqHeader::GetReqID()
{
  return m_ReqID;
}
Vector
AquaSimGoalReqHeader::GetSenderPos()
{
  return SenderPos;
}
Vector
AquaSimGoalReqHeader::GetSinkPos()
{
  return SinkPos;
}
Vector
AquaSimGoalReqHeader::GetSourcePos()
{
  return SourcePos;
}

uint32_t
AquaSimGoalReqHeader::GetSerializedSize(void) const
{
  return 1+1+1+4+4+1+12+12+12;
}
void
AquaSimGoalReqHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (SA.GetAsInt());
  i.WriteU8 (RA.GetAsInt());
  i.WriteU8 (DA.GetAsInt());
  //i.WriteU8 (SA.GetLength());
  //i.WriteU8 (RA.GetLength());
  //i.WriteU8 (DA.GetLength());
  i.WriteU32((uint32_t)(m_SendTime.GetSeconds()*1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_TxTime.GetSeconds()*1000.0 + 0.5));
  i.WriteU8 (m_ReqID);
  //Messy...
  i.WriteU32 ((uint32_t)(SenderPos.x*1000.0 +0.5));
  i.WriteU32 ((uint32_t)(SenderPos.y*1000.0 +0.5));
  i.WriteU32 ((uint32_t)(SenderPos.z*1000.0 +0.5));
  i.WriteU32 ((uint32_t)(SinkPos.x*1000.0 +0.5));
  i.WriteU32 ((uint32_t)(SinkPos.y*1000.0 +0.5));
  i.WriteU32 ((uint32_t)(SinkPos.z*1000.0 +0.5));
  i.WriteU32 ((uint32_t)(SourcePos.x*1000.0 +0.5));
  i.WriteU32 ((uint32_t)(SourcePos.y*1000.0 +0.5));
  i.WriteU32 ((uint32_t)(SourcePos.z*1000.0 +0.5));
}
uint32_t
AquaSimGoalReqHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  SA = (AquaSimAddress) i.ReadU8();
  RA = (AquaSimAddress) i.ReadU8();
  DA = (AquaSimAddress) i.ReadU8();
  //ReadFrom(i, SA, 8);	//read 8bit addr
  //ReadFrom(i, RA, 8); //read 8bit addr
  //ReadFrom(i, DA, 8);	//read 8bit addr
  m_SendTime = Seconds ( ( (double) i.ReadU32()) / 1000.0 );
  m_TxTime = Seconds ( ( (double) i.ReadU32()) / 1000.0 );
  m_ReqID = i.ReadU8();
  //Messy...
  SenderPos.x = ( (double) i.ReadU32() ) / 1000.0;
  SenderPos.y = ( (double) i.ReadU32() ) / 1000.0;
  SenderPos.z = ( (double) i.ReadU32() ) / 1000.0;
  SinkPos.x = ( (double) i.ReadU32() ) / 1000.0;
  SinkPos.y = ( (double) i.ReadU32() ) / 1000.0;
  SinkPos.z = ( (double) i.ReadU32() ) / 1000.0;
  SourcePos.x = ( (double) i.ReadU32() ) / 1000.0;
  SourcePos.y = ( (double) i.ReadU32() ) / 1000.0;
  SourcePos.z = ( (double) i.ReadU32() ) / 1000.0;

  return GetSerializedSize();
}
void
AquaSimGoalReqHeader::Print (std::ostream &os) const
{
  os << "GOAL Req Header: SenderAddress=" << SA << ", RecvAddress=" << RA << ", DestAddress=" <<
        DA << ", SendTime=" << m_SendTime << ", TxTime=" << m_TxTime << ", ReqId=" << m_ReqID <<
        ", SenderPos=" << SenderPos.x << "," << SenderPos.y << "," << SenderPos.z <<
        ", SinkPos=" << SinkPos.x << "," << SinkPos.y << "," << SinkPos.z <<
        ", SourcePos=" << SourcePos.x << "," << SourcePos.y << "," << SourcePos.z << "\n";
}
TypeId
AquaSimGoalReqHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}


NS_OBJECT_ENSURE_REGISTERED(AquaSimGoalRepHeader);

AquaSimGoalRepHeader::AquaSimGoalRepHeader()
{
}
AquaSimGoalRepHeader::~AquaSimGoalRepHeader()
{
}

TypeId
AquaSimGoalRepHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::AquaSimGoalRepHeader")
    .SetParent<Header>()
    .AddConstructor<AquaSimGoalRepHeader>()
  ;
  return tid;
}

uint32_t
AquaSimGoalRepHeader::size(BackoffType type)
{
  return 15; //bytes
}
void
AquaSimGoalRepHeader::SetSA(AquaSimAddress sa)
{
  SA = sa;
}
void
AquaSimGoalRepHeader::SetRA(AquaSimAddress ra)
{
  RA = ra;
}
void
AquaSimGoalRepHeader::SetSendTime(Time sendtime)
{
  m_SendTime = sendtime;
}
void
AquaSimGoalRepHeader::SetTxTime(Time txtime)
{
  m_TxTime = txtime;
}
void
AquaSimGoalRepHeader::SetReqID(uint8_t reqid)
{
  m_ReqID = reqid;
}
void AquaSimGoalRepHeader::SetBackoffTime(Time backoffTime)
{
  m_BackoffTime = backoffTime;
}
void
AquaSimGoalRepHeader::SetReplyerPos(Vector replyerPos)
{
  ReplyerPos = replyerPos;
}

AquaSimAddress
AquaSimGoalRepHeader::GetSA()
{
  return SA;
}
AquaSimAddress
AquaSimGoalRepHeader::GetRA()
{
  return RA;
}
Time
AquaSimGoalRepHeader::GetSendTime()
{
  return m_SendTime;
}
Time
AquaSimGoalRepHeader::GetTxTime()
{
  return m_TxTime;
}
uint8_t
AquaSimGoalRepHeader::GetReqID()
{
  return m_ReqID;
}
Time
AquaSimGoalRepHeader::GetBackoffTime()
{
  return m_BackoffTime;
}
Vector
AquaSimGoalRepHeader::GetReplyerPos()
{
  return ReplyerPos;
}

uint32_t
AquaSimGoalRepHeader::GetSerializedSize(void) const
{
  return 1+1+4+4+1+4+12;
}
void
AquaSimGoalRepHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8(SA.GetAsInt());
  i.WriteU8(RA.GetAsInt());
  //i.WriteU8(SA.GetLength());
  //i.WriteU8(RA.GetLength());
  i.WriteU32((uint32_t)(m_SendTime.GetSeconds()*1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_TxTime.GetSeconds()*1000.0 + 0.5));
  i.WriteU8(m_ReqID);
  i.WriteU32((uint32_t)(m_BackoffTime.GetSeconds()*1000.0 + 0.5));
  //Messy...
  i.WriteU32((uint32_t)(ReplyerPos.x*1000.0 +0.5));
  i.WriteU32((uint32_t)(ReplyerPos.y*1000.0 +0.5));
  i.WriteU32((uint32_t)(ReplyerPos.z*1000.0 +0.5));
}
uint32_t
AquaSimGoalRepHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  SA = (AquaSimAddress) i.ReadU8();
  RA = (AquaSimAddress) i.ReadU8();
  //ReadFrom(i, SA,8);	//read 8bit addr
  //ReadFrom(i, RA, 8);	//read 8bit addr
  m_SendTime = Seconds ( ( (double) i.ReadU32()) / 1000.0 );
  m_TxTime = Seconds ( ( (double) i.ReadU32()) / 1000.0 );
  m_ReqID = i.ReadU8();
  m_BackoffTime = Seconds ( ( (double) i.ReadU32()) / 1000.0 );
  ReplyerPos.x = ( (double) i.ReadU32() ) / 1000.0;
  ReplyerPos.y = ( (double) i.ReadU32() ) / 1000.0;
  ReplyerPos.z = ( (double) i.ReadU32() ) / 1000.0;

  return GetSerializedSize();
}
void
AquaSimGoalRepHeader::Print (std::ostream &os) const
{
  os << "GOAL Rep Header: SenderAddress=" << SA << ", RecvAddress=" << RA <<
        ", SendTime=" << m_SendTime << ", TxTime=" << m_TxTime <<
        ", ReqId=" << m_ReqID << ", BackoffTime=" << m_BackoffTime <<
        ", ReplyerPos=" << ReplyerPos.x << "," << ReplyerPos.y << "," << ReplyerPos.z << "\n";
}
TypeId
AquaSimGoalRepHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}


NS_OBJECT_ENSURE_REGISTERED(AquaSimGoalAckHeader);

AquaSimGoalAckHeader::AquaSimGoalAckHeader()
{
}
AquaSimGoalAckHeader::~AquaSimGoalAckHeader()
{
}

TypeId
AquaSimGoalAckHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::AquaSimGoalAckHeader")
    .SetParent<Header>()
    .AddConstructor<AquaSimGoalAckHeader>()
  ;
  return tid;
}

uint32_t
AquaSimGoalAckHeader::size(BackoffType type)
{
  return 4; //bytes
}
void
AquaSimGoalAckHeader::SetSA(AquaSimAddress sa)
{
  SA = sa;
}
void
AquaSimGoalAckHeader::SetRA(AquaSimAddress ra)
{
  RA = ra;
}
void
AquaSimGoalAckHeader::SetPush(bool push)
{
  m_Push = push;
}
void
AquaSimGoalAckHeader::SetReqID(uint8_t reqid)
{
  m_ReqID = reqid;
}

AquaSimAddress
AquaSimGoalAckHeader::GetSA()
{
  return SA;
}
AquaSimAddress
AquaSimGoalAckHeader::GetRA()
{
  return RA;
}
bool
AquaSimGoalAckHeader::GetPush()
{
  return m_Push;
}
uint8_t
AquaSimGoalAckHeader::GetReqID()
{
  return m_ReqID;
}

uint32_t
AquaSimGoalAckHeader::GetSerializedSize(void) const
{
  return 4;
}
void
AquaSimGoalAckHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (SA.GetAsInt());
  i.WriteU8 (RA.GetAsInt());
  //i.WriteU8 (SA.GetLength());
  //i.WriteU8 (RA.GetLength());
  i.WriteU8 (m_Push);
  i.WriteU8 (m_ReqID);
}
uint32_t
AquaSimGoalAckHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  SA = (AquaSimAddress) i.ReadU8();
  RA = (AquaSimAddress) i.ReadU8();
  //ReadFrom(i, SA,8);	//read 8bit addr
  //ReadFrom(i, RA, 8);	//read 8bit addr
  m_Push = i.ReadU8();
  m_ReqID = i.ReadU8();

  return GetSerializedSize();
}
void
AquaSimGoalAckHeader::Print (std::ostream &os) const
{
  os << "GOAL ACK Header: SenderAddress=" << SA << ", RecvAddress=" << RA <<
        ", Push=";
  if(m_Push == 0) os << "False";
  else os << "True";
  os << ", ReqId=" << m_ReqID << "\n";
}
TypeId
AquaSimGoalAckHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}
