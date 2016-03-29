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

/*
 * 	COPE-MAC
 * COntention based Parallel rEservation MAC
 */

#ifndef AQUA_SIM_MAC_COPEMAC_H
#define AQUA_SIM_MAC_COPEMAC_H

#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"


#include "aqua-sim-mac.h"
#include "aqua-sim-address.h"

#include <map>
#include <vector>
#include <queue>

#define COPEMAC_CALLBACK_DELAY 	0.001
#define COPEMAC_BACKOFF_TIME	0.01
#define	COPEMAC_REV_DELAY	0.5
#define BACKOFF_DELAY_ERROR	1
#define	MAX_INTERVAL		1
#define SEND_DELAY		0.001
#define COPEMAC_MAXIMUMCOUNTER	6

namespace ns3{

//-----------------------------------------------
//save the incoming packets.
//-----------------------------------------------


struct PktElem{
  Ptr<Packet> pkt_;
  PktElem* next_;
  inline PktElem(): next_(NULL) {
  }
  inline PktElem(Ptr<Packet> pkt): pkt_(pkt), next_(NULL) {
  }
};

struct PktList{
  PktElem* head;
  PktElem* tail;
  inline PktList(): head(NULL), tail(NULL){
  };
  inline ~PktList() {
    PktElem* temp = NULL;
    while (head != tail ) {
	temp = head;
	head = head->next_;
	temp->pkt_=0;
	temp->next_ = NULL;
    }
  }
};


class AquaSimCopeMac;

//all pending packets are stored in this data structure
class PktWareHouse{
  friend class AquaSimCopeMac;
private:
  std::map<AquaSimAddress, PktList> Queues;
  int m_cachedPktNum;
  bool m_locked;
public:
  PktWareHouse():m_cachedPktNum(0), m_locked(false) {
  }
  inline bool IsEmpty() {
      return !m_cachedPktNum;
  }

  //inline void Lock() {
  //	m_locked = true;
  //}

  //inline void Unlock() {
  //	m_locked = false;
  //}
  void	Insert2PktQs(Ptr<Packet> p);
  bool	DeletePkt(AquaSimAddress Recver, int SeqNum);
};


//----------------------------------------------------
//cache the reservation request
struct RevReq{
  AquaSimAddress requestor;
  int acceptedRevID;  //if acceptedRevID and rejectedRevID is not succesive, both are rejected
  int rejectedRevID;
  Time StartTime;
  Time EndTime;
};

struct DataAck{
  AquaSimAddress Sender;
  int SeqNum;
};

//---------------------------------------------------
class PktSendTimer: public Timer{
public:
  PktSendTimer(Ptr<AquaSimCopeMac> mac, Ptr<Packet> pkt): Timer() {
      m_mac = mac;
      m_pkt = pkt;
  }
  virtual ~PktSendTimer();
  static TypeId GetTypeId(void);
  void PktSendTimerExpire();

protected:
  Ptr<AquaSimCopeMac> m_mac;
  Ptr<Packet> m_pkt;
};


enum RevType {
  PRE_REV,	//the interval is in reserved in REV Request
  AVOIDING,	//overheared from rev-ack. cannot receive but can send at this time
  SENDING,	//the interval is for that this node sends packet
  RECVING	//the interval is for receiving packet from other nodes
};

//Reservation Element
struct RevElem{
  Time		StartTime;
  Time		EndTime;
  AquaSimAddress	Reservor;   //reserve for which node
  RevType	rev_type;
  int		RevID;


  //node may reserve time for itself to send out packet, this timer is used to send packet
  PktSendTimer* m_sendTimer;
  RevElem*	next;
  RevElem();
  RevElem(int RevID_, Time StartTime_,
	  Time EndTime_, AquaSimAddress Reservor_, RevType rev_type_);
  ~RevElem();
};


class RevQueues : public Object{
private:
  RevElem* Head_;
  Ptr<AquaSimCopeMac> mac_;
public:
  RevQueues(Ptr<AquaSimCopeMac> mac);
  ~RevQueues();
  static TypeId GetTypeId(void);
  /*
   * If [startTime, EndTime] overlaps with some
   * existing reservation time interval,
   * push will fails and false is returned.
   * Otherwise, insert successfully and return true.
   * If force is true, it will not check availability
   */
  bool Push(int RevID, Time StartTime, Time EndTime, AquaSimAddress Reservor,
	  RevType rev_type, Ptr<Packet> pkt);
  void ClearExpired(Time ExpireTime);
  bool CheckAvailability(Time StartTime, Time EndTime, RevType rev_type);
  void DeleteRev(int RevID);
  void UpdateStatus(int RevID, RevType new_type);
  void PrintRevQueue();
  Time GetValidStartTime(Time Interval, Time SinceTime=Simulator::Now());  //absolute time
};

//---------------------------------------------------


struct NDRecord{
	Time nd_sendtime;
	Time nd_recvtime;
};


class AckWaitTimer: public Timer {
public:
  AckWaitTimer():Timer() {}
  Ptr<Packet> m_pkt;
  Ptr<AquaSimCopeMac> m_mac;
  Timer m_ackWaitTimer;
};


class AquaSimCopeMac: public AquaSimMac {
public:
  AquaSimCopeMac();
  ~AquaSimCopeMac();
  static TypeId GetTypeId(void);

