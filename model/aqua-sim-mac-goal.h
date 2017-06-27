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

#ifndef AQUA_SIM_MAC_GOAL_H
#define	AQUA_SIM_MAC_GOAL_H

/***********************************************************
 * This is a geo-routing MAC designed in cross-layer approach
 * It smoonthly integrates VBF and handshake to avoid collisions
 * and guarantee a better reliability
 * The corresponding conference paper is
 @inproceedings{Zhu2010co:geomac,
	author={Yibo Zhu and Zhong Zhou and Peng Zheng and Jun-Hong Cui},
	title={{An Efficient Geo-Routing Aware MAC Protocol for Underwater Acoustic Networks (Invited Paper)}},
	year= {2010},
	booktitle= {Proc. ADHOCNETS},
	pages= {185--200},
	address= {Victoria, BC, Canada}
 }

 The Journal verion is
 @article{Zhu2011at:effic,
	author={Yibo Zhu and Zhong Zhou and Zheng Peng and Jun-Hong Cui},
	title={{An Efficient Geo-routing Aware MAC Protocol for Underwater Acoustic Networks}},
	year= {2011},
	journal= {ICST Transactions on Mobile Communications and Applications},
	pages= {1--14},
	number= {7--9},
	volume= {11}
 }

 */


#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"
#include "ns3/timer.h"
#include "ns3/nstime.h"

#include "aqua-sim-mac.h"
#include "aqua-sim-header-goal.h"
#include "aqua-sim-address.h"

#include <deque>
#include <set>
#include <map>
#include <list>

#define GOAL_CALLBACK_DELAY	0.001

namespace ns3{

class AquaSimGoal;

struct AquaSimGoal_PktQ{
	int BurstNum;
	std::deque<Ptr<Packet> >  Q_;

	AquaSimGoal_PktQ(): BurstNum(0) {

	}
};

struct SchedElem{
	Time BeginTime;
	Time EndTime;
	bool IsRecvSlot;
	SchedElem(Time BeginTime_, Time EndTime_, bool IsRecvSlot_=false);
	SchedElem(SchedElem& e);
};

//---------------------------------------------------------------------
/**
 * \ingroup aqua-sim-ng
 *
 * \brief Helper timer for GOAL
 */
class AquaSimGoal_PreSendTimer: public Timer{
public:
	AquaSimGoal_PreSendTimer() {}
	~AquaSimGoal_PreSendTimer();
	AquaSimGoal_PreSendTimer(AquaSimGoal* mac): Timer(), mac_(mac) {
	}

	Ptr<Packet>&	Pkt() {
		return m_pkt;
	}

protected:
	AquaSimGoal*		mac_;
	Ptr<Packet>		m_pkt;
	void expire();
	friend class AquaSimGoal;
};

//---------------------------------------------------------------------
/**
* \brief Helper timer for GOAL
*/
class AquaSimGoal_BackoffTimer: public Timer{
public:
	AquaSimGoal_BackoffTimer() {}
	~AquaSimGoal_BackoffTimer();
	AquaSimGoal_BackoffTimer(AquaSimGoal* mac): Timer(), mac_(mac) {
	}

	Ptr<Packet>	ReqPkt() {
		return m_ReqPkt;
	}
	Time BackoffTime() {
		return m_BackoffTime;
	}
	SchedElem* SE() {
		return m_SE;
	}

	void SetSE(SchedElem* SE) {
		m_SE = SE;
	}

protected:
	AquaSimGoal*		mac_;
	Ptr<Packet>		m_ReqPkt;
	SchedElem*	m_SE;
	Time		m_BackoffTime;
	void expire();
	friend class AquaSimGoal;
};


//---------------------------------------------------------------------
/**
* \brief Helper timer for GOAL
*/
class AquaSimGoal_AckTimeoutTimer: public Timer{
public:
	AquaSimGoal_AckTimeoutTimer() {}
	~AquaSimGoal_AckTimeoutTimer();
	AquaSimGoal_AckTimeoutTimer(AquaSimGoal* mac): Timer(), mac_(mac) {
	}

	/*Ptr<Packet>& pkt() {
		return pkt_;
	}*/
	/*Time& SendTime() {
		return SendTime_;
	}*/
	std::map<int, Ptr<Packet> >& PktSet() {
		return m_PktSet;
	}

protected:
	AquaSimGoal*		mac_;
	std::map<int, Ptr<Packet> > m_PktSet; //map uid to packet
	//Ptr<Packet>		pkt_;
	//Time		SendTime_;  //the time when this packet will be sent out
	void expire();
	friend class AquaSimGoal;
};

//---------------------------------------------------------------------
/**
* \brief Helper timer for GOAL
*/
class AquaSimGoal_NxtRoundTimer: public Timer{
public:
	AquaSimGoal_NxtRoundTimer() {}
	~AquaSimGoal_NxtRoundTimer();
	AquaSimGoal_NxtRoundTimer(AquaSimGoal* mac): Timer(), mac_(mac) {
	}

protected:
	AquaSimGoal*		mac_;
	void expire();
	friend class AquaSimGoal;
};

//---------------------------------------------------------------------
/**
* \brief Helper timer for GOAL
*/
class AquaSimGoalDataSendTimer: public Timer{
public:
	AquaSimGoalDataSendTimer() {}
	~AquaSimGoalDataSendTimer();
	AquaSimGoalDataSendTimer(AquaSimGoal* mac): Timer(), mac_(mac) {
		m_MinBackoffTime = Seconds(100000000);
		m_NxtHop = AquaSimAddress();
		m_GotRep = false;
	}

