/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Moshin Raza Jafri <mohsin.jafri@unive.it>
 */

#ifndef AQUA_SIM_ROUTING_DDBR_H
#define AQUA_SIM_ROUTING_DDBR_H

#include "aqua-sim-routing.h"
#include "aqua-sim-routing-vbf.h"
#include "aqua-sim-routing-dbr.h"
#include "aqua-sim-address.h"

#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"
#include "ns3/mobility-model.h"
#include "ns3/vector.h"
#include "ns3/timer.h"
#include <deque>


#define DBR_PORT    0xFF

#define DBR_BEACON_DESYNC 0.1   // desynchronizing form for alive beacons
#define DBR_BEACON_INT    10    // interval between beacons
#define DBR_JITTER      1   // jitter for broadcasting
#define IP_HDR_LEN      20

namespace ns3 {

// Vectorbasedforward  Entry
// Vectorbasedforward Agent
class AquaSimDDBR;
class AquaSimAddress;
class Packet;
class Time;
class MNeighbEnt;
class MNeighbTable;

// class AquaSimDDBR : public Object {
//   // friend class DBR_BeaconTimer;
//   friend class DDBR_SendingTimer;

// public:
//   AquaSimDDBR();
//   virtual ~AquaSimDDBR();
//   static TypeId GetTypeId(void);

//   virtual bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
//   virtual bool Recv2(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);

//   //virtual void Tap(const Ptr<Packet> p);
//   void Send_Callback(void);

// protected:
//   //PortClassifier *dmux;

//   double m_bDesync; // desynchronizing term

//   double m_latest;    // latest time to send the packet

//   //NsObject *port_dmux;
//   DDBR_SendingTimer *m_sendTimer;    // sending timer

//   ASSPktCache *m_pc;       // packet cache for broadcasting;
//   MMyPacketQueue m_pq;     // packet queue
//   int m_pktCnt;       // counter for packets have been sent
//   Ptr<UniformRandomVariable> m_rand;

//   // void ForwardPacket(Ptr<Packet>, int = 0);
//   void HandlePktForward(Ptr<Packet> p);

// };

class DDBR_SendingTimer : public Timer {
public:
  DDBR_SendingTimer(AquaSimDDBR *a) :
  Timer(Timer::CANCEL_ON_DESTROY), m_a(a) {}
  void Expire();
private:
  AquaSimDDBR *m_a;
};  // class DBR_SendingTimer

class ASSPktCache {
public:
	ASSPktCache();
	~ASSPktCache();
  static TypeId GetTypeId(void);

	int& Size(void)
	{ return m_size; }

	int AccessPacket(int pid);
	void AddPacket(int pid);
	void DeletePacket(int pid);
//	void Dump(void);

private:
	int *m_pCache;			// packet cache
	int m_size;			// cache size
	int m_maxSize;			// max cache size
};  // class ASPktCache


class MNeighbEnt{
public:
  MNeighbEnt(/*AquaSimDBR* ina*/) : m_routeFlag(0)
    {
      m_location = Vector();
    }
  // the agent is used for timer object

  Vector m_location;    // location of neighbor, actually we only need depth info
  AquaSimAddress m_netID;    // IP of neighbor
  int m_routeFlag;    // indicates that a routing path exists

  // user timer
  //DBR_DeadNeighbTimer dnt;  // timer for expiration of neighbor
};  // class NeighbEnt


class MNeighbTable : public Object {
public:
  MNeighbTable(/*Ptr<AquaSimDBR> a*/);
  ~MNeighbTable();
  static TypeId GetTypeId(void);

  void Dump(void);
  void EntDelete(const MNeighbEnt *e);         // delete an neighbor
  MNeighbEnt *EntAdd(const MNeighbEnt *e);      // add an neighbor
  MNeighbEnt *EntFindShadowest(Vector location);  // find the neighbor with minimal depth
  void UpdateRouteFlag(AquaSimAddress, int);

private:
  int m_numEnts;       // number of entries in use
  int m_maxEnts;       // capacity of the table
  //Ptr<AquaSimDBR> m_a;       // agent owns the table
  MNeighbEnt **m_tab;    // neighbor table
};  // class NeighbTable


class QqueueItem : public Object {
public:
	QqueueItem() : m_sendTime(0), pcket(0) {}
	QqueueItem(Ptr<Packet> p, double t, int pckett ) : m_p(p), m_sendTime(t), pcket(pckett) {}

	Ptr<Packet> m_p;		// pointer to the packet
	double m_sendTime;	// time to send the packet
  int pcket;
};  // class QueueItem

class MMyPacketQueue : public Object {
public:
	MMyPacketQueue() : m_dq() {}
	~MMyPacketQueue() { m_dq.clear(); }

	bool empty() { return m_dq.empty(); }
	int size() { return m_dq.size(); }
	void dump();

	void pop() { m_dq.pop_front(); };
	QqueueItem* front() { return m_dq.front(); };
	void insert(QqueueItem* q);
  void inserting(QqueueItem* q);
	bool update(Ptr<Packet> p, double t, int pckett );
	bool purge(Ptr<Packet> p);
  bool purgenow(Ptr<Packet> p, double t);
  void print();
  void printall();

private:
	std::deque<QqueueItem*> m_dq;
};  // class MMyPacketQueue

class AquaSimDDBR : public AquaSimRouting {

    friend class DDBR_SendingTimer;

 public:
  AquaSimDDBR();
  virtual ~AquaSimDDBR();
  static TypeId GetTypeId(void);
  virtual bool Recv(Ptr< Packet > packet, const Address &dest, uint16_t protocolNumber);

  virtual bool Recv1(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
  // virtual bool Recv2(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
  //virtual void Tap(const Ptr<Packet> p);
  void Send_Callback(void);
  void DeadNeighb_Callback(MNeighbEnt *ne);

 protected:
  int m_pkCount;
  double cum_time;
  double tot_pkt;
  double new_pkt;
  // int port_number;
  AquaSimPktHashTable PktTable;
  /*AquaSimPktHashTable SourceTable;
  AquaSimPktHashTable Target_discoveryTable;
  AquaSimPktHashTable SinkTable;*/    //not used...
  //UWDelayTimer delaytimer;

  void Terminate();
  // void Reset();
  void ConsiderNew(Ptr<Packet> pkt);
  Ptr<Packet> CreatePacket();
  Ptr<Packet> PrepareMessage(unsigned int dtype, AquaSimAddress addr, int msg_type);

  void DataForSink(Ptr<Packet> pkt);
  void StopSource();
  void MACprepare(Ptr<Packet> pkt);
  void MACsend(Ptr<Packet> pkt, Time delay=Seconds(0));

  // double cum_time;
  double m_bDesync; // desynchronizing term
  double m_latest;    // latest time to send the packet
  MNeighbTable *m_nTab;      // neighbor entry table
  DDBR_SendingTimer *m_sendTimer;    // sending timer
  ASSPktCache *m_pc;       // packet cache for broadcasting;
  MMyPacketQueue m_pq;     // packet queue
  int m_pktCnt;       // counter for packets have been sent
  Ptr<UniformRandomVariable> m_rand;
  // void ForwardPacket(Ptr<Packet>, int = 0);
  void HandlePktForward(Ptr<Packet> p);

};


} // namespace ns3

#endif /* AQUA_SIM_ROUTING_DDBR_H */
