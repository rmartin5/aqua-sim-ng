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
int
AlohaHeader::GetSize()
{
  return 2+2+1; //src, dst, type
}

uint32_t
AlohaHeader::GetSerializedSize(void) const
{
  return 2+2+1;
}
void
AlohaHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteU16 (SA.GetAsInt());
  start.WriteU16 (DA.GetAsInt());
  //start.WriteU8 (SA.GetLength());
  //start.WriteU8 (DA.GetLength());
  start.WriteU8 (m_pType);
}
uint32_t
AlohaHeader::Deserialize (Buffer::Iterator start)
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
  int pkt_size = 2*sizeof(uint16_t)+1; //source and destination address, packet_type

  if( pType == SFAMA_RTS || pType == SFAMA_CTS ) {
    pkt_size += sizeof(uint16_t); //slotnum
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

/*
 * JMAC header
 */
JammingMacHeader::JammingMacHeader()
{
  // // Initialize schedule map
  // // TODO: make the number of nodes a variable (it's fixed to 2 now)
  // for (uint8_t i=0; i<2; i++)
  // {
  //   m_delays_ms.insert(std::make_pair(i, 65535));  // 2**16-1 is the maximum possible delay in ms - this means that the delay is NOT set, by default
  // }
}

JammingMacHeader::~JammingMacHeader()
{
}

TypeId
JammingMacHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::JammingMacHeader")
    .SetParent<Header>()
    .AddConstructor<JammingMacHeader>()
  ;
  return tid;
}

uint32_t
JammingMacHeader::GetSerializedSize(void) const
{
	// DATA
	if (m_ptype == 0)
	{
		// ptype + node_id [in bytes]
		return 2;
	}
	// CC-request
	if (m_ptype == 1)
	{
		return 2 + 3*4; // (x,y,z) coordinates each 4-byte long
	}
	// CS-reply (contains a schedule)
	if (m_ptype == 2)
	{
		return 2 + 8 + 2*GetNodesAmount(); // 16-bits for every node (64 nodes max, currently)
	}

	// This should not happen
  NS_FATAL_ERROR ("JammingMAC packet-type is invalid!");
	return 0;
}

void
JammingMacHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 ((uint8_t)(m_ptype));
  i.WriteU8 ((uint8_t)(m_node_id));
  if (m_ptype == 2)
  {
    // Serialize node_list
    i.WriteU64(m_node_list);
    // Serialize the schedule
    for (uint8_t j=0; j<64; j++)
    {
      if (GetNodeBit(j) == 1)
      {
        i.WriteU16(m_delays_ms.at(j));
      }
    }
  }
  if (m_ptype == 1)
  {
    // serialize coordinates
    i.WriteU32(m_x_coord);
    i.WriteU32(m_y_coord);
    i.WriteU32(m_z_coord);
  }
}

uint32_t
JammingMacHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_ptype = i.ReadU8();
  m_node_id = i.ReadU8();
  if (m_ptype == 2)
  {
    // Deserialize node_list
    m_node_list = i.ReadU64();
    // Deserialize the schedule
    for (uint8_t j=0; j<64; j++)
    {
      if (GetNodeBit(j) == 1)
      {
        m_delays_ms.insert(std::make_pair(j, i.ReadU16()));
      }
    }
  }
  if (m_ptype == 1)
  {
    m_x_coord = i.ReadU32();
    m_y_coord = i.ReadU32();
    m_z_coord = i.ReadU32();
  }
  return GetSerializedSize();
}

void
JammingMacHeader::Print (std::ostream &os) const
{
  os << "JammingMAC Header: ";

  os << "PType=" << int(m_ptype) << " ";
  os << "\n";
}

TypeId
JammingMacHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

void
JammingMacHeader::SetPType(uint8_t ptype)
{
	m_ptype = ptype;
}

uint8_t
JammingMacHeader::GetPType()
{
	return m_ptype;
}

