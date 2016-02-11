/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef AQUA_SIM_HEADER_H
#define AQUA_SIM_HEADER_H

#include <string>
#include <iostream>

#include "ns3/address.h"
#include "ns3/header.h"
#include "ns3/nstime.h"
//#include "ns3/uan-address.h"	//use this for now.

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
  enum PacketStatus { RECEPTION = 0, COLLISION = 1, INVALID = 2};
  enum AquaSimMacDemuxPktType{ UWPTYPE_LOC, UWPTYPE_SYNC, UWPTYPE_OTHER };
  
  AquaSimHeader();
  virtual ~AquaSimHeader();
  static TypeId GetTypeId(void);

  //Protocol specific access to the header

  /***TODO This is clunky and needs to be revamped to match real world headers***/
  //Could probably combine direction and packet status using std::bitset to utilize uint8_t
      //icmpv6-header shows a helpful example of header flag handling too

  //Getters
  Time GetTxTime();  // tx time for this packet in sec
  uint32_t GetSize();	// simulated packet size
  uint8_t GetDirection();/* setting all direction of pkts to be none as default */
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
  double GetPt();	//TODO adapt double variables (conversion to uint32 issues) *** See TxTime***
  double GetPr();
  double GetFreq();
  double GetNoise();
  std::string GetModName();
  uint8_t GetPacketStatus();	// default is INVALID
  Time GetTimeStamp();

  /*
  * Demux feature needs to be implemented
  AquaSimMacDemuxPktType & MacDemuxPType(void) { return m_macDemuxPType; }
  */

  //Setters
  void SetTxTime(Time time);
  void SetDirection(uint8_t direction);
  void SetNextHop(Address nextHop);
  void SetNumForwards(uint8_t numForwards);
  void SetSAddr(Address sAddr);
  void SetDAddr(Address dAddr);
  void SetSPort(int32_t sPort);	//TODO should be removed...
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
  void SetPacketStatus(uint8_t status);
  void SetTimeStamp(Time timestamp);

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
  Time m_txTime;
  uint8_t m_direction;  // direction: 0=down, 1=none, 2=up
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
  uint8_t m_status;  // 0=reception, 1=collision, 2=invalid
  Time m_timestamp;

  /*
  * Demux features needs to be implemented
  *AquaSimMacDemuxPktType m_macDemuxPType;
  */

}; //class AquaSimHeader

class RMacHeader : public Header
{
public:
  RMacHeader();
  virtual ~RMacHeader();
  static TypeId GetTypeId(void);

  void SetPtype(uint8_t ptype);
  void SetPktNum(uint32_t pkNum);
  void SetDataNum(uint32_t dataNum);
  void SetBlockNum(uint8_t blockNum);
  void SetSenderAddr(Address senderAddr);
  void SetRecvAddr(Address recvAddr);
  void SetSt(double st);
  void SetDuration(double duration);
  void SetInterval(double interval);
  void SetArrivalTime(Time arrivalTime);
  void SetTs(double ts);

  uint8_t GetPtype();
  uint32_t GetPktNum();
  uint32_t GetDataNum();
  uint8_t GetBlockNum();
  Address GetSenderAddr();
  Address GetRecvAddr();
  double GetSt();
  double GetDuration();
  double GetInterval();
  Time GetArrivalTime();
  double GetTs ();

  //inherited methods
  virtual uint32_t GetSerializedSize(void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId(void) const;

private:
  uint8_t m_ptype;     //packet type
  uint32_t m_pkNum;    // sequence number
  uint32_t m_dataNum;
  uint8_t m_blockNum; // the block num, in real world, one bit is enough
  Address m_senderAddr;
  Address m_recvAddr;
  double m_st; // Timestamp when pkt is generated.
  double m_duration;
  double m_interval;
  Time m_arrivalTime;
  double m_ts;

}; // class RMacHeader

}  // namespace ns3

#endif /* AQUA_SIM_HEADER_H */
