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

#include "aqua-sim-header-mac.h"
#include "aqua-sim-rmac.h"
#include "aqua-sim-tmac.h"

#include "ns3/log.h"
#include "ns3/buffer.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MacHeader");
NS_OBJECT_ENSURE_REGISTERED(MacHeader);

MacHeader::MacHeader() :
  m_demuxPType(UWPTYPE_OTHER)
{
}

TypeId
MacHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::MacHeader")
    .SetParent<Header>()
    .AddConstructor<MacHeader>()
  ;
  return tid;
}

uint32_t
MacHeader::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_sa = (AquaSimAddress) i.ReadU16();
  m_da = (AquaSimAddress) i.ReadU16();
  m_demuxPType = i.ReadU8();

  return GetSerializedSize();
}

uint32_t
MacHeader::GetSerializedSize(void) const
{
  //reserved bytes for header
  return (5);
}

void
MacHeader::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU16(m_sa.GetAsInt());
  i.WriteU16(m_da.GetAsInt());
  i.WriteU8(m_demuxPType);
}

void
MacHeader::Print(std::ostream &os) const
{
  os << "Mac Header is: SA=" << m_sa << " DA=" << m_da << " DemuxPType=";
  switch (m_demuxPType){
    case UWPTYPE_OTHER:   os << "OTHER";  break;
    case UWPTYPE_LOC:     os << "LOC";    break;
    case UWPTYPE_SYNC:    os << "SYNC";   break;
    case UWPTYPE_SYNC_BEACON: os << "SYNC-BEACON"; break;
  }
  os << "\n";
}

TypeId
MacHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

AquaSimAddress
MacHeader::GetSA()
{
  return m_sa;
}
AquaSimAddress
MacHeader::GetDA()
{
  return m_da;
}
uint8_t
MacHeader::GetDemuxPType()
{
  return m_demuxPType;
}
void
MacHeader::SetSA(AquaSimAddress sa)
{
  m_sa = sa;
}
void
MacHeader::SetDA(AquaSimAddress da)
{
  m_da = da;
}
void
MacHeader::SetDemuxPType(uint8_t demuxPType)
{
  m_demuxPType = demuxPType;
}


NS_OBJECT_ENSURE_REGISTERED(TMacHeader);

TMacHeader::TMacHeader()
{
}
TMacHeader::~TMacHeader()
{
}
TypeId
TMacHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::TMacHeader")
    .SetParent<Header>()
    .AddConstructor<TMacHeader>()
  ;
  return tid;
}

uint8_t
TMacHeader::GetPtype()
{
  return m_ptype;
}
uint32_t
TMacHeader::GetPktNum()
{
  return m_pktNum;
}
uint32_t
TMacHeader::GetDataNum()
{
  return m_dataNum;
}
uint8_t
TMacHeader::GetBlockNum()
{
  return m_blockNum;
}
AquaSimAddress
TMacHeader::GetSenderAddr()
{
  return m_senderAddr;
}
AquaSimAddress
TMacHeader::GetRecvAddr()
{
  return m_recvAddr;
}
double
TMacHeader::GetST()
{
  return m_st;
}
double
TMacHeader::GetTS()
{
  return m_ts;
}
double
TMacHeader::GetDuration()
{
  return m_duration;
}
double
TMacHeader::GetInterval()
{
  return m_interval;
}
double
TMacHeader::GetArrivalTime()
{
  return m_arrivalTime;
}

void
TMacHeader::SetPtype(uint8_t ptype)
{
  m_ptype = ptype;
}
void
TMacHeader::SetPktNum(uint32_t pktNum)
{
  m_pktNum = pktNum;
}
void
TMacHeader::SetDataNum(uint32_t dataNum)
{
  m_dataNum = dataNum;
}
void
TMacHeader::SetBlockNum(uint8_t blockNum)
{
  m_blockNum = blockNum;
}
void
TMacHeader::SetSenderAddr(AquaSimAddress senderAddr)
{
  m_senderAddr = senderAddr;
}
void
TMacHeader::SetRecvAddr(AquaSimAddress recvAddr)
{
  m_recvAddr = recvAddr;
}
void
TMacHeader::SetST(double st)
{
  m_st = st;
}
void
TMacHeader::SetTS(double ts)
{
  m_ts = ts;
}
void
TMacHeader::SetDuration(double durable)
{
  m_duration = durable;
}
void
TMacHeader::SetInterval(double interval)
{
  m_interval = interval;
}
void
TMacHeader::SetArrivalTime(double arrivalTime)
{
  m_arrivalTime = arrivalTime;
}

