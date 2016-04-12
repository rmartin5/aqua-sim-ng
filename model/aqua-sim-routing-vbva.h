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

#ifndef AQUA_SIM_ROUTING_VBVA_H
#define AQUA_SIM_ROUTING_VBVA_H

#include "aqua-sim-routing.h"
#include "aqua-sim-address.h"
#include "aqua-sim-datastructure.h"
#include "aqua-sim-routing-buffer.h"
#include "ns3/vector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"

#include <math.h>
#include <map>

namespace ns3 {

typedef struct Neighbornode{
  routing_vector    vec;
  Vector3D node;
  unsigned int forwarder_id;
  unsigned int status;

} neighbornode;


typedef struct Neighborhood{
  int number;
  neighbornode neighbor[MAX_NEIGHBOR];
} neighborhood;

typedef std::pair<AquaSimAddress, unsigned int> hash_entry;

class VBHeader;

class AquaSimVBVAPktHashTable {
public:
  std::map<hash_entry,neighborhood*> m_htable;

  AquaSimVBVAPktHashTable() {
    m_windowSize=WINDOW_SIZE;
    //Tcl_InitHashTable(&htable, 3);
  }

  int  m_windowSize;
  void Reset();
  void DeleteHash(VBHeader*); //delete the enrty that has the same key as the new packet
  void DeleteHash(AquaSimAddress, unsigned);

  void MarkNextHopStatus(AquaSimAddress, unsigned int,unsigned int, unsigned int);

  void PutInHash(VBHeader *);
  void PutInHash(VBHeader *, Vector3D *, Vector3D*, Vector3D*, unsigned int=FRESHED);
  neighborhood* GetHash(AquaSimAddress senderAddr, unsigned int pkt_num);
};  // class AquaSimVBVAPktHashTable


class AquaSimVBVADataHashTable {
public:
  std::map<hash_entry,unsigned int*> m_htable;
  //Tcl_HashTable htable;

  AquaSimVBVADataHashTable() {
    //Tcl_InitHashTable(&htable, MAX_ATTRIBUTE);
  }

  void Reset();
  void DeleteHash(AquaSimAddress, unsigned int);
  void PutInHash(AquaSimAddress, unsigned int,unsigned int);
  unsigned int *GetHash(AquaSimAddress, unsigned int);
};  // class AquaSimVBVADataHashTable

/*
 *  Vectorbased Void Avoidance
 */
class AquaSimVBVA : public AquaSimRouting {
public:
  AquaSimVBVA();
  static TypeId GetTypeId(void);
  virtual bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);

  // Vectorbasedforward_Entry routing_table[MAX_DATA_TYPE];

protected:
  double m_positionUpdateTime;
  int m_pkCount;
  int m_counter;
  double m_priority;
  const double m_miniDistance;// distance used for flooding packet delay
  const double m_miniThreshold;// desirablenss used for normal data packet delay
  bool m_measureStatus;  //?? where do I use this?
  int m_controlPacketSize;

  // int port_number;
  AquaSimVBVAPktHashTable PktTable;
  AquaSimVBVAPktHashTable SourceTable;
  AquaSimVBVAPktHashTable Target_discoveryTable;
  AquaSimVBVAPktHashTable SinkTable;

  AquaSimVBVADataHashTable PacketStatusTable;

  //delete later
  AquaSimVBVAPktHashTable CenterPktTable;
  AquaSimVBVAPktHashTable DataTerminationPktTable;

  AquaSimRoutingBuffer m_voidAvoidanceBuffer;
  //  AquaSimRoutingBuffer m_receivingBuffer;

  //Trace *tracetarget;       // Trace Target
  double m_width;
  // the width is used to test if the node is close enough to the path specified by the packet
  Ptr<UniformRandomVariable> m_rand;

  void Terminate();
  void Reset();
  void ConsiderNew(Ptr<Packet>);

  void SetForwardDelayTimer(Ptr<Packet>,double);
  //void SetShiftTimer(AquaSimAddress,int,double);
  void SetShiftTimer(Ptr<Packet>,double);
  // void set_flooding_timer(AquaSimAddress,int,double);
//  void set_flooding_forward_timer(Ptr<Packet>, double);


