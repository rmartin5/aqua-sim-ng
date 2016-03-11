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

#include "aqua-sim-routing-buffer.h"
#include "aqua-sim-header-routing.h"

using namespace ns3;

void
AquaSimRoutingBuffer::AddNewPacket(Ptr<Packet> p)
{
	routing_buffer_cell* t1=new routing_buffer_cell;

  VBHeader vbh;
  p->PeekHeader(vbh);

	AquaSimAddress source;
  /*if (m_usr) source=vbh->sender_id;
	else source=vbvah->sender_id;*/
  source = vbh.GetSenderAddr();
  unsigned int pkt_num;
  pkt_num = vbh.GetPkNum();
  /*
  if(m_usr) pkt_num=vbh->pk_num;
	else pkt_num= vbvah->pk_num;
  */

	Ptr<Packet> tpkt=DeQueue(source,pkt_num); // avoid duplication
	if(tpkt) tpkt=0;


	if (IsFull()) {
		//      printf("ok, full\n");
		Dehead()=0;
	}
	if(!t1) {
		//  printf("AquaSimRoutingBuffer: can't get allocated  memory\n");
		return;
	}

	t1->packet=p;
	t1->next=NULL;

	if(m_head==NULL) {
		// printf("head is empty ok\n");
		m_tail=t1;
		m_head=t1;
	}
	else{
		//  printf("head is not empty ok\n");
		m_tail->next=t1;
		m_tail=t1;
	}

	m_numOfPacket++;
}


void
AquaSimRoutingBuffer::CopyNewPacket(Ptr<Packet> pkt){
	routing_buffer_cell* t1=new routing_buffer_cell;
	Ptr<Packet> p=pkt->Copy();

  VBHeader vbh;
  p->PeekHeader(vbh);

	AquaSimAddress source;
	/*if (m_usr) source=vbh->sender_id;
	else source=vbvah->sender_id;*/
  source = vbh.GetSenderAddr();
	unsigned int pkt_num;
  pkt_num = vbh.GetPkNum();
	/*if(m_usr) pkt_num=vbh->pk_num;
	else pkt_num= vbvah->pk_num;*/


	/*
	   AquaSimAddress source=((m_usr)?: vbh->sender_id,vbvah->sender_id);
	   unsigned int pkt_num=((m_usr)?: vbh->pk_num, vbvah->pk_num);


	   AquaSimAddress source=vbh->sender_id;
	   unsigned int pkt_num=vbh->pk_num;
	 */

	//  printf("uw_buffer: pkt_num is%d\n",pkt_num);
	Ptr<Packet> tpkt=DeQueue(source,pkt_num); // avoid duplication
	if(tpkt) tpkt=0;

	if (IsFull()) Dehead()=0;

	t1->packet=p;
	t1->next=NULL;

	if(m_head==NULL) {
		m_tail=t1;
		m_head=t1;
	}
	else{
		m_tail->next=t1;
		m_tail=t1;
	}

	m_numOfPacket++;
	//printf("CopyNewPacket the pkt_num is %d and %d packets in buffer\n",vbh->pk_num,m_numOfPacket);
}


Ptr<Packet>
AquaSimRoutingBuffer::Head()
{
	//routing_buffer_cell* t1;
	//routing_buffer_cell* t2;
	Ptr<Packet> p;

	if(!m_head) return NULL;
	else return m_head->packet;
}


Ptr<Packet>
AquaSimRoutingBuffer::Dehead()
{
	//routing_buffer_cell* t1;
	routing_buffer_cell* t2;
	Ptr<Packet> p;

	if(!m_head) return NULL;
	p=m_head->packet;
	t2=m_head;
	m_head=m_head->next;
	m_numOfPacket--;

	if(!m_head) m_tail=NULL;
	delete t2;
	return p;
}

bool
AquaSimRoutingBuffer::IsEmpty()
{
	return(0==m_numOfPacket);
}

bool
AquaSimRoutingBuffer::IsFull()
{
	//printf("maximum size is %d and num_packet is%d\n",m_maximumSize,m_numOfPacket);
	return(m_maximumSize==m_numOfPacket);
}


Ptr<Packet>
AquaSimRoutingBuffer::DeQueue( AquaSimAddress sender,unsigned int num)
{
	routing_buffer_cell * current_p=m_head;
	routing_buffer_cell * previous_p=m_head;
	Ptr<Packet> p;
	if(IsEmpty()) return NULL;
	while (current_p)
	{
    VBHeader vbh;
    (current_p->packet)->PeekHeader(vbh);

		AquaSimAddress source;
		/*if (m_usr) source=vbh->sender_id;
		else source=vbvah->sender_id;*/
    source = vbh.GetSenderAddr();
		unsigned int pkt_num;
    pkt_num = vbh.GetPkNum();
		/*if(m_usr) pkt_num=vbh->pk_num;
		else pkt_num= vbvah->pk_num;*/

		/*
		   AquaSimAddress source=((m_usr)?: vbh->sender_id,vbvah->sender_id);
		   unsigned int pkt_num=((m_usr)?: vbh->pk_num, vbvah->pk_num);
		 */

		//printf("ok, DEQUEUE buffer sender id is %d num=%d\n",vbh->sender_id.addr_,vbh->pk_num);
		if((source == sender) && /*(source.port_==sender.port_)&&*/(pkt_num==num))
		{
			p=current_p->packet;

			if(current_p==m_head)
      {
				m_head=m_head->next;
				if(!m_head) m_tail=0;
				//	delete current_p;
			}
			else
			{
				if(current_p==m_tail) m_tail=previous_p;
				previous_p->next=current_p->next;
				//    delete current_p;
			}

			delete current_p;
			m_numOfPacket--;
			return p;
		}
		previous_p=current_p;
		current_p=current_p->next;

	}
	return NULL;
}


Ptr<Packet>
AquaSimRoutingBuffer::LookupCopy( AquaSimAddress sender,unsigned int num)
{
	routing_buffer_cell * current_p=m_head;
	Ptr<Packet> p;
	if(IsEmpty())
  {
		//printf("buffer: the data link is empty\n");
		return NULL;
	}
	while (current_p)
	{
    VBHeader vbh;
    (current_p->packet)->PeekHeader(vbh);

		AquaSimAddress source;
		/*if (m_usr) source=vbh->sender_id;
		else source=vbvah->sender_id;*/
    source = vbh.GetSenderAddr();
		unsigned int pkt_num;
    pkt_num = vbh.GetPkNum();
		/*if(m_usr) pkt_num=vbh->pk_num;
		else pkt_num= vbvah->pk_num;*/

		/*
		   AquaSimAddress source=((m_usr)?: vbh->sender_id,vbvah->sender_id);
		   unsigned int pkt_num=((m_usr)?: vbh->pk_num, vbvah->pk_num);
		 */

		//printf("ok, Lookup buffer sender id is %d num=%d\n",vbh->sender_id.addr_,vbh->pk_num);
		if((source == sender) && /*(source.port_==sender.port_)&&*/ (pkt_num==num))
    {
			p=current_p->packet;
			return p;
		}
		current_p=current_p->next;
	}
	return p;
}
