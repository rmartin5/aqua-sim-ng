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

AquaSimHeader::AquaSimHeader(void) :
    m_txTime(0), m_direction(0), m_nextHop(),
    m_numForwards(0), m_errorFlag(0), m_uId(-1),
    m_pt(-1), m_pr(-1),
    m_txRange(-1), m_freq(-1), m_noise(0)
{
  //m_src.addr(Address());
  //m_dst.addr(Address());
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
  m_txTime = i.ReadNtohU32();
  m_direction = i.ReadU8();	//wasted space due to only needing 2 bits
  m_numForwards = i.ReadU8();
  m_errorFlag = i.ReadU8();	//wasted space due to only needing 1 bit
  m_uId = i.ReadNtohU16();

  m_pt = i.ReadU8();
  m_pr = i.ReadU8();
  m_txRange = i.ReadNtohU16();
  m_freq = i.ReadU8();
  m_noise = i.ReadU8();

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
  return (4 + 1 + 1 + 1 + 2 + 1 + 1 + 2 + 1 + 1);
}

void 
AquaSimHeader::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtonU32(m_txTime);
  i.WriteU8(m_direction);
  i.WriteU8(m_numForwards);
  i.WriteU8(m_errorFlag);
  i.WriteHtonU16(m_uId);
  //src/dst port + address
  i.WriteU8(m_pt);
  i.WriteU8(m_pr);
  i.WriteU16(m_txRange);
  i.WriteU8(m_freq);
  i.WriteU8(m_noise);
  //m_modName
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

  os << " Tx Power=" << m_pt << " Rx Power=" << m_pr;

  os << " Trans Range=" << m_txRange;

  os << " Frequency=" << m_freq;

  os << " Noise=" << m_noise;

}

double
AquaSimHeader::GetTxTime(void)
{
  return m_txTime;
}

uint32_t
AquaSimHeader::GetSize(void)
{
  return GetSerializedSize();
}

uint8_t
AquaSimHeader::GetDirection(void)
{
  return m_direction;
}

Address
AquaSimHeader::GetNextHop(void)
{
  return m_nextHop;
}

uint8_t
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

double
AquaSimHeader::GetTxRange()
{
  return m_txRange;
}
double
AquaSimHeader::GetPt()
{
  return m_pt;
}
double
AquaSimHeader::GetPr()
{
  return m_pr;
}
double
AquaSimHeader::GetFreq()
{
  return m_freq;
}
double
AquaSimHeader::GetNoise()
{
  return m_noise;
}
std::string
AquaSimHeader::GetModName()
{
  return m_modName;
}



void
AquaSimHeader::SetTxTime(double time)
{
  m_txTime = time;
}

void
AquaSimHeader::SetDirection(uint8_t direction)
{
  m_direction = direction;
}

void
AquaSimHeader::SetNextHop(Address nextHop)
{
  m_nextHop = nextHop;
}

void
AquaSimHeader::SetNumForwards(uint8_t numForwards)
{
  m_numForwards = numForwards;
}

void
AquaSimHeader::SetSAddr(Address sAddr)
{
  m_src.addr = sAddr;
}

void
AquaSimHeader::SetDAddr(Address dAddr)
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
AquaSimHeader::SetErrorFlag(uint8_t error)
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
AquaSimHeader::SetTxRange(double txRange)
{
  m_txRange = txRange;
}
void
AquaSimHeader::SetPt(double pt)
{
  m_pt = pt;
}
void
AquaSimHeader::SetPr(double pr)
{
  m_pr = pr;
}
void
AquaSimHeader::SetFreq(double freq)
{
  m_freq = freq;
}
void
AquaSimHeader::SetNoise(double noise)
{
  m_noise = noise;
}
void
AquaSimHeader::SetModName(std::string modName)
{
  m_modName = modName;
}


bool
AquaSimHeader::CheckConflict()
{
  return (m_txRange > 0 && m_pt < 0) || (m_txRange < 0 && m_pt > 0);
}

void
AquaSimHeader::Stamp(Ptr<Packet> p, double pt, double pr)
{
  m_pt = pt;
  m_pr = pr;
}
