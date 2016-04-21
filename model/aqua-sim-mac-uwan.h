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

#ifndef AQUA_SIM_MAC_UWAN_H
#define AQUA_SIM_MAC_UWAN_H

#include "ns3/timer.h"
#include "ns3/nstime.h"

#include "aqua-sim-address.h"
#include "aqua-sim-mac.h"
#include "aqua-sim-channel.h"

#include <set>
#include <queue>

#define UWAN_CALLBACK_DELAY 0.001
#define INFINITE_PERIOD   10000000.0
#define PRE_WAKE_TIME     0.1

namespace ns3{

class AquaSimUwan;
struct ScheduleTime;

class AquaSimUwan_WakeTimer: public Timer {
friend class AquaSimUwan;
friend class ScheduleQueue;
public:
AquaSimUwan_WakeTimer(Ptr<AquaSimUwan> mac, ScheduleTime* ScheT)
  : Timer(Timer::CANCEL_ON_DESTROY)
{
  m_mac = mac;
  m_ScheT = ScheT;
}

protected:
	Ptr<AquaSimUwan> m_mac;
	ScheduleTime* m_ScheT;
	void expire();
};

/*
 * Data structure for neighbors' schedule
 */

struct ScheduleTime {
	ScheduleTime* next_;
	Time		SendTime_;
	AquaSimAddress	nodeId_;  //with this field, we can determine that which node will send packet.
	AquaSimUwan_WakeTimer	timer_;  //necessary
	//Ptr<Packet>  pkt_;    //the packet this node should send out

	ScheduleTime(Time SendTime, AquaSimAddress node_id, Ptr<AquaSimUwan> mac):
			next_(NULL), SendTime_(SendTime), nodeId_(node_id), timer_(mac, this) {
	}

	~ScheduleTime() {}

	void Start(Time Delay) {
		if( Delay.IsPositive() || Delay.IsZero() )
		  timer_.Schedule(Delay);
	}

};


/*The SendTime in SYNC should be translated to absolute time
 *and then insert into ScheduleQueue
 */
class ScheduleQueue{
private:
	ScheduleTime* m_head;
	Ptr<AquaSimUwan> m_mac;
public:
	ScheduleQueue(Ptr<AquaSimUwan> mac): m_mac(mac) {
		m_head = new ScheduleTime(Seconds(0), AquaSimAddress(), NULL);
	}

	~ScheduleQueue() {
		ScheduleTime* tmp;
		while( m_head != NULL ) {
			tmp = m_head;
			m_head = m_head->next_;
			delete tmp;
		}
	}

	static TypeId GetTypeId(void);

public:
	void Push(Time SendTime, AquaSimAddress node_id, Time Interval);  //first parameter is the time when sending next packet, the last one is the time interval between current time and sending time
	ScheduleTime* Top();		//NULL is returned if the queue is empty
	void Pop();
	bool CheckGuardTime(Time SendTime, Time GuardTime, Time MaxTxTime); //the efficiency is too low, I prefer to use the function below
	Time GetAvailableSendTime(Time StartTime, Time OriginalSchedule, Time GuardTime, Time MaxTxTime);
	void ClearExpired(Time CurTime);
	void Print(Time GuardTime, Time MaxTxTime, bool IsMe, AquaSimAddress index);
};



class AquaSimUwan_SleepTimer: public Timer {
friend class AquaSimUwan;
public:
	AquaSimUwan_SleepTimer(Ptr<AquaSimUwan> mac): Timer(Timer::CANCEL_ON_DESTROY) {
		m_mac = mac;
	}
protected:
	Ptr<AquaSimUwan> m_mac;
	void expire();
};


class AquaSimUwan_PktSendTimer: public Timer {
friend class AquaSimUwan;
public:
	AquaSimUwan_PktSendTimer(Ptr<AquaSimUwan> mac): Timer(Timer::CANCEL_ON_DESTROY) {
		m_mac = mac;
	}

  void SetTxTime(Time txTime) {
    m_txTime = txTime;
  }
	Time GetTxTime() {
		return m_txTime;
	}

