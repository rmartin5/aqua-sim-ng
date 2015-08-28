/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include <iostream>

using namespace ns3;

#ifndef AQUA_SIM_HEADER_H
#define AQUA_SIM_HEADER_H

#include "ns3/address.h"

/* Aqua Sim Header
  */


struct addr_t {
	Address addr;
	int32_t port;
};

class AquaSimHeader : public Header
{
public:

  enum dir_t { DOWN = -1, NONE = 0, UP = 1 };
  
  AquaSimHeader();
  virtual ~AquaSimHeader();

  //Protocol specific access to the header

  /***This is clunky and needs to be revamped to match real world headers***/

  //Getters
  uint32_t GetTxTime();  // tx time for this packet in sec
  uint32_t GetSize();	// simulated packet size
  uint8_t GetDirection();/* setting all direction of pkts to be downward as default
						until channel changes it to +1 (UPWARD) */
  //uint8_t GetAddrType();	//type of next hop addr... not in use currently
  Address GetNextHop();	// next hop for this packet
  uint16_t GetNumForwards();	// how many times this pkt was forwarded
  Address GetSAddr();
  Address GetDAddr();
  int32_t GetSPort();
  int32_t GetDPort();
  uint8_t GetErrorFlag();
  uint16_t GetUId();

  //Setters
  uint32_t SetTxTime(uint32_t time);
  uint8_t SetDirection(uint8_t direction);
  Address SetNextHop(Address nextHop);
  uint16_t SetNumForwards(uint16_t numForwards);
  Address SetSAddr(Address sAddr);
  Address SetDAddr(Address dAddr);
  int32_t SetSPort(int32_t sPort);
  int32_t SetDPort(int32_t dPort);
  uint8_t SetErrorFlag(uint8_t error);
  uint16_t SetUId(uint16_t uId);


  //inherited by Header class
  static TypeId GetTypeId(void);
  virtual TypeId GetInstanceTypeId(void) const;
  virtual void Print(std::ostream &os) const;
  virtual void Serialize(Buffer::Iterator start) const;
  virtual uint32_t Deserialize(Buffer::Iterator start);
  virtual uint32_t GetSerializedSize(void) const;

private:
  //uint32_t m_data;
  uint32_t m_txTime;
  uint8_t m_direction;  // direction: 0=none, 1=up, -1=down
  //uint8_t m_addrType;
  Address m_nextHop;
  uint16_t m_numForwards;
  addr_t m_src;
  addr_t m_dst;
  uint8_t m_errorFlag;	
  uint16_t m_uId;

};


#endif /* AQUA_SIM_HEADER_H */