	std::set<Ptr<Packet> >& DataPktSet() {
		return m_DataPktSet;
	}

	AquaSimAddress& NxtHop() {
		return m_NxtHop;
	}

	Time& MinBackoffTime() {
		return m_MinBackoffTime;
	}

	Time& TxTime() {
		return m_TxTime;
	}

	int& ReqID() {
		return m_ReqID;
	}
	void SetReqID(int reqID) { //redundant
	  m_ReqID = reqID;
	}

	bool& GotRep() {
		return m_GotRep;
	}
	void SetRep( bool gotRep) { //redundant
	  m_GotRep = gotRep;
	}

	SchedElem* SE() {
		return m_SE;
	}

	void SetSE(SchedElem* SE) {
		m_SE = SE;
	}

protected:
	AquaSimGoal*	mac_;
	std::set<Ptr<Packet> > m_DataPktSet;
	AquaSimAddress	m_NxtHop;
	Time		m_MinBackoffTime;
	Time		m_TxTime;

	SchedElem*	m_SE;

	int			m_ReqID;
	bool		m_GotRep;
	void expire();
	friend class AquaSimGoal;
};


//---------------------------------------------------------------------
/**
* \brief Helper timer for GOAL
*
* Used for accumulative ACK
*/
class AquaSimGoal_SinkAccumAckTimer: public Timer{
public:
	AquaSimGoal_SinkAccumAckTimer() {}
	~AquaSimGoal_SinkAccumAckTimer();
	AquaSimGoal_SinkAccumAckTimer(AquaSimGoal* mac): Timer(), mac_(mac) {}

	std::set<int>& AckSet() {
		return m_AckSet;
	}


protected:
	AquaSimGoal*		mac_;
	std::set<int>	m_AckSet;
	void expire();

	friend class AquaSimGoal;
};

//---------------------------------------------------------------------
struct RecvedInfo{
	AquaSimAddress	Sender;
	Time		RecvTime;
};

/**
* \brief Helper queue for GOAL
*/
class TimeSchedQueue{
private:
  std::list<SchedElem*> m_SchedQ;
	Time m_minInterval;
	Time m_bigIntervalLen;

public:
	TimeSchedQueue(Time MinInterval, Time BigIntervalLen);
	~TimeSchedQueue();
	SchedElem* Insert(Time BeginTime, Time EndTime, bool IsRecvSlot=false);
	SchedElem* Insert(SchedElem* e);
	void Remove(SchedElem* e);
	//return an available start time for sending packet
	Time GetAvailableTime(Time EarliestTime, Time SlotLen, bool BigInterval = false);
	//true for no collision
	bool CheckCollision(Time BeginTime, Time EndTime);
	void ClearExpiredElems();
	//for test
	void Print(char* filename);
};

//---------------------------------------------------------------------
//check if the destination is same. If same, schedule the data together!

/**
* \brief Geo-rOuting Aware MAC protocoL (GOAL)
*/
class AquaSimGoal: public AquaSimMac{
public:
	AquaSimGoal();
  ~AquaSimGoal();
  static TypeId GetTypeId(void);
	int64_t AssignStreams (int64_t stream);

	// to process the incomming packet
	virtual  bool RecvProcess(Ptr<Packet>);
	// to process the outgoing packet
	virtual  bool TxProcess(Ptr<Packet>);

private:
	int		m_maxBurst;			//maximum number of packets sent in one burst
	Time	m_dataPktInterval;
	Time	m_guardTime;			//to tolerate the mobility and inaccuracy of position information
	Time	m_estimateError;


	bool	m_isForwarding;		//true for initializing a session of forwarding packet
	double	m_propSpeed;		//the speed of propagation
	double	m_txRadius;
	Time	m_maxDelay;		//the maximum propagation delay between two one-hop neighbor
	double	m_pipeWidth;		//for VBF and HH-VBF
	TimeSchedQueue m_TSQ;	//Time schedule queue.
	static	int m_reqPktSeq;
	int		m_dataPktSize;	//the size of data packet, in byte
	int		m_maxRetransTimes;

	/*
	 * which kind of backoff function of existing routing protocol is used, such as HH-VBF
	 */
	BackoffType	m_backoffType;
	AquaSimGoal_SinkAccumAckTimer		SinkAccumAckTimer;
	Time						m_maxBackoffTime;		//the max time for waiting for the reply packet

