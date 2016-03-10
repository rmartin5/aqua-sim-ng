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
  virtual bool Recv(Ptr<Packet>);

  // Vectorbasedforward_Entry routing_table[MAX_DATA_TYPE];

 protected:
  int m_pkCount;
  // int port_number;
  /*  TODO implement these... see vectorbasedforward.h for class info.
  UWPkt_Hash_Table PktTable;
  UWPkt_Hash_Table SourceTable;
  UWPkt_Hash_Table Target_discoveryTable;
  UWPkt_Hash_Table SinkTable;
  */
  //UWDelayTimer delaytimer;

  //Trace *tracetarget;       //TODO Trace Target


  void Terminate();
  void Reset();
  void ConsiderNew(Ptr<Packet> pkt);
  Ptr<Packet> CreatePacket();
  Ptr<Packet> PrepareMessage(unsigned int dtype, AquaSimAddress addr, int msg_type);

  void DataForSink(Ptr<Packet> pkt);
  void StopSource();
  void MACprepare(Ptr<Packet> pkt);
  void MACsend(Ptr<Packet> pkt, Time delay=Seconds(0));

  //void trace(char *fmt,...);
};

} // namespace ns3

#endif /* AQUA_SIM_ROUTING_FLOODING_H */