uint32_t
TMacHeader::GetSerializedSize(void) const
{
  return 1+4+4+1+2+2+4+4+4+4+4;
}
void
TMacHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteU8 (m_ptype);
  start.WriteU32 (m_pktNum);
  start.WriteU32 (m_dataNum);
  start.WriteU8 (m_blockNum);
  start.WriteU16 (m_senderAddr.GetAsInt());
  start.WriteU16 (m_recvAddr.GetAsInt());
  start.WriteU32 ((uint32_t)m_st * 1000.0);
  start.WriteU32 ((uint32_t)m_ts * 1000.0);
  start.WriteU32 ((uint32_t)m_duration * 1000.0);
  start.WriteU32 ((uint32_t)m_interval * 1000.0);
  start.WriteU32 ((uint32_t)m_arrivalTime * 1000.0);

}
uint32_t
TMacHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_ptype = i.ReadU8();
  m_pktNum = i.ReadU32();
  m_dataNum = i.ReadU32();
  m_blockNum = i.ReadU8();
  m_senderAddr = (AquaSimAddress) i.ReadU16();
  m_recvAddr = (AquaSimAddress) i.ReadU16();
  //ReadFrom(i, m_senderAddr,8);	//read 8bit addr
  //ReadFrom(i, m_recvAddr, 8);	//read 8bit addr
  m_st = ( (double) i.ReadU32 ()) / 1000.0;
  m_ts = ( (double) i.ReadU32 ()) / 1000.0;
  m_duration = ( (double) i.ReadU32 ()) / 1000.0;
  m_interval = ( (double) i.ReadU32 ()) / 1000.0;
  m_arrivalTime = ( (double) i.ReadU32 ()) / 1000.0;

  return GetSerializedSize();
}
void
TMacHeader::Print (std::ostream &os) const
{
  os << "TMac Header: ptype=";
  switch(m_ptype)
  {
    case PT_OTHER: os << "OTHER"; break;
    case PT_DATA: 	os << "DATA"; break;
    case PT_RTS: 	os << "RTS"; break;
    case PT_CTS: 	os << "CTS"; break;
    case PT_ND: 		os << "ND"; break;
    case PT_SACKND: 	os << "SACKND"; break;
    case PT_ACKDATA: 	os << "ACKDATA"; break;
    case PT_SYN: 	os << "SYN"; break;
    default: break;
  }
  os << " PktNum=" << m_pktNum << " DataNum=" << m_dataNum
     << " BlockNum=" << m_blockNum << " senderAddr=" << m_senderAddr
     << " recvAddr=" << m_recvAddr << " st=" << m_st << " ts=" << m_ts
     << " Duration=" << m_duration << " Interval=" << m_interval
     << " ArrivalTime=" << m_arrivalTime << "\n";
}
TypeId
TMacHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}



/*
 * AlohaHeader
 */
AlohaHeader::AlohaHeader()
{
}

AlohaHeader::~AlohaHeader()
{
}

TypeId
AlohaHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::AlohaHeader")
    .SetParent<Header>()
    .AddConstructor<AlohaHeader>()
  ;
  return tid;
}

int
AlohaHeader::size()
{
  return sizeof(AquaSimAddress)*2 + 1; /*for packet_type*/
}
void
AlohaHeader::SetSA(AquaSimAddress sa)
{
  SA = sa;
}
void
AlohaHeader::SetDA(AquaSimAddress da)
{
  DA = da;
}
void
AlohaHeader::SetPType(uint8_t pType)
{
  m_pType = pType;
}
AquaSimAddress
AlohaHeader::GetSA()
{
  return SA;
}
AquaSimAddress
AlohaHeader::GetDA()
{
  return DA;
}
uint8_t
AlohaHeader::GetPType()
{
  return m_pType;
}

uint32_t
AlohaHeader::GetSerializedSize(void) const
{
  return 1+1+1;
}
void
AlohaHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteU8 (SA.GetAsInt());
  start.WriteU8 (DA.GetAsInt());
  //start.WriteU8 (SA.GetLength());
  //start.WriteU8 (DA.GetLength());
  start.WriteU8 (m_pType);
}
uint32_t
AlohaHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  SA = (AquaSimAddress) i.ReadU8();
  DA = (AquaSimAddress) i.ReadU8();
  //ReadFrom(i, SA,8);	//read 8bit addr
  //ReadFrom(i, DA, 8);	//read 8bit addr
  m_pType = i.ReadU8();

  return GetSerializedSize();
}
void
AlohaHeader::Print (std::ostream &os) const
{
  os << "Aloha Header: SendAddress=" << SA << ", DestAddress=" << DA << ", PacketType=";
  switch(m_pType)
  {
    case DATA: os << "DATA"; break;
    case ACK: os << "ACK"; break;
  }
  os << "\n";
}
TypeId
AlohaHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}



