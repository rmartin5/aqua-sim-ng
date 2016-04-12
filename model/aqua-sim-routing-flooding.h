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


#ifndef AQUA_SIM_ROUTING_FLOODING_H
#define AQUA_SIM_ROUTING_FLOODING_H

#include "aqua-sim-routing.h"
#include "aqua-sim-routing-vbf.h"

namespace ns3 {

// Vectorbasedforward  Entry
// Vectorbasedforward Agent
class AquaSimAddress;
class Packet;
class Time;

class AquaSimFloodingRouting : public AquaSimRouting {
 public:
  AquaSimFloodingRouting();
  static TypeId GetTypeId(void);
  virtual bool Recv(Ptr< Packet > packet, const Address &dest, uint16_t protocolNumber);

  // Vectorbasedforward_Entry routing_table[MAX_DATA_TYPE];

 protected:
  int m_pkCount;
  // int port_number;
  AquaSimPktHashTable PktTable;
  /*AquaSimPktHashTable SourceTable;
  AquaSimPktHashTable Target_discoveryTable;
  AquaSimPktHashTable SinkTable;*/    //not used...
  //UWDelayTimer delaytimer;

  void Terminate();
  void Reset();
  void ConsiderNew(Ptr<Packet> pkt);
  Ptr<Packet> CreatePacket();
  Ptr<Packet> PrepareMessage(unsigned int dtype, AquaSimAddress addr, int msg_type);

  void DataForSink(Ptr<Packet> pkt);
  void StopSource();
  void MACprepare(Ptr<Packet> pkt);
  void MACsend(Ptr<Packet> pkt, Time delay=Seconds(0));
};

} // namespace ns3

#endif /* AQUA_SIM_ROUTING_FLOODING_H */