void
JammingMacHeader::SetSchedule(uint8_t node_id, uint16_t delay_ms)
{
  m_delays_ms.insert(std::make_pair(node_id, delay_ms));
  SetNodeBit(node_id);
}

uint16_t
JammingMacHeader::GetSchedule(uint8_t node_id)
{
	return m_delays_ms.at(node_id);
}

void
JammingMacHeader::SetCoordinates(Vector coords)
{
  m_x_coord = coords.x * 1000000;
  m_y_coord = coords.y * 1000000;
  m_z_coord = coords.z * 1000000;
}

Vector
JammingMacHeader::GetCoordinates()
{
  Vector coords = Vector((double) m_x_coord/1000000, (double) m_y_coord/1000000, (double) m_z_coord/1000000);
  return coords;
}

void
JammingMacHeader::SetNodeBit(uint8_t node_id)
{
  int mask = 1 << node_id;  // node_id serves as position
  m_node_list = (m_node_list & ~mask) | ((1 << node_id) & mask);
}

uint8_t
JammingMacHeader::GetNodeBit(uint8_t node_id) const
{
  return (((1 << 1) - 1) & (m_node_list >> node_id));
}

void
JammingMacHeader::SetNodeId(uint8_t node_id)
{
  m_node_id = node_id;
}

uint8_t
JammingMacHeader::GetNodeId()
{
  return m_node_id;
}

uint8_t
JammingMacHeader::GetNodesAmount() const
{
  uint8_t count = 0;
  uint64_t n = m_node_list;
  while (n)
  { 
    count += n & 1; 
    n >>= 1; 
  }
  return count;
}

/*
 * TRUMAC header
 */
TrumacHeader::TrumacHeader()
{
}

TrumacHeader::~TrumacHeader()
{
}

TypeId
TrumacHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::TrumacHeader")
    .SetParent<Header>()
    .AddConstructor<TrumacHeader>()
  ;
  return tid;
}

uint32_t
TrumacHeader::GetSerializedSize(void) const
{
  return 1+1; // ptype + next_sender_id
}

void
TrumacHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 ((uint8_t)(m_ptype));
  i.WriteU8 ((uint8_t)(m_next_sender_id));
}

uint32_t
TrumacHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_ptype = i.ReadU8();
  m_next_sender_id = i.ReadU8();
  return GetSerializedSize();
}

void
TrumacHeader::Print (std::ostream &os) const
{
  os << "TR-MAC Header: ";

  os << "PType=" << int(m_ptype) << " ";
  os << "NextId=" << int(m_next_sender_id) << " ";
  os << "\n";
}

TypeId
TrumacHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

void
TrumacHeader::SetPType(uint8_t ptype)
{
	m_ptype = ptype;
}

uint8_t
TrumacHeader::GetPType()
{
	return m_ptype;
}

void
TrumacHeader::SetNextNodeId(uint8_t next_node_id)
{
  m_next_sender_id = next_node_id;
}

uint8_t
TrumacHeader::GetNextNodeId()
{
  return m_next_sender_id;
}

/*
 * MacLibraHeader
 */
MacLibraHeader::MacLibraHeader()
{
}

MacLibraHeader::~MacLibraHeader()
{
}

TypeId
MacLibraHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::MacLibraHeader")
    .SetParent<Header>()
    .AddConstructor<MacLibraHeader>()
  ;
  return tid;
}

uint32_t
MacLibraHeader::GetSerializedSize(void) const
{
	// DATA
	if (m_ptype == 0)
	{
		// ptype + header_id + hop_count + reward + ... [in bytes]
		return 45;
	}
	// RREQ / RREP
	if ((m_ptype == 1) || (m_ptype == 2))
	{
		return 1 + 4 + 1 + 2 + 2 + 16;	// Add tx/rx parameters
	}
	// ACK
	if (m_ptype == 3)
	{
		return 1 + 4 + 4 + 4 + 4 + 16;	// Add tx/rx parameters
	}
	// INIT
	if (m_ptype == 4)
	{
		return 1 + 4 + 4 + 16;	// Add tx/rx parameters
	}
	// RTS
	if (m_ptype == 5)
	{
		return 1 + 4 + 4 + 16;	// Add tx/rx parameters
	}
	// CTS
	if (m_ptype == 6)
	{
		return 1 + 4 + 4 + 16;	// Add tx/rx parameters
	}
	// Direct Reward Message
	if (m_ptype == 7)
	{
		return 29 + 4;
	}

	// This should not happen
	return 0;
}

