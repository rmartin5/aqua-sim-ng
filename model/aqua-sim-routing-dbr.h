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

/**
 *  Depth based routing alogrithm header file.
 *  Author: Hai Yan, University of Connecticut CSE, March, 2007
 */

#ifndef	AQUA_SIM_ROUTING_DBR_H
#define	AQUA_SIM_ROUTING_DBR_H

#include "aqua-sim-routing.h"
#include "aqua-sim-address.h"

#include "ns3/vector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"
#include "ns3/mobility-model.h"
#include "ns3/vector.h"
#include "ns3/timer.h"

#include <deque>

#define	DBR_PORT		0xFF

#define	DBR_BEACON_DESYNC	0.1		// desynchronizing form for alive beacons
#define	DBR_BEACON_INT		10		// interval between beacons
#define	DBR_JITTER			1		// jitter for broadcasting
#define IP_HDR_LEN      20

namespace ns3 {

class AquaSimDBR;
class MyPacketQueue;
class NeighbEnt;
class NeighbTable;
//class DBRHeader;

#if	0
struct DBRPacket {
	int dest;
	int src;
	Packet *pkt;	// inner NS packet

	DBRPacket() : pkt(NULL) {}
	DBRPacket(Packet *p, DBRHeader *dbrh) :
			pkt(p) {}
};
#endif

class DBR_BeaconTimer : public Timer {
public:
	DBR_BeaconTimer(AquaSimDBR *a) :
    Timer(Timer::CANCEL_ON_DESTROY), m_a(a) {}
  void Expire();
private:
  AquaSimDBR *m_a;
};  // class DBR_BeaconTimer

class DBR_SendingTimer : public Timer {
public:
	DBR_SendingTimer(AquaSimDBR *a) :
    Timer(Timer::CANCEL_ON_DESTROY), m_a(a) {}
	void Expire();
private:
  AquaSimDBR *m_a;
};  // class DBR_SendingTimer

class QueueItem : public Object {
public:
	QueueItem() : /*m_p(0),*/ m_sendTime(0) {}
	QueueItem(Ptr<Packet> p, double t) : m_p(p), m_sendTime(t) {}

	Ptr<Packet> m_p;		// pointer to the packet
	double m_sendTime;	// time to send the packet
};  // class QueueItem

class MyPacketQueue : public Object {
public:
	MyPacketQueue() : m_dq() {}
	~MyPacketQueue() { m_dq.clear(); }

	bool empty() { return m_dq.empty(); }
	int size() { return m_dq.size(); }
	void dump();

	void pop() { m_dq.pop_front(); };
	QueueItem* front() { return m_dq.front(); };
	void insert(QueueItem* q);
	bool update(Ptr<Packet> p, double t);
	bool purge(Ptr<Packet> p);

private:
	std::deque<QueueItem*> m_dq;
};  // class MyPacketQueue

class NeighbEnt{
public:
	NeighbEnt(/*AquaSimDBR* ina*/) : m_routeFlag(0)
    {
      m_location = Vector();
    }
	// the agent is used for timer object

	Vector m_location;		// location of neighbor, actually we only need depth info
	AquaSimAddress m_netID;    // IP of neighbor
	int m_routeFlag;		// indicates that a routing path exists

	// user timer
	//DBR_DeadNeighbTimer dnt;	// timer for expiration of neighbor
};  // class NeighbEnt

class NeighbTable : public Object {
public:
	NeighbTable(/*Ptr<AquaSimDBR> a*/);
	~NeighbTable();
	static TypeId GetTypeId(void);

	void Dump(void);
	void EntDelete(const NeighbEnt *e);        	// delete an neighbor
	NeighbEnt *EntAdd(const NeighbEnt *e);     	// add an neighbor
	NeighbEnt *EntFindShadowest(Vector location);  // find the neighbor with minimal depth
	void UpdateRouteFlag(AquaSimAddress, int);

private:
	int m_numEnts;       // number of entries in use
	int m_maxEnts;       // capacity of the table
	//Ptr<AquaSimDBR> m_a;       // agent owns the table
	NeighbEnt **m_tab;    // neighbor table
};  // class NeighbTable


class ASPktCache {
public:
	ASPktCache();
	~ASPktCache();
  static TypeId GetTypeId(void);

	int& Size(void)
	{ return m_size; }

	int AccessPacket(int pid);
	void AddPacket(int pid);
	void DeletePacket(int pid);
	void Dump(void);

private:
	int *m_pCache;			// packet cache
	int	m_size;					// cache size
	int m_maxSize;				// max cache size
};  // class ASPktCache

class AquaSimDBR : public AquaSimRouting {
	friend class DBR_BeaconTimer;
	friend class DBR_SendingTimer;

public:
	AquaSimDBR();
	virtual ~AquaSimDBR();
  static TypeId GetTypeId(void);

	virtual bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
	virtual bool Recv2(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);

	//virtual void Tap(const Ptr<Packet> p);

	void DeadNeighb_Callback(NeighbEnt *ne);
	void Beacon_Callback(void);
	void Send_Callback(void);

protected:
	//PortClassifier *dmux;

	double m_bInt;		// beacon interval
	double m_bDesync;	// desynchronizing term

	double m_latest;		// latest time to send the packet

	//NsObject *port_dmux;
	NeighbTable *m_nTab;			// neighbor entry table
	DBR_BeaconTimer *m_beaconTimer;		// beacon timer
	DBR_SendingTimer *m_sendTimer;		// sending timer

	ASPktCache *m_pc;				// packet cache for broadcasting;
	MyPacketQueue m_pq;			// packet queue
	int m_pktCnt;				// counter for packets have been sent
  Ptr<UniformRandomVariable> m_rand;

	void ForwardPacket(Ptr<Packet>, int = 0);
	Ptr<Packet> MakeBeacon(void);
	void SendBeacon(void);
	void BeaconIn(Ptr<Packet>);

	void HandlePktForward(Ptr<Packet> p);

};

}  // namespace ns3

#endif	/* AQUA_SIM_ROUTING_DBR_H */
