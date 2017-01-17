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
* 96Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
* Author: Robert Martin <robert.martin@engr.uconn.edu>
*/


#ifndef AQUA_SIM_ROUTING_DDOS_H
#define AQUA_SIM_ROUTING_DDOS_H

#include "aqua-sim-routing.h"
//#include "aqua-sim-routing-vbf.h"

#include "ns3/random-variable-stream.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"
#include "ns3/vector.h"
#include "ns3/event-id.h"

#include <map>
#include <sstream>
#include <string>
#include <queue>
#include <set>


namespace ns3 {


struct DdosTable{
  int nodeID;
  Vector nodePosition;  //not currently used but here for mimicing the algorithm
  int NAck;             //channel measurement
  int pktRecv;          //transmittion dominance
  int interestEntries;  //timeout ratio
  int entriesTimeout;   //timeout ratio
  EventId pushback;
  EventId throttle;
  double timeoutThreshold;
  double maxThreshold;
  double thresholdOffset;
  DdosTable(int node) : nodeID(node), NAck(0), pktRecv(0),
                interestEntries(0), entriesTimeout(0),
                //pushback(Timer::CANCEL_ON_DESTROY),
                //throttle(Timer::CANCEL_ON_DESTROY),
                timeoutThreshold(0.5), maxThreshold(0.3),
                thresholdOffset(0.0) {}
};

struct PIT_info{
  std::set<int> nodeID ;  //can have multiple requestors of same data
  Timer entryTimeout;

  PIT_info(std::set<int> nodeIDs):
    nodeID(nodeIDs), entryTimeout(Timer::CANCEL_ON_DESTROY) {}
};

struct MachineLearningStruct{
  //nominal scores for all fields.
  double mobility;  //mobility occurance
  double pushback;  //pushback percentage to threshold
  double throttle;  //throttle percentage to threshold
  double timeout;
};

class Packet;
class Address;

/**
 * \ingroup aqua-sim-ng
 *
 * \brief   AquaSimDDOS is implemented to detect and throttle distributed
 *   denial of service (DDoS) attacks in a spread and focused attack style.
 *   Emulated to mimic certain components of Named Data Network (NDN),
 *   particularly naming prefix of data, interest/data packets,
 *   and Pending Interest Table (PIT)
 */
class AquaSimDDOS : public AquaSimRouting {
public:
  AquaSimDDOS();
  ~AquaSimDDOS() {};
  static TypeId GetTypeId(void);
  virtual bool Recv(Ptr< Packet > packet, const Address &dest, uint16_t protocolNumber);
  void RecvNAck(Ptr<Packet> p);

  void SetThresholds(double timeout, double max);
protected:
  virtual void DoDispose();

private:
  void SendNAck(Ptr<Packet> p);
  void SendInterest(Ptr<Packet> p);
  void SendData(Ptr<Packet> p);
  void SendAlert(int attackerAddr);
  void DdosAttackCheck();
  void MachineLearning();
  void Pushback(int nodeID);
  void Throttle(int nodeID);
  void ResetPushback(int nodeID);
  void ResetThrottle(int nodeID);
  bool NodeContainsRelatedData(std::string interest);
  bool NodeContainsDataPath(std::string interest);
  void RemoveEntry(std::string interest, bool isTimeout);
  double Normalize(double v1, double v2);
  void UpdateInterest();

  //void FilePrintout(bool pushback, int nodeID, int attackerID);  //remove & replace with tracers

  std::map<std::string, PIT_info> PIT;  //Pending Interest Table
  std::map<int, DdosTable> DdosDetectionTable;
  Time m_pitEntryTimeout;
  int m_totalPktSent; //channel measurement
  int m_totalPktRecv; //transmission dominance

  Time m_ddosCheckFrequency;
  Time m_pushbackLength;
  Time m_throttleLength;

  Ptr<UniformRandomVariable> m_rand;
  //int m_dataCacheSize;

  std::ostringstream m_interestMsg;
  std::string m_baseInterest;         //simulate base name of interest
  int m_interestVersionNum;   //simulate interest version being requested
  bool isAttacker;
  std::vector<std::string> m_possibleBaseInt;

  double m_timeoutThreshold;  // for reset purposes
  double m_maxThreshold;  // for reset purposes
  double m_pushbackReduction;
  double m_throttleReduction;

  //machine learning componenents
  typedef std::queue<MachineLearningStruct> ml;
  std::map<int, ml> StatLearningTable;
  std::map<int, ml> RuleMiningTable;
  Time m_statisticalFreq;  //statistical distribution model frequency
  int m_ruleMiningFreq;   //rules mining model frequency, based on statisticalFreq occurance
  int m_statisticalIteration;

}; // class AquaSimDDOS

} // namespace ns3

#endif /* AQUA_SIM_ROUTING_DDOS_H */
