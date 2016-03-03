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

#include "ns3/log.h"
#include "ns3/packet.h"

#include "aqua-sim-rmac-buffer.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE("TransmissionBuffer");
NS_OBJECT_ENSURE_REGISTERED(TransmissionBuffer);


TypeId
TransmissionBuffer::GetTypeId()
{
  static TypeId tid = TypeId("ns3::TransmissionBuffer")
  ;
  return tid;
}

void
TransmissionBuffer::AddNewPacket(Ptr<Packet> p){
  //buffer_cell* t2;  //unused
  buffer_cell* t1=new buffer_cell;

  t1->packet=p;
  t1->next=NULL;

  if(head_==NULL) {
     tail_=t1;
     head_=t1;
  }
  else{
  tail_->next=t1;
  tail_=t1;
  }

  num_of_packet++;
}


Ptr<Packet>
TransmissionBuffer::head(){
  //buffer_cell* t1;  //unused
  //buffer_cell* t2;  //unused
  Ptr<Packet> p;

  if(!head_) return NULL;
  else return head_->packet;
}


Ptr<Packet>
TransmissionBuffer::dehead(){
  buffer_cell* t1;
  buffer_cell* t2;
  Ptr<Packet> p;

  if(!head_) return NULL;
   p=head_->packet;
   t1=head_->next;
   t2=head_;

   head_=t1;
   num_of_packet--;

   if(head_==NULL) tail_=NULL;
    delete t2;
   return p;
}


Ptr<Packet>
TransmissionBuffer::next(){
  Ptr<Packet> p;
  if(!current_p) return NULL;
  p=current_p->packet;
  current_p=current_p->next;
  Ptr<Packet> p1= p->Copy();
   return p1;
}


int
TransmissionBuffer::DeletePacket(Ptr<Packet> p){
  buffer_cell* t1;
  buffer_cell* t2;

  // insert this packet at the head of the link
  t2=head_;


  if (!t2) return 0;//0 no such point, 1:delete this point

  if (p==t2->packet){
    NS_LOG_INFO("AquaSimRMac(TransmissionBuffer): the packet is at the head of list");
    head_=t2->next;
    num_of_packet--;

   if(head_==NULL) tail_=NULL;

    p=0;
    delete t2;

    return 1;
  }

  int modified=0;
  while(t2->next){
    if ((t2->next)->packet!=p) t2=t2->next;
    else{

     t1=t2->next;
     t2->next=t1->next;

     if(t1==tail_) tail_=t2;
     num_of_packet--;
    delete t1;
    p=0;
    modified=1;
    }
  }

  return modified;
}


buffer_cell*
TransmissionBuffer::lookup(Ptr<Packet> p){
  buffer_cell* t2;
  t2=head_;
  while((t2->packet!=p)&&(!t2)) t2=t2->next;
  return t2;
}


void
TransmissionBuffer::LockBuffer(){
  current_p=head_;
  lock_p=tail_;
  lock=true;
}


void
TransmissionBuffer::UnlockBuffer(){
  lock=false;
  lock_p=NULL;
}


bool
TransmissionBuffer::IsEmpty(){
  return(0==num_of_packet);
}

bool
TransmissionBuffer::ToBeFull(){
  return((MAXIMUM_BUFFER-1)==num_of_packet);
}




bool
TransmissionBuffer::IsEnd(){
  if (lock_p) return (lock_p->next==current_p);
  return(NULL==current_p);
}



bool
TransmissionBuffer::IsFull(){
  return(MAXIMUM_BUFFER==num_of_packet);
}

}  // namespace ns3