void
MacLibraHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  if (m_ptype == 0)
  {
	  i.WriteU8 ((uint8_t)(m_ptype));
	  i.WriteU32 ((uint32_t)(m_header_id));
	  i.WriteU8 ((uint8_t)(m_hop_count));
	  i.WriteU32 ((uint32_t)(m_reward));
	  i.WriteU16 ((uint16_t)(m_sender_addr.GetAsInt()));
	  i.WriteU16 ((uint16_t)(m_src_addr.GetAsInt()));
	  i.WriteU16 ((uint16_t)(m_dst_addr.GetAsInt()));
	  i.WriteU32 ((uint32_t)(m_direct_distance));
	  // Add tx/rx parameters
	  i.WriteU64 ((uint64_t)(m_tx_power));
	  i.WriteU64 ((uint64_t)(m_rx_power));
	  i.WriteU32 ((uint32_t)(m_next_hop_distance));
	  i.WriteU32 ((uint32_t)(m_optimal_distance));
	  i.WriteU8 ((uint8_t)(m_max_hops_number));
  }
  if (m_ptype == 1)
  {
	  i.WriteU8 ((uint8_t)(m_ptype));
	  i.WriteU32 ((uint32_t)(m_header_id));
	  i.WriteU8 ((uint8_t)(m_hop_count));
	  i.WriteU16 ((uint16_t)(m_src_addr.GetAsInt()));
	  i.WriteU16 ((uint16_t)(m_dst_addr.GetAsInt()));
	  // Add tx/rx parameters
	  i.WriteU64 ((uint64_t)(m_tx_power));
	  i.WriteU64 ((uint64_t)(m_rx_power));
  }
  if (m_ptype == 2)
  {
	  i.WriteU8 ((uint8_t)(m_ptype));
	  i.WriteU32 ((uint32_t)(m_header_id));
	  i.WriteU8 ((uint8_t)(m_hop_count));
	  i.WriteU16 ((uint16_t)(m_src_addr.GetAsInt()));
	  i.WriteU16 ((uint16_t)(m_dst_addr.GetAsInt()));
	  // Add tx/rx parameters
	  i.WriteU64 ((uint64_t)(m_tx_power));
	  i.WriteU64 ((uint64_t)(m_rx_power));
  }
  if (m_ptype == 3)
  {
	  i.WriteU8 ((uint8_t)(m_ptype));
	  i.WriteU32 ((uint32_t)(m_header_id));
	  i.WriteU8 ((uint32_t)(m_reward));
	  i.WriteU32 ((uint32_t)(m_ack_message_id));
	  i.WriteU16 ((uint16_t)(m_src_addr.GetAsInt()));
	  i.WriteU16 ((uint16_t)(m_dst_addr.GetAsInt()));
	  // Add tx/rx parameters
	  i.WriteU64 ((uint64_t)(m_tx_power));
	  i.WriteU64 ((uint64_t)(m_rx_power));
  }
  // 4 - INIT
  if (m_ptype == 4)
  {
	  i.WriteU8 ((uint8_t)(m_ptype));
	  i.WriteU32 ((uint32_t)(m_header_id));
	  i.WriteU16 ((uint16_t)(m_src_addr.GetAsInt()));
	  i.WriteU16 ((uint16_t)(m_dst_addr.GetAsInt()));
	  // Add tx/rx parameters
	  i.WriteU64 ((uint64_t)(m_tx_power));
	  i.WriteU64 ((uint64_t)(m_rx_power));
  }
  // 5 - RTS
  if (m_ptype == 5)
  {
	  i.WriteU8 ((uint8_t)(m_ptype));
	  i.WriteU32 ((uint32_t)(m_header_id));
	  i.WriteU16 ((uint16_t)(m_src_addr.GetAsInt()));
	  i.WriteU16 ((uint16_t)(m_dst_addr.GetAsInt()));
	  // Add tx/rx parameters
	  i.WriteU64 ((uint64_t)(m_tx_power));
	  i.WriteU64 ((uint64_t)(m_rx_power));
  }
  // 6 - CTS
  if (m_ptype == 6)
  {
	  i.WriteU8 ((uint8_t)(m_ptype));
	  i.WriteU32 ((uint32_t)(m_header_id));
	  i.WriteU16 ((uint16_t)(m_src_addr.GetAsInt()));
	  i.WriteU16 ((uint16_t)(m_dst_addr.GetAsInt()));
	  // Add tx/rx parameters
	  i.WriteU64 ((uint64_t)(m_tx_power));
	  i.WriteU64 ((uint64_t)(m_rx_power));
  }
  // 7 - Direct Reward Message
  if (m_ptype == 7)
  {
	  i.WriteU8 ((uint8_t)(m_ptype));
	  i.WriteU32 ((uint32_t)(m_header_id));
	  i.WriteU16 ((uint16_t)(m_src_addr.GetAsInt()));
	  i.WriteU16 ((uint16_t)(m_dst_addr.GetAsInt()));
	  i.WriteU32 ((uint32_t)(m_reward));
	  // Add tx/rx parameters
	  i.WriteU64 ((uint64_t)(m_tx_power));
	  i.WriteU64 ((uint64_t)(m_rx_power));
	  i.WriteU32 ((uint32_t)(m_next_hop_distance));
  }

}

