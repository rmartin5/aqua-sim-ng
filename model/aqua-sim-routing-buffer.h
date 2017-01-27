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

#ifndef AQUA_SIM_ROUTING_BUFFER_H
#define AQUA_SIM_ROUTING_BUFFER_H

#include "aqua-sim-address.h"
#include "ns3/packet.h"
#include "ns3/object.h"

namespace ns3 {

typedef struct AquaSimRoutingBufferCell : Object {
  Ptr<Packet> packet;
  Ptr<AquaSimRoutingBufferCell> next;
  double arrival_time;
} routing_buffer_cell;

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Buffer helper class for underwater routing
 */
class AquaSimRoutingBuffer : public Object{
public:
  AquaSimRoutingBuffer(int size=10, int myuser=1)
  {
    m_numOfPacket=0;
    m_maximumSize=size;
    m_usr=myuser;
  };
  ~AquaSimRoutingBuffer();

  void AddNewPacket(Ptr<Packet>);
  void CopyNewPacket(Ptr<Packet>);// copy the packet and put into queue
  Ptr<Packet> Dehead();
  Ptr<Packet> DeQueue(AquaSimAddress,unsigned int);
  // Ptr<Packet> Lookup(AquaSimAddress, int);
  Ptr<Packet> LookupCopy(AquaSimAddress,unsigned int);
  Ptr<Packet> Head();
  inline int BufferSize()const {return m_numOfPacket;}
  bool IsEmpty();
  bool IsFull();
  int m_usr; // this added later distinguish VBVA and VBF 1 is used for vbf 0 is used for vbva
protected:
  void DoDispose();
private:
  int m_numOfPacket;
  int m_maximumSize;
  Ptr<routing_buffer_cell> m_head;
  Ptr<routing_buffer_cell> m_tail;
};  // class AquaSimRoutingBuffer

}  // namespace ns3

#endif /* AQUA_SIM_ROUTING_BUFFER_H */
