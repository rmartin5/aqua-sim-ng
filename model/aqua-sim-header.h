/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef AQUA_SIM_HEADER_H
#define AQUA_SIM_HEADER_H

#include <string>
#include <iostream>

#include "ns3/address.h"
#include "ns3/header.h"

namespace ns3 {

class Packet;

/**
 *  Aqua-Sim Header
 */


struct addr_t {
  Address addr;
  int32_t port;
};

class AquaSimHeader : public Header
{
public:

  enum dir_t { DOWN = 0, NONE = 1, UP = 2 };
  enum AquaSimMacDemuxPktType{ UWPTYPE_LOC, UWPTYPE_SYNC, UWPTYPE_OTHER };
  
  AquaSimHeader();
  virtual ~AquaSimHeader();
  static TypeId GetTypeId(void);

  //Protocol specific access to the header

  /***TODO This is clunky and needs to be revamped to match real world headers***/

  //Getters
  double GetTxTime();  // tx time for this packet in sec
  uint32_t GetSize();	// simulated packet size
  uint8_t GetDirection();/* setting all direction of pkts to be downward as default
						until channel changes it to +1 (UPWARD) */
  //uint8_t GetAddrType();	//type of next hop addr... not in use currently
  Address GetNextHop();	// next hop for this packet
  uint8_t GetNumForwards();	// how many times this pkt was forwarded
  Address GetSAddr();
  Address GetDAddr();
  int32_t GetSPort();
  int32_t GetDPort();
  bool GetErrorFlag();
  uint16_t GetUId();

  //Packet Stamp Getters:
  double GetTxRange();
  double GetPt();	//TODO remove or adapt double variables (conversion to uint32 issues)
  double GetPr();
  double GetFreq();
  double GetNoise();
  std::string GetModName();

  /*
  * Demux feature needs to be implemented
  AquaSimMacDemuxPktType & MacDemuxPType(void) { return m_macDemuxPType; }
  */

  //Setters
  void SetTxTime(double time);
  void SetDirection(uint8_t direction);
  void SetNextHop(Address nextHop);
  void SetNumForwards(uint8_t numForwards);
  void SetSAddr(Address sAddr);
  void SetDAddr(Address dAddr);
  void SetSPort(int32_t sPort);
  void SetDPort(int32_t dPort);
  void SetErrorFlag(bool error);
  void SetUId(uint16_t uId);

  //Packet Stamp Setters:
  void SetTxRange(double txRange);
  void SetPt(double pt);
  void SetPr(double pr);
  void SetFreq(double freq);
  void SetNoise(double noise);
  void SetModName(std::string modName);


  bool CheckConflict();  //check if parameters conflict
  void Stamp(Ptr<Packet> p, double pt, double pr);

  //inherited by Header class
  virtual TypeId GetInstanceTypeId(void) const;
  virtual void Print(std::ostream &os) const;
  virtual void Serialize(Buffer::Iterator start) const;
  virtual uint32_t Deserialize(Buffer::Iterator start);
  virtual uint32_t GetSerializedSize(void) const;

private:
  //uint32_t m_data;
  double m_txTime;
  uint8_t m_direction;  // direction: 1=down, 2=none, 3=up
  //uint8_t m_addrType;
  Address m_nextHop;
  uint8_t m_numForwards;
  addr_t m_src;
  addr_t m_dst;
  uint8_t m_errorFlag;	
  uint16_t m_uId;


  //Packet Stamp variables:
  /**
  * only one between m_ptLevel and m_txRange can should be set
  * run CheckConflict() to avoid conflict
  */

  double m_pt;		//transmission power, set according to Pt_level_
  double m_pr;		//rx power, set by channel/propagation module
  double m_txRange;	//transmission range
  double m_freq;	//central frequency
  double m_noise;	//background noise at the receiver side
  std::string m_modName;

  /*
  * Demux features needs to be implemented
  *AquaSimMacDemuxPktType m_macDemuxPType;
  */

}; //class AquaSimHeader

}  // namespace ns3

#endif /* AQUA_SIM_HEADER_H */