  void RecordPacket(VBHeader*, unsigned int=FRESHED);
  //  void RecordPacket(hdr_uwvb*);
  void ProcessFloodingTimeout(Ptr<Packet>);// no use right now
  void ProcessSelfcenteredTimeout(Ptr<Packet>);
  void ProcessVoidAvoidanceTimeout(Ptr<Packet>);
  void ProcessBackpressureTimeout(Ptr<Packet>);
  //void ProcessVoidAvoidanceTimeout(AquaSimAddress,Vector3D*,Vector3D*,int);
  void ProcessForwardTimeout(Ptr<Packet>);
  void ProcessBackwardfloodingPacketTimeout(Ptr<Packet>);// no use right

  //void processFloodingPacket(Ptr<Packet>);

  void ProcessBackpressurePacket(Ptr<Packet>);
  void ProcessCenteredPacket(Ptr<Packet>);
  void ProcessBackFloodedPacket(Ptr<Packet>);

  void MakeCopy(Ptr<Packet>);
  void SendFloodingPacket(Ptr<Packet>); // delete this function later
  // void sendVectorShiftPacket(AquaSimAddress, int);
  void SendDataTermination(const Ptr<Packet>);

  double Advance(Ptr<Packet>);
  double Distance(const Ptr<Packet>);
  double Distance(const Vector3D*, const Vector3D*);

  double Projection(Ptr<Packet>);
  double Projection(const Vector3D*, const Vector3D*, const Vector3D *);
  double CalculateMappedDistance(const Vector3D*, const Vector3D*, const Vector3D*);
  double CalculateDelay(Ptr<Packet>, Vector3D*);
  double CalculateDelay(const Vector3D*,const Vector3D*,const Vector3D*, const Vector3D*);

  double CalculateSelfCenteredDelay(const Vector3D*,const Vector3D*, const Vector3D*, const Vector3D*);

  double CalculateFloodingDesirableness(const Ptr<Packet>);// no use right now
  double CalculateDesirableness(const Ptr<Packet>);
  double CalculateBackFloodDelay(const Vector3D*,const Vector3D*, const Vector3D*, const Vector3D*);// no use right now

  Ptr<Packet> GenerateVectorShiftPacket(const AquaSimAddress*, int,const Vector3D*, const Vector3D*);

 Ptr<Packet> GenerateControlDataPacket(Ptr<Packet>,unsigned int);
  //  Ptr<Packet> GenerateBackpressurePacket(const AquaSimAddress*,int);
  Ptr<Packet> GenerateBackpressurePacket(Ptr<Packet>);
  void CalculatePosition(Ptr<Packet>);
  void SetMeasureTimer(Ptr<Packet>,double);

  bool IsStuckNode(const neighbornode*,const Vector3D*,int,unsigned int);
  bool IsWorthFloodingForward(AquaSimAddress,int);// useless??
  //bool IsWorthFloodingForward(AquaSimAddress,int);
  bool IsVoidNode(AquaSimAddress,int,const Vector3D*);
  bool IsUpstreamNode(const Vector3D&,const Vector3D&, const Vector3D&);
  bool IsVoidNode(AquaSimAddress,int);
  bool IsEndNode(AquaSimAddress,int);
  bool IsNewlyTouchedNode(AquaSimAddress, unsigned int);
  bool IsTarget(Ptr<Packet>);
  bool IsCloseEnough(Ptr<Packet>);
  bool IsSamePosition(const Vector3D*, const Vector3D*);
  bool IsControlMessage(const Ptr<Packet>);

//  Packet *create_packet();
//  Packet *prepare_message(unsigned int dtype, AquaSimAddress to_addr, int msg_type);


  void DataForSink(Ptr<Packet> pkt);
  // void StopSource();
  void MACprepare(Ptr<Packet> pkt);
  void MACsend(Ptr<Packet> pkt, double delay=0);

  //void trace(char *fmt,...);
};  // class AquaSimVBVA

} // namespace ns3
#endif  /* AQUA_SIM_ROUTING_VBVA_H */