	//Ptr<Packet>& pkt() {
	//	return m_p;
	//}
public:
	Ptr<Packet> m_p;
protected:
	Ptr<AquaSimUwan> m_mac;
	Time	m_txTime;
	void expire();
};


class AquaSimUwan_StartTimer: public Timer {
friend class AquaSimUwan;
public:
	AquaSimUwan_StartTimer(Ptr<AquaSimUwan> mac): Timer(Timer::CANCEL_ON_DESTROY) {
		m_mac = mac;
	}

protected:
	Ptr<AquaSimUwan> m_mac;
	void expire();
};


class AquaSimUwan: public AquaSimMac{
	friend class AquaSimUwan_CallbackHandler;
	friend class AquaSimUwan_WakeTimer;
	friend class AquaSimUwan_SleepTimer;
	friend class AquaSimUwan_StatusHandler;
	friend class AquaSimUwan_PktSendTimer;
	friend class AquaSimUwan_StartTimer;
	friend class AquaSimUwan_TxStatusHandler;
	//friend AquaSimUwan_SendPktTimer;

public:
	AquaSimUwan();
  ~AquaSimUwan();
  static TypeId GetTypeId(void);

	virtual  bool RecvProcess(Ptr<Packet>);
	/*
	 * UWAN MAC assumes that node knows when it will send out the next packet.
	 * To simulate such pre-knowledge, we should not send out the outgoing packet in TxProcess(),
	 * but just queue it and then send out it according to the Schedule via SendoutPkt().
	 */
	virtual  bool TxProcess(Ptr<Packet>);

protected:
	void	SendFrame(Ptr<Packet> p, bool IsMacPkt, Time delay = Seconds(0.0));
	void	TxPktProcess(AquaSimUwan_PktSendTimer* pkt_send_timer);


//	AquaSimUwan_PktSendTimer	pkt_send_timer;

	//AquaSimUwan_WakeTimer wake_timer;	//wake this node after NextCyclePeriod;
	AquaSimUwan_SleepTimer		m_sleepTimer;
	AquaSimUwan_StartTimer		m_startTimer;

	Ptr<Packet> MakeSYNCPkt(Time CyclePeriod, AquaSimAddress Recver = AquaSimAddress::GetBroadcast()); //perhaps CyclePeriod is not required
	Ptr<Packet> FillMissingList(Ptr<Packet> p);
	Ptr<Packet> FillSYNCHdr(Ptr<Packet> p, Time CyclePeriod);

	void	Wakeup(AquaSimAddress node_id);  //perhaps I should calculate the energy consumption in these two functions
	void	Sleep();
	void	SendoutPkt(Time NextCyclePeriod);
	//bool	setWakeupTimer(); //if the node still need to keep wake, return false.
	void	SetSleepTimer(Time Interval);     //keep awake for To, and then fall sleep
	void	Start();	//initilize NexCyclePeriod_ and the sleep timer, sendout first SYNC pkt
	Time	GenNxCyclePeriod();   //I want to use normal distribution
	void	ProcessMissingList(uint8_t *data, AquaSimAddress src);
	void	SYNCSchedule(bool initial = false);

	void	SendInfo();


private:
	std::set<AquaSimAddress> m_CL;				//contact list
	std::set<AquaSimAddress> m_neighbors;		//neighbor list.
	/*the difference between m_CL and m_neighbors is the Missing list*/
	Time  m_nextCyclePeriod;		//next sending cycle
	Time  m_avgCyclePeriod;
	Time  m_stdCyclePeriod;

	/*  The length of initial cycle period, whenever I send current
	 *  packet, I should first decide when I will send out next one.
	 */
	static Time  m_initialCyclePeriod;
	static Time  m_listenPeriod;		//the length of listening to the channel after transmission.
	static Time  m_maxTxTime;
	static Time  m_maxPropTime;		//GuardTime_;		//2* static Time  m_maxPropTime;
	static Time  m_helloTxLen;
	static Time  m_wakePeriod;

	ScheduleQueue	m_wakeSchQueue;
	/*
	 *packet queue which this node cache the packet from upper layer
	 */
	std::queue<Ptr<Packet> >  m_packetQueue;
	int		m_cycleCounter;   //count the number of cycle.
	int		m_numPktSend;
	uint		m_nextHopNum;
	std::set<AquaSimUwan_PktSendTimer *> m_pktSendTimerSet;

  Ptr<UniformRandomVariable> m_rand;
  double m_bitRate;
  double m_encodingEfficiency;

}; // AquaSimUwan

}  //namespace ns3


#endif  /* AQUA_SIM_MAC_UWAN_H */