/*
 * FamaHeader
 */
FamaHeader::FamaHeader()
{
}

FamaHeader::~FamaHeader()
{
}

TypeId
FamaHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::FamaHeader")
    .SetParent<Header>()
    .AddConstructor<FamaHeader>()
  ;
  return tid;
}

int
FamaHeader::size()
{
  return sizeof(AquaSimAddress)*4 + 1; /*for packet_type*/
}
void
FamaHeader::SetSA(AquaSimAddress sa)
{
  SA = sa;
}
void
FamaHeader::SetDA(AquaSimAddress da)
{
  DA = da;
}
void
FamaHeader::SetPType(uint8_t pType)
{
  m_pType = pType;
}
AquaSimAddress
FamaHeader::GetSA()
{
  return SA;
}
AquaSimAddress
FamaHeader::GetDA()
{
  return DA;
}
uint8_t
FamaHeader::GetPType()
{
  return m_pType;
}

uint32_t
FamaHeader::GetSerializedSize(void) const
{
  return 2+2+1;
}
void
FamaHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteU16 (SA.GetAsInt());
  start.WriteU16 (DA.GetAsInt());
  //start.WriteU8 (SA.GetLength());
  //start.WriteU8 (DA.GetLength());
  start.WriteU8 (m_pType);
}
uint32_t
FamaHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  SA = (AquaSimAddress) i.ReadU16();
  DA = (AquaSimAddress) i.ReadU16();
  //ReadFrom(i, SA,8);	//read 8bit addr
  //ReadFrom(i, DA, 8);	//read 8bit addr
  m_pType = i.ReadU8();

  return GetSerializedSize();
}
void
FamaHeader::Print (std::ostream &os) const
{
  os << "FAMA Header: SendAddress=" << SA << ", DestAddress=" << DA << ", PacketType=";
  switch(m_pType)
  {
    case RTS: os << "RTS"; break;
    case CTS: os << "CTS"; break;
    case FAMA_DATA: os << "FAMA_DATA"; break;
    case ND: os << "ND"; break;
  }
  os << "\n";
}
TypeId
FamaHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}



/*
 * CopeHeader
 */
CopeHeader::CopeHeader()
{
}

CopeHeader::~CopeHeader()
{
}

TypeId
CopeHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::CopeHeader")
    .SetParent<Header>()
    .AddConstructor<CopeHeader>()
  ;
  return tid;
}

uint32_t
CopeHeader::size()
{
  return GetSerializedSize(); //return m_size;
}
void
CopeHeader::SetSA(AquaSimAddress sa)
{
  SA = sa;
}
void
CopeHeader::SetDA(AquaSimAddress da)
{
  DA = da;
}
void
CopeHeader::SetPType(uint8_t pType)
{
  m_pType = pType;
}
AquaSimAddress
CopeHeader::GetSA()
{
  return SA;
}
AquaSimAddress
CopeHeader::GetDA()
{
  return DA;
}
uint8_t
CopeHeader::GetPType()
{
  return m_pType;
}

uint32_t
CopeHeader::GetSerializedSize(void) const
{
  return 2+2+1;
}
void
CopeHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteU16 (SA.GetAsInt());
  start.WriteU16 (DA.GetAsInt());
  //start.WriteU8 (SA.GetLength());
  //start.WriteU8 (DA.GetLength());
  start.WriteU8 (m_pType);
}
uint32_t
CopeHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  SA = (AquaSimAddress) i.ReadU16();
  DA = (AquaSimAddress) i.ReadU16();
  //ReadFrom(i, SA,8);	//read 8bit addr
  //ReadFrom(i, DA, 8);	//read 8bit addr
  m_pType = i.ReadU8();

  return GetSerializedSize();
}
void
CopeHeader::Print (std::ostream &os) const
{
  os << "COPE-MAC Header: packet_type=";
  switch(m_pType) {
    case COPE_ND: os << "COPE_ND"; break;
    case COPE_ND_REPLY: os << "COPE_ND_REPLY"; break;
    case MULTI_REV: os << "MULTI_REV"; break;
    case MULTI_REV_ACK: os << "MULTI_REV_ACK"; break;
    case MULTI_DATA_ACK: os << "MULTI_DATA_ACK"; break;
    default: break;
  }
  os << ", SenderAddress=" << SA << ", DestAddress=" << DA << "\n";
}
TypeId
CopeHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}


/*
 * SFamaHeader
 */
SFamaHeader::SFamaHeader()
{
}

SFamaHeader::~SFamaHeader()
{
}

TypeId
SFamaHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::SFamaHeader")
    .SetParent<Header>()
    .AddConstructor<SFamaHeader>()
  ;
  return tid;
}

