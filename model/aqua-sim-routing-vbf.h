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

#ifndef AQUA_SIM_ROUTING_VBF_H
#define AQUA_SIM_ROUTING_VBF_H

#include "aqua-sim-routing.h"
#include "aqua-sim-address.h"
#include "aqua-sim-datastructure.h"
#include "aqua-sim-channel.h"
#include "ns3/vector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"

#include <map>

namespace ns3 {

class VBHeader;

struct vbf_neighborhood{
  int number;
  Vector neighbor[MAX_NEIGHBOR];
};

typedef std::pair<AquaSimAddress, unsigned int> hash_entry;

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Packet Hash table for VBF to assist in specialized tables.
 */
class AquaSimPktHashTable {
public:
  std::map<hash_entry,vbf_neighborhood*> m_htable;
  //std::map<hash_t, hash_entry> m_htable;

  AquaSimPktHashTable() {
    m_windowSize=WINDOW_SIZE;
    //  lower_counter=0;
    // 50 items in the hash table, however, because it begins by 0, so, minus 1
    //Tcl_InitHashTable(&m_htable, 3);
  }
  ~AquaSimPktHashTable();

  int  m_windowSize;
  void Reset();
  void PutInHash(VBHeader * vbh);
  void PutInHash(VBHeader *, Vector *);
  vbf_neighborhood* GetHash(AquaSimAddress senderAddr, unsigned int pkt_num);
//private:
//int lower_counter;
};  // class AquaSimPktHashTable

/**
 * \brief Packet Hash table for VBF to assist in specialized tables.
 */
class AquaSimDataHashTable {
public:
  std::map<int*,int*> m_htable;
  //Tcl_HashTable htable;

  AquaSimDataHashTable() {
    //Tcl_InitHashTable(&htable, MAX_ATTRIBUTE);
  }
  ~AquaSimDataHashTable();

  void Reset();
  void PutInHash(int *attr);
  int* GetHash(int *attr);
};  // class AquaSimDataHashTable

/**
 * \brief Vector Based Forwarding
 * http://engr.uconn.edu/~jcui/UWSN_papers/vbf_networking2006.pdf
 */
class AquaSimVBF : public AquaSimRouting {
public:
  AquaSimVBF();
  static TypeId GetTypeId(void);
  virtual bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);

  // AquaSimVBF_Entry routing_table[MAX_DATA_TYPE];

protected:
  int m_pkCount;
  int m_counter;
  int m_hopByHop;
  int m_enableRouting;   //if true, VBF can perform routing functionality. Otherwise, not perform
  //int m_useOverhear;
  double m_priority;
  bool m_measureStatus;
  // int m_portNumber;
  AquaSimPktHashTable PktTable;
  AquaSimPktHashTable SourceTable;
  AquaSimPktHashTable Target_discoveryTable;
  AquaSimPktHashTable SinkTable;

  double m_width;
  // the width is used to test if the node is close enough to the path specified by the packet
  Ptr<UniformRandomVariable> m_rand;

  void Terminate();
  void Reset();
  void ConsiderNew(Ptr<Packet> pkt);
  void SetDelayTimer(Ptr<Packet>,double);
  void Timeout(Ptr<Packet>);
  double Advance(Ptr<Packet> );
  double Distance(Ptr<Packet> );
  double Projection(Ptr<Packet>);
  double CalculateDelay(Ptr<Packet>, Vector*);
  //double RecoveryDelay(Ptr<Packet>, Vector*);
  void CalculatePosition(Ptr<Packet>);
  void SetMeasureTimer(Ptr<Packet>,double);
  bool IsTarget(Ptr<Packet>);
  bool IsCloseEnough(Ptr<Packet>);


  Ptr<Packet> CreatePacket();
  Ptr<Packet> PrepareMessage(unsigned int dtype, AquaSimAddress to_addr, int msg_type);


  void DataForSink(Ptr<Packet> pkt);
  void StopSource();
  void MACprepare(Ptr<Packet> pkt);
  void MACsend(Ptr<Packet> pkt, double delay=0);

  virtual void DoDispose();
};  // class AquaSimVBF

}  // namespace ns3

#endif /* AQUA_SIM_ROUTING_VBF_H */
