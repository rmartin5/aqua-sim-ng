//....

#include "aqua-sim-header.h"
#include "ns3/header.h"
#include "ns3/buffer.h"
#include "ns3/log.h"
#include <iostream>

using namespace ns3;

#include "ns3/address.h"

//Aqua Sim Header

NS_LOG_COMPONENT_DEFINE("AquaSimHeader");
NS_OBJECT_ENSURE_REGISTERED(AquaSimHeader);

AquaSimHeader::AquaSimHeader(void)	//shouldnt direction be NONE to start?
: m_txTime(0), m_direction(DOWN), m_nextHop(Address()),
  m_numForwards(0), m_src.addr(Address()), m_dst.addr(Address()),
  m_src.port(0), m_dst.port(0), 
  m_errorFlag(0), m_uId(-1)
{
}

AquaSimHeader::~AquaSimHeader(void)
{
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
	m_txTime = i.ReadNtohU32();
	m_direction = i.ReadU8();	//wasted space due to only needing 2 bits
	m_nextHop = i.ReadU8();
	m_numForwards = i.ReadNtohU16();
	m_src.addr = i.ReadU8();
	m_src.port = i.ReadNtohU32();
	m_dst.addr = i.ReadU8();
	m_dst.port = i.ReadNtohU32();
	m_errorFlag = i.ReadU8();	//wasted space due to only needing 1 bit
	m_uId = i.ReadNtohU16();

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
	return (4 + 1 + 1 + 2 + 5 + 5 + 1 + 2)
}

void 
AquaSimHeader::Serialize(Buffer::Iterator start) const
{
	Buffer::Iterator i = start;
	i.WriteHtonU32(m_txTime);
	i.WriteU8(m_direction);
	i.WriteU8(m_nextHop);
	i.WriteHtonU16(m_numForwards);
	i.WriteU8(m_src.addr);
	i.WriteHtonU32(m_src.port);
	i.WriteU8(m_dst.addr);
	i.WriteHtonU32(m_dst.port);
	i.WriteU8(m_errorFlag);
	i.WriteHtonU16(m_uId);
}

void 
AquaSimHeader::Print(std::ostream &os) const
{
	os << "TxTime=" << m_txTime << " Direction=";

	if (m_direction == -1)
	{
		os << " DOWN";
	}
	if (m_direction == 0)
	{
		os << " NONE";
	}
	if (m_direction == 1)
	{
		os << " UP";
	}

	os << " NextHop=" << m_nextHop << " NumForwards=" << m_numForwards;
	os << " Source(addr,port)=(" << m_src.addr << "," << m_src.port << ")";
	os << " Dest(addr,port)=(" << m_dst.addr << "," << m_dst.port << ")";
	os << " Error=";

	if (m_errorFlag != 0)
	{
		os << "True";
	}
	else
	{
		os << "False";
	}

	os << " UniqueID=" << m_uId;
}

uint32_t
AquaSimHeader::GetTxTime(void)
{
	return m_txTime;
}

uint32_t
AquaSimHeader::GetSize(void)
{
	return GetSerializedSize();
}

uint8_t&
AquaSimHeader::GetDirection(void)
{
	return m_direction;
}

Address
AquaSimHeader::GetNextHop(void)
{
	return m_nextHop;
}

uint16_t
AquaSimHeader::GetNumForwards(void)
{
	return m_numForwards;
}

Address
AquaSimHeader::GetSAddr(void)
{
	return (m_src.addr);
}

Address
AquaSimHeader::GetDAddr(void)
{
	return (m_dst.addr);
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

uint8_t
AquaSimHeader::GetErrorFlag(void)
{
	return m_errorFlag;
}

uint16_t
AquaSimHeader::GetUId(void)
{
	return m_uId;
}

uint32_t 
AquaSimHeader::SetTxTime(uint32_t time)
{
	m_txTime = time;
}

uint8_t 
AquaSimHeader::SetDirection(uint8_t direction)
{
	m_direction = direction;
}

Address
AquaSimHeader::SetNextHop(Address nextHop)
{
	m_nextHop = nextHop;
}

uint16_t 
AquaSimHeader::SetNumForwards(uint16_t numForwards)
{
	m_numForwards = numForwards;
}

Address
AquaSimHeader::SetSAddr(Address sAddr)
{
	m_src.addr = sAddr;
}

Address
AquaSimHeader::SetDAddr(Address dAddr)
{
	m_dst.addr = dAddr;
}

int32_t 
AquaSimHeader::SetSPort(int32_t sPort)
{
	m_src.port = sPort;
}

int32_t
AquaSimHeader::SetDPort(int32_t dPort)
{
	m_dst.port = dPort;
}

uint8_t 
AquaSimHeader::SetErrorFlag(uint8_t error)
{
	m_errorFlag = error;
}

uint16_t 
AquaSimHeader::SetUId(uint16_t uId)
{
	NS_LOG_FUNCTION(this << "this is not unique and must be removed/implemented");
	m_uId = uId;
}