int
SFamaHeader::GetSize(enum PacketType pType)
{
  int pkt_size = 2*sizeof(Address); //source and destination addr in hdr_mac

  if( pType == SFAMA_RTS || pType == SFAMA_CTS ) {
    pkt_size += sizeof(uint16_t)+1; //size of packet_type and slotnum
  }

  return pkt_size;
}
void
SFamaHeader::SetPType(uint8_t pType)
{
  m_pType = pType;
}
void
SFamaHeader::SetSlotNum(uint16_t slotNum)
{
  m_slotNum = slotNum;
}
uint8_t
SFamaHeader::GetPType()
{
  return m_pType;
}
uint16_t
SFamaHeader::GetSlotNum()
{
  return m_slotNum;
}

uint32_t
SFamaHeader::GetSerializedSize(void) const
{
  return 1+2;
}
void
SFamaHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteU8 (m_pType);
  start.WriteU16 (m_slotNum);
}
uint32_t
SFamaHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_pType = i.ReadU8();
  m_slotNum = i.ReadU16();

  return GetSerializedSize();
}
void
SFamaHeader::Print (std::ostream &os) const
{
  os << "Slotted FAMA Header: packet_type=";
  switch(m_pType) {
    case SFAMA_RTS: os << "SFAMA_RTS"; break;
    case SFAMA_CTS: os << "SFAMA_CTS"; break;
    case SFAMA_DATA: os << "SFAMA_DATA"; break;
    case SFAMA_ACK: os << "SFAMA_ACK"; break;
    default: break;
  }
  os << ", SlotNum=" << m_slotNum << "\n";
}
TypeId
SFamaHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}



/*
 * UwanSyncHeader
 */
UwanSyncHeader::UwanSyncHeader()
{
}

UwanSyncHeader::~UwanSyncHeader()
{
}

TypeId
UwanSyncHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::UwanSyncHeader")
    .SetParent<Header>()
    .AddConstructor<UwanSyncHeader>()
  ;
  return tid;
}

int
UwanSyncHeader::GetSize()
{
  return 8*sizeof(Time);
}
void
UwanSyncHeader::SetCyclePeriod(double cyclePeriod)
{
  m_cyclePeriod = cyclePeriod;
}
double
UwanSyncHeader::GetCyclePeriod()
{
  return m_cyclePeriod;
}

uint32_t
UwanSyncHeader::GetSerializedSize(void) const
{
  return 4;
}
void
UwanSyncHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteU32 ((uint32_t)m_cyclePeriod * 1000.0);
}
uint32_t
UwanSyncHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_cyclePeriod = ( (double) i.ReadU32 ()) / 1000.0;

  return GetSerializedSize();
}
void
UwanSyncHeader::Print (std::ostream &os) const
{
  os << "UWAN SYNC Header: cyclePeriod=" << m_cyclePeriod << "\n";
}
TypeId
UwanSyncHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}


/*
 * LocalizationHeader
 */
LocalizationHeader::LocalizationHeader()
{
}

LocalizationHeader::~LocalizationHeader()
{
}

TypeId
LocalizationHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::LocalizationHeader")
    .SetParent<Header>()
    .AddConstructor<LocalizationHeader>()
  ;
  return tid;
}

void
LocalizationHeader::SetNodePosition(Vector nodePosition)
{
  m_nodePosition = nodePosition;
}
void LocalizationHeader::SetConfidence(double confidence)
{
  m_confidence = confidence;
}
Vector
LocalizationHeader::GetNodePosition()
{
  return m_nodePosition;
}
double
LocalizationHeader::GetConfidence()
{
  return m_confidence;
}

uint32_t
LocalizationHeader::GetSerializedSize(void) const
{
  return 16;
}
void
LocalizationHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU32 ((uint32_t)(m_nodePosition.x*1000.0));
  i.WriteU32 ((uint32_t)(m_nodePosition.y*1000.0));
  i.WriteU32 ((uint32_t)(m_nodePosition.z*1000.0));
  i.WriteU32 ((uint32_t)(m_confidence*1000.0));
}
uint32_t
LocalizationHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_nodePosition.x = ( (double) i.ReadU32() ) / 1000.0;
  m_nodePosition.y = ( (double) i.ReadU32() ) / 1000.0;
  m_nodePosition.z = ( (double) i.ReadU32() ) / 1000.0;
  m_confidence = ((double) i.ReadU32())/1000.0;

  return GetSerializedSize();
}
void
LocalizationHeader::Print (std::ostream &os) const
{
  os << "Localization Header: nodePosition(" << m_nodePosition.x <<
      "," << m_nodePosition.y << "," << m_nodePosition.z << "), confidence=" <<
      m_confidence << "\n";
}
TypeId
LocalizationHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}