	Time						m_VBF_MaxDelay;		//predefined max delay for vbf


	std::set<AquaSimGoal_PreSendTimer*>		m_preSendTimerSet;
	std::set<AquaSimGoal_BackoffTimer*>		m_backoffTimerSet;
	//data packet is stored here. It will be inserted into PktSendTimerSet_ after receiving AcK
	std::set<AquaSimGoal_AckTimeoutTimer*>	m_ackTimeoutTimerSet;
	//set<AquaSimGoal_RepTimeoutTimer*>	RepTimeoutTimerSet_;
	std::set<AquaSimGoalDataSendTimer*>	m_dataSendTimerSet;

	std::map<AquaSimAddress, AquaSimGoal_PktQ>  m_PktQs;
	int				m_sinkSeq;     //the packet to which destination should be sent.
	int				m_qsPktNum;    //the number of packets in m_PktQs.
	//map<int, AquaSimGoal_AckTimeoutTimer*>	SentPktSet;

	std::map<int, Time> m_originPktSet;

	std::map<int, RecvedInfo> m_recvedList;	/*the list of recved packet. map uid to Recv info( recv time+sender)
								 *used for preventing from retransmission
								 */
	std::set<int>	m_sinkRecvedList;
	Time	m_recvedListAliveTime;
	AquaSimGoal_NxtRoundTimer		m_nxtRoundTimer;
	Time	m_nxtRoundMaxWaitTime;

	void PurifyRecvedList();


	void PreSendPkt(Ptr<Packet> pkt, Time delay=Seconds(0.000001));  //default delay should be 0, but I am afraid it is stored as a minus value
	void SendoutPkt(Ptr<Packet> pkt);

	void PrepareDataPkts();
	void SendDataPkts(std::set<Ptr<Packet> > DataPktSet, AquaSimAddress NxtHop, Time TxTime);


	Ptr<Packet> MakeReqPkt(std::set<Ptr<Packet> > DataPktset, Time DataSendTime, Time TxTime);	//broadcast. the datapkt is the one which will be sent
	Ptr<Packet> MakeRepPkt(Ptr<Packet> ReqPkt, Time BackoffTime);		//unicast. Make sure that make ack pkt first, then backoff
	Ptr<Packet> MakeAckPkt(std::set<int> AckSet, bool PSH=false, int ReqID=-1);	//only sink uses this ack
	void	ProcessReqPkt(Ptr<Packet> ReqPkt);
	void	ProcessRepPkt(Ptr<Packet> RepPkt);
	void	ProcessDataPkt(Ptr<Packet> DataPkt);
	void	ProcessAckPkt(Ptr<Packet> AckPkt);
	void	ProcessPSHAckPkt(Ptr<Packet> AckPkt);

	void	ProcessPreSendTimeout(AquaSimGoal_PreSendTimer* PreSendTimer);
	//void	processRepTimeout(AquaSimGoal_RepTimeoutTimer* RepTimeoutTimer);
	void	ProcessBackoffTimeOut(AquaSimGoal_BackoffTimer* backoff_timer);
	void	ProcessAckTimeout(AquaSimGoal_AckTimeoutTimer* AckTimeoutTimer);
	void	ProcessDataSendTimer(AquaSimGoalDataSendTimer* DataSendTimer);
	void	ProcessSinkAccumAckTimeout();
	void	ProcessOverhearedRepPkt(Ptr<Packet> RepPkt);
	void	ProcessNxtRoundTimeout();

	void	GotoNxtRound();

	void	Insert2PktQs(Ptr<Packet> DataPkt, bool FrontPush=false);

	//calculate the distance between two nodes.
	double	Dist(Vector Pos1, Vector Pos2);

	//backoff time calculation method
	Time	GetBackoffTime(Ptr<Packet> ReqPkt);
	//VBF backoff time
	//minus value will be returned if this node is out of forward area.
	double	GetVBFbackoffTime(Vector Source, Vector Sender, Vector Sink);
	double	GetHH_VBFbackoffTime(Vector Sender, Vector Sink);

	//distance from this node to line
	double  DistToLine(Vector LinePoint1, Vector LinePoint2);
	Time	JitterStartTime(Time Txtime); //Jitter the start time to avoid collision

	void SetupTransDistance(double range);
	Ptr<UniformRandomVariable> m_rand;
protected:

	friend class AquaSimGoal_CallbackHandler;
	friend class AquaSimGoal_BackoffTimer;
	friend class AquaSimGoal_PreSendTimer;
	//friend class AquaSimGoal_RepTimeoutTimer;
	friend class AquaSimGoal_AckTimeoutTimer;
	friend class AquaSimGoalDataSendTimer;
	friend class AquaSimGoal_SinkAccumAckTimer;
	friend class AquaSimGoal_NxtRoundTimer;

	virtual void DoDispose();
};  // class AquaSimGoal

} // namespace ns3

#endif  /* AQUA_SIM_MAC_GOAL_H */