uint32_t
MacLibraHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_ptype = i.ReadU8();
//  std::cout << "DESERIALIZED:" << m_ptype << "\n";

  // DATA frame format: | PTYPE | ID | HOP_COUNT | REWARD_VALUE | SENDER_ADDR | SRC_ADDR | DST_ADDR | DIRECT_DISTANCE | TX_POWER | RX_POWER |
  if (m_ptype == 0)
  {
//	  m_ptype = i.ReadU8();
	  m_header_id = i.ReadU32();
	  m_hop_count = i.ReadU8();
	  m_reward = i.ReadU32();
	  m_sender_addr = (AquaSimAddress) i.ReadU16();
	  m_src_addr = (AquaSimAddress) i.ReadU16();
	  m_dst_addr = (AquaSimAddress) i.ReadU16();
	  m_direct_distance = i.ReadU32();
	  // Get tx/rx parameters
	  m_tx_power = i.ReadU64();
	  m_rx_power = i.ReadU64();
	  m_next_hop_distance = i.ReadU32();
	  // Carry optimal_distance value
	  m_optimal_distance = i.ReadU32();
	  m_max_hops_number = i.ReadU8();
  }
  if (m_ptype == 1)
  {
//	  m_ptype = i.ReadU16();
	  m_header_id = i.ReadU32();
	  m_hop_count = i.ReadU8();
	  m_src_addr = (AquaSimAddress) i.ReadU16();
	  m_dst_addr = (AquaSimAddress) i.ReadU16();
	  // Get tx/rx parameters
	  m_tx_power = i.ReadU64();
	  m_rx_power = i.ReadU64();
  }
  if (m_ptype == 2)
  {
//	  m_ptype = i.ReadU8();
	  m_header_id = i.ReadU32();
	  m_hop_count = i.ReadU8();
	  m_src_addr = (AquaSimAddress) i.ReadU16();
	  m_dst_addr = (AquaSimAddress) i.ReadU16();
	  // Get tx/rx parameters
	  m_tx_power = i.ReadU64();
	  m_rx_power = i.ReadU64();
  }
  if (m_ptype == 3)
  {
//	  m_ptype = i.ReadU8();
	  m_header_id = i.ReadU32();
	  m_reward = i.ReadU32();
	  m_ack_message_id = i.ReadU32();
	  m_src_addr = (AquaSimAddress) i.ReadU16();
	  m_dst_addr = (AquaSimAddress) i.ReadU16();
	  // Get tx/rx parameters
	  m_tx_power = i.ReadU64();
	  m_rx_power = i.ReadU64();
  }
  // 4 - INIT
  if (m_ptype == 4)
  {
//	  m_ptype = i.ReadU8();
	  m_header_id = i.ReadU32();
//	  m_reward = i.ReadU32();
//	  m_ack_message_id = i.ReadU32();
	  m_src_addr = (AquaSimAddress) i.ReadU16();
	  m_dst_addr = (AquaSimAddress) i.ReadU16();
	  // Get tx/rx parameters
	  m_tx_power = i.ReadU64();
	  m_rx_power = i.ReadU64();
  }
  // 5 - RTS
  if (m_ptype == 5)
  {
//	  m_ptype = i.ReadU8();
	  m_header_id = i.ReadU32();
//	  m_reward = i.ReadU32();
//	  m_ack_message_id = i.ReadU32();
	  m_src_addr = (AquaSimAddress) i.ReadU16();
	  m_dst_addr = (AquaSimAddress) i.ReadU16();
	  // Get tx/rx parameters
	  m_tx_power = i.ReadU64();
	  m_rx_power = i.ReadU64();
  }
  // 6 - CTS
  if (m_ptype == 6)
  {
//	  m_ptype = i.ReadU8();
	  m_header_id = i.ReadU32();
//	  m_reward = i.ReadU32();
//	  m_ack_message_id = i.ReadU32();
	  m_src_addr = (AquaSimAddress) i.ReadU16();
	  m_dst_addr = (AquaSimAddress) i.ReadU16();
	  // Get tx/rx parameters
	  m_tx_power = i.ReadU64();
	  m_rx_power = i.ReadU64();
  }
  // 7 - Direct Reward Message
  if (m_ptype == 7)
  {
//	  m_ptype = i.ReadU8();
	  m_header_id = i.ReadU32();
//	  m_reward = i.ReadU32();
//	  m_ack_message_id = i.ReadU32();
	  m_src_addr = (AquaSimAddress) i.ReadU16();
	  m_dst_addr = (AquaSimAddress) i.ReadU16();
	  m_reward = i.ReadU32();
	  // Get tx/rx parameters
	  m_tx_power = i.ReadU64();
	  m_rx_power = i.ReadU64();
	  // Carry next_hop_distance
	  m_next_hop_distance = i.ReadU32();
  }

  return GetSerializedSize();
}

