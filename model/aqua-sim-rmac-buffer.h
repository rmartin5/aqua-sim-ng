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

#ifndef AQUA_SIM_RMAC_BUFFER_H_
#define AQUA_SIM_RMAC_BUFFER_H_

#include "ns3/packet.h"

#define MAXIMUM_BUFFER 1

namespace ns3{

struct buffer_cell{
  Ptr<Packet> packet;
  buffer_cell * next;
  double delay;
};


class TransmissionBuffer {
public:
  TransmissionBuffer(){
		head_=NULL;
		current_p=NULL;
		num_of_packet=0;
		lock=false;
		tail_=NULL;
		lock_p=NULL;
			};
  static TypeId GetTypeId (void);

  void AddNewPacket(Ptr<Packet> p);
  void LockBuffer();
  void UnlockBuffer();
  int  DeletePacket(Ptr<Packet> p);
  Ptr<Packet> dehead();
  Ptr<Packet> next();
  Ptr<Packet> head();
  bool  IsEnd();
  bool IsEmpty();
  bool IsFull();
  bool ToBeFull();
  bool IsLocked(){return lock;};
  buffer_cell * lookup(Ptr<Packet> p);
  int num_of_packet;// number of sending packets
  buffer_cell* head_;
  bool lock;
private:
     buffer_cell* current_p;
     buffer_cell* lock_p;
     buffer_cell* tail_;
};  // class TransmissionBuffer



}  // namespace ns3

#endif /* AQUA_SIM_RMAC_BUFFER_H_ */
