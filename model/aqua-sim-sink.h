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

#ifndef AQUA_SIM_SINK_H
#define AQUA_SIM_SINK_H

#include <set>

#include "ns3/object.h"
#include "ns3/address.h"
#include "ns3/random-variable-stream.h"
#include "ns3/timer.h"

#include "aqua-sim-hash-table.h"
#include "aqua-sim-node.h"
#include "aqua-sim-phy.h"


// Aqua Sim Sink

/*
 *
 *
 * This is redundant since AquaSimNode can act as a sink...
 * 	Plus node inherits mobility model allowing easier vector positioning.
 * 	Could instead integrate some additional features from here into AquaSimNode.
 * 	Also could implement bound checking within the Mobility Model or AquaSimNode.
 *
 * 	Remove/rethink this work in future iterations.
 */

/*
 * *** CURRENTLY NOT IN USE
 */

namespace ns3{

struct SenseAreaElem{
  double senseX;
  double senseY;
  double senseZ;
  double senseR;
  SenseAreaElem(double x, double y, double z, double r) {
    senseX = x;
    senseY = y;
    senseZ = z;
    senseR = r;
  }

  friend bool operator<(const SenseAreaElem&  e1, const SenseAreaElem& e2);
};

class Packet;

class SenseArea{
private:
  std::set<SenseAreaElem> AreaSet;
public:
  bool IsInSenseArea(double nx, double ny, double nz);
  void Insert(double x, double y, double z, double r);
}; // class SenseArea


// Class SinkAgent as source and sink for directed diffusion

class AquaSimSink : public Object {

public:
  AquaSimSink(void);
  virtual ~AquaSimSink(void);
  static TypeId GetTypeId(void);

  virtual void Timeout(int);

  void Report(void);
  void Recv(Ptr<Packet>);  //handler not implemented
  void Reset(void);
  void SetAddr(Address address);
  int GetPktCount(void);
  void IncrPktCount(void);
  Ptr<Packet> CreatePacket(void);

protected:
  bool m_AppDup;
  bool m_periodic;
  static int m_pktId;
  //bool m_alwaysMaxRate;
  int m_pktCount;
  //  unsigned int m_dataType;
  int m_numRecv;
  int m_numSend;
  // int m_recvPerSec; //? what's this for

  /*used ti indicate if the sink is active, send out interest first or
  passive, it gets the data ready and then sends out the interest. 1 is passive
  and 0 is active.*/

  int m_passive;

  double m_targetX;
  double m_targetY;
  double m_targetZ;
  double m_range;

  int m_activeSense;
  SenseArea SenseAreaSet;
  double m_senseInterval;

  Address m_targetId;
  Address m_here;	//address of this sink

  char   m_fName[80];

  //the monitoring area. nodes within this area will send
  double m_senseX;
  double m_senseY;
  double m_senseZ;
  double m_senseR;

  Ptr<AquaSimNode> m_node;

  double m_cumDelay;
  double m_lastArrivalTime;

  Ptr<AquaSimHashTable>  DataTable;

  bool IsDeviation(void);
  void Terminate(void);
  void BcastInterest(void);
  void SourceDeny(uint32_t, double, double, double);
  void DataReady(void);
  void Start(void);
  void GenerateInterval(void);
  void ExponentialStart(void);
  void Stop(void);
  virtual void SendPkt(void);

  int m_running;
  int m_random;   //1 is random; 2 is exponential distribution
  int m_maxPkts;

  double m_interval; // interval to send data pkt
  double m_exploreInterval;
  double m_dataInterval;
  double  m_dataRate;

  int m_packetSize;  // # of bytes in the packet
  int m_exploreRate;
  int m_dataCounter;
  int  m_exploreCounter;
  int m_exploreStatus;

  Ptr<UniformRandomVariable> m_uniformRand;

  //int simple_report_rate;
  //  int data_counter;

  // Timer handlers
  Timer m_sinkTimer;
  Timer m_periodicTimer;
  Timer m_reportTimer;

  Ptr<AquaSimPhy> m_phy;

};  // class AquaSimSink

}  // namespace ns3

#endif /* AQUA_SIM_SINK_H */