void
MacLibraHeader::Print (std::ostream &os) const
{
  os << "MacRouting Header: ";

  if (m_ptype == 0)
  {
	  os << "PType=" << int(m_ptype) << " ";
	  os << "Header_id=" << m_header_id << " ";
	  os << "Hop_count=" << int(m_hop_count) << " ";
	  os << "Reward=" << m_reward / m_multiplier_32 << " ";
	  os << "Sender_addr=" << m_sender_addr << " ";
	  os << "Src_addr=" << m_src_addr << " ";
	  os << "Dst_addr=" << m_dst_addr << " ";
	  os << "Direct_distance=" << m_direct_distance / m_multiplier_32 << " ";
	  os << "Tx_power=" << m_tx_power / m_multiplier_64 << " ";
	  os << "Rx_power=" << m_rx_power / m_multiplier_64 << " ";
	  os << "Next_hop_distance=" << m_next_hop_distance / m_multiplier_32 << " ";
	  os << "Optimal_distance=" << m_optimal_distance / m_multiplier_32 << " ";
	  os << "Max_hops_number=" << int(m_max_hops_number) << " ";
  }

  if (m_ptype == 7)
  {
	  os << "PType=" << int(m_ptype) << " ";
	  os << "Header_id=" << m_header_id << " ";
	  os << "Src_addr=" << m_src_addr << " ";
	  os << "Dst_addr=" << m_dst_addr << " ";
	  os << "Reward=" << m_reward / m_multiplier_32 << " ";
	  os << "Tx_power=" << m_tx_power / m_multiplier_64 << " ";
	  os << "Rx_power=" << m_rx_power / m_multiplier_64 << " ";
	  os << "Next_hop_distance=" << m_next_hop_distance / m_multiplier_32 << " ";
  }

  os << "\n";
}