  // to process the incoming packet
  virtual bool RecvProcess(Ptr<Packet> pkt);
  // to process the outgoing packet
  virtual bool TxProcess(Ptr<Packet> pkt);
protected:
  void PreSendPkt(Ptr<Packet> pkt, Time delay=Seconds(0.0001));  //send out the packet after delay
  void SendPkt(Ptr<Packet> pkt);
  void Start();
  //int	round2Slot(Time time);  //round the time to the slot sequence num since now
  //Time Slot2Time(int SlotNum, Time BaseTime = Simulator::Now());
  //Time Round2RecverSlotBegin(Time time, AquaSimAddress recver);

  void StartHandShake();
  Time Map2OwnTime(Time SenderTime, AquaSimAddress Sender); //map time SenderTime on sender to the time on this node

  void RecordDataPkt(Ptr<Packet> pkt);
  //process control packets.
  void ProcessND(Ptr<Packet> pkt);
  void ProcessNDReply(Ptr<Packet> pkt);
  void ProcessMultiRev(Ptr<Packet> pkt);
  void ProcessMultiRevAck(Ptr<Packet> pkt);
  void ProcessDataAck(Ptr<Packet> pkt);

  //make control packets.
  Ptr<Packet> MakeND();  //ND packet include the neighbors which this node already knows.
  Ptr<Packet> MakeNDReply();
  Ptr<Packet> MakeMultiRev();
  Ptr<Packet> MakeMultiRevAck();
  Ptr<Packet> MakeDataAck();

  //for test
  void PrintDelayTable();
  void PrintResult();

  void ClearAckWaitingList();
  void InsertAckWaitingList(Ptr<Packet> p, Time delay);
  void ClearExpiredElem();
  void CtrlPktInsert(Ptr<Packet> ctrl_p, Time delay);

  //timeout functions & Events
  void StatusProcess();

  void NDProcessInitor();
void DataSendTimerExpire();
void RevAckAccumTimerExpire();
void DataAckAccumTimerExpire();
void AckWaitTimerExpire(Ptr<Packet> pkt);
void BackoffHandler(Ptr<Packet> pkt);

  //timers
  Timer DataSendTimer;
  Timer RevAckAccumTimer;
  Timer DataAckAccumTimer;
  //Timer CtrlPktTimer;

private:
  /*Time PktInterval_;*/	//the interval between sending two packet
  Time	m_NDInterval;	//the interval between two successive ND process
  Time	m_dataAccuPeriod;	//the period of data pulse
  Time	m_dataTxStartTime;
  Time	m_revAckAccumTime;
  Time	m_dataAckAccumTime;
  /*length of time slot should be bigger than maximum transmissioin time of packet*/
  //Time TimeSlotLen_;
  PktWareHouse m_PktWH;

  std::map<AquaSimAddress, Time> m_propDelays;  //the propagation delay to neighbors
  std::map<AquaSimAddress, Time> m_ndReceiveTime;   //the time when receives ND packet
  std::map<AquaSimAddress, Time> m_ndDepartNeighborTime;    //the time when neighbor send ND to this node
  std::vector<RevReq*> m_pendingRevs;  //all pending revs are stored here. Schedule it before sending ack
  std::vector<DataAck*> m_pendingDataAcks;
  std::map<AquaSimAddress, int> m_sucDataNum;   //result is here

  RevQueues m_RevQ;
  uint m_nextHop;
  uint m_neighborId;
  //Time MajorBackupInterval;
  //int MaxSlotRange_;  //the interval between majorInterval and backup

  //the begin time of MajorInterval will be chosen among the following two values
  //they are the offset based on current time
  Time m_majorIntervalLB;  //lower bound of major interval//RecvSlotLowerRange;

  Time m_dataStartTime;
  static int RevID;
  Time m_guardTime;
  double m_NDWin;
  Time m_NDReplyWin;
  std::map<AquaSimAddress, NDRecord> m_PendingND;

  std::map<int, AckWaitTimer> m_AckWaitingList; //stores the packet is (prepared to) sent out but not receive the ack yet.

  Time m_ackTimeOut;
  std::queue<Timer> m_ctrlQ;
  int m_pktSize;
  int m_isParallel;
  int m_NDProcessMaxTimes; //the maximum times of delay measurement
  int m_backoffCounter;
  Ptr<Packet> m_backoffPkt;
  Ptr<UniformRandomVariable> m_rand;

  friend class RevQueues;
  friend class PktSendTimer;
  friend class AckWaitTimer;

};  // class AquaSimCopeMac


} // namespace ns3

#endif /* AQUA_SIM_MAC_COPEMAC_H */
