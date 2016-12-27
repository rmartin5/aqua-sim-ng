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

#ifndef AQUA_SIM_ROUTING_DYNAMIC_H
#define AQUA_SIM_ROUTING_DYNAMIC_H

#include "aqua-sim-address.h"
#include "aqua-sim-routing.h"

#include "ns3/timer.h"
#include "ns3/random-variable-stream.h"

#include <map>

#define IP_HDR_LEN      20

namespace ns3 {

/***** Dynamic Routing Table ******/
struct DN {
  AquaSimAddress first;
  AquaSimAddress second;
};
//typedef std::map<AquaSimAddress, int> DN;//should be defined
class AquaSimDynamicRouting;
typedef std::map<AquaSimAddress, DN> t_table;//should be defined

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Helper table for dynamic routing protocol.
 */
class AquaSimDynamicRoutingTable {

friend class AquaSimDynamicRouting;
t_table m_rt;
int m_chg;
AquaSimAddress m_nodeId;

public:
 AquaSimDynamicRoutingTable();
 static TypeId GetTypeId(void);

 AquaSimAddress NodeId() {
   return m_nodeId;
  }
 void SetNodeId(AquaSimAddress nodeId)
 {
   m_nodeId = nodeId;
 }
 void Print(AquaSimAddress id);
 void Clear();
 void RemoveEntry(AquaSimAddress);
 //void AddEntry(AquaSimAddress, AquaSimAddress);
 void AddEntry(AquaSimAddress, DN);//add by jun
 void Update(t_table*, AquaSimAddress); // add by jun
 AquaSimAddress Lookup(AquaSimAddress);

 uint32_t Size();
 int IfChg ();
};  // class AquaSimDynamicRoutingTable


/***** Dynamic Routing ******/

/* Timers */

/**
 * \brief Helper timer for dynamic routing protocol.
 */
class AquaSimDynamicRouting_PktTimer : public Timer {
public:
  AquaSimDynamicRouting_PktTimer(AquaSimDynamicRouting* routing, double updateInterval)
   : Timer()
  {
    m_routing = routing;
    m_updateInterval = updateInterval;
  }
  ~AquaSimDynamicRouting_PktTimer();

  double GetUpdateInterval()
  {
    return m_updateInterval;
  }
  void SetUpdateInterval(double updateInterval)
  {
    m_updateInterval = updateInterval;
  }
protected:
  AquaSimDynamicRouting* m_routing;
  double m_updateInterval;
  void Expire();

  friend class AquaSimDynamicRouting;
};

/**
 * \brief Dynamic routing protocol
 */
class AquaSimDynamicRouting : public AquaSimRouting {

friend class AquaSimDynamicRouting_PktTimer;

public:
  AquaSimDynamicRouting();
  AquaSimDynamicRouting(AquaSimAddress);
  static TypeId GetTypeId(void);
  virtual bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
  int m_coun;

protected:
  //PortClassifier* dmux_; // For passing packets up to agents.
  //Trace* logtarget_; // For logging.
  AquaSimDynamicRouting_PktTimer m_pktTimer; // Timer for sending packets.

  inline AquaSimAddress RaAddr() { return m_raAddr; }
  // inline uw_drouting_state& state() { return state_; }
  inline int AccessibleVar() { return m_accessibleVar; };

  void ForwardData(Ptr<Packet>);
  void RecvDRoutingPkt(Ptr<Packet>);
  void SendDRoutingPkt();
  void ResetDRoutingPktTimer();
  double BroadcastJitter(double range);

  virtual void DoDispose();
private:
  AquaSimAddress m_raAddr;
  // uw_drouting_state state_;//?????????define state
  //AquaSimDynamicRoutingTable rtable_;//????????define table
  AquaSimDynamicRoutingTable m_rTable;//add by jun

  int m_accessibleVar;
  uint8_t m_seqNum;
  Ptr<UniformRandomVariable> m_rand;

};  // class AquaSimDynamicRouting

}  // namespace ns3

#endif /* AQUA_SIM_ROUTING_DYNAMIC_H */