TypeId
MacLibraHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

void
MacLibraHeader::SetPType(uint8_t ptype)
{
	m_ptype = ptype;
}

int
MacLibraHeader::GetPType()
{
	return m_ptype;
}

void
MacLibraHeader::SetId(uint32_t header_id)
{
	m_header_id = header_id;
}

void
MacLibraHeader::SetHopCount(uint8_t hop_count)
{
	m_hop_count = hop_count;
}

void
MacLibraHeader::SetSrcAddr(AquaSimAddress src_addr)
{
	m_src_addr = src_addr;
}

void
MacLibraHeader::SetDstAddr(AquaSimAddress dst_addr)
{
	m_dst_addr = dst_addr;
}

void
MacLibraHeader::SetSenderAddr(AquaSimAddress sender_addr)
{
	m_sender_addr = sender_addr;
}

AquaSimAddress
MacLibraHeader::GetSrcAddr()
{
	return m_src_addr;
}

AquaSimAddress
MacLibraHeader::GetDstAddr()
{
	return m_dst_addr;
}

AquaSimAddress
MacLibraHeader::GetSenderAddr()
{
	return m_sender_addr;
}

void
MacLibraHeader::SetReward(double reward_value)
{
	m_reward = reward_value * m_multiplier_32;
}

double
MacLibraHeader::GetReward()
{
	return m_reward / m_multiplier_32;
}

int
MacLibraHeader::GetHopCount()
{
	return m_hop_count;
}

void
MacLibraHeader::IncrementHopCount()
{
	m_hop_count++;
}

void
MacLibraHeader::SetRxPower(double rx_power)
{
	m_rx_power = rx_power * m_multiplier_64;
//	std::cout << "SET RX POWER: " << m_tx_power << "\n";
}

void
MacLibraHeader::SetTxPower(double tx_power)
{
	m_tx_power = tx_power * m_multiplier_64;
}

void
MacLibraHeader::SetDirectDistance(double direct_distance)
{
	m_direct_distance = direct_distance * m_multiplier_32;
}

void
MacLibraHeader::SetOptimalDistance(double optimal_distance)
{
	m_optimal_distance = optimal_distance * m_multiplier_32;
}

void
MacLibraHeader::SetNextHopDistance(double next_hop_distance)
{
	m_next_hop_distance = next_hop_distance * m_multiplier_32;
}

void
MacLibraHeader::SetMaxHopsNumber(uint8_t max_hops_number)
{
	m_max_hops_number = max_hops_number;
}

double
MacLibraHeader::GetRxPower()
{
	return m_rx_power / m_multiplier_64;
}

double
MacLibraHeader::GetTxPower()
{
	return m_tx_power / m_multiplier_64;
}

double
MacLibraHeader::GetDirectDistance()
{
	return m_direct_distance / m_multiplier_32;
}

double
MacLibraHeader::GetOptimalDistance()
{
	return m_optimal_distance / m_multiplier_32;
}

double
MacLibraHeader::GetNextHopDistance()
{
	return m_next_hop_distance / m_multiplier_32;
}

uint8_t
MacLibraHeader::GetMaxHopsNumber()
{
	return m_max_hops_number;
}
