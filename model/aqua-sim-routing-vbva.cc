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

#include "aqua-sim-routing-vbva.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-header.h"
#include "aqua-sim-pt-tag.h"
#include "aqua-sim-propagation.h"

#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "ns3/mobility-model.h"
#include "ns3/simulator.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimVBVA");

void AquaSimVBVAPktHashTable::Reset()
{
  m_htable.clear();
  /*
  neighborhood *hashPtr;
  Tcl_HashEntry *entryPtr;
  Tcl_HashSearch searchPtr;

  entryPtr = Tcl_FirstHashEntry(&htable, &searchPtr);
  while (entryPtr != NULL) {
    hashPtr = (neighborhood *)Tcl_GetHashValue(entryPtr);
     delete hashPtr;
    Tcl_DeleteHashEntry(entryPtr);
    entryPtr = Tcl_NextHashEntry(&searchPtr);
  }
  */
}


neighborhood*
AquaSimVBVAPktHashTable::GetHash(AquaSimAddress senderAddr,unsigned int pk_num)
{
  hash_entry entry = std::make_pair (senderAddr,pk_num);
  std::map<hash_entry,neighborhood*>::iterator it;

  it = m_htable.find(entry);

  if (it == m_htable.end())
    return NULL;

  return it->second;
  /*
  unsigned int key[3];

  key[0] = sender_id.addr_;
  key[1] = sender_id.port_;
  key[2] = pk_num;

  Tcl_HashEntry *entryPtr = Tcl_FindHashEntry(&htable, (char *)key);

  if (entryPtr == NULL )
     return NULL;

  return (neighborhood *)Tcl_GetHashValue(entryPtr);
  */
}


void AquaSimVBVAPktHashTable::DeleteHash(VBHeader * vbh)
{
	//Tcl_HashEntry *entryPtr;
	//neighborhood* hashPtr;

  hash_entry entry = std::make_pair (vbh->GetSenderAddr(),vbh->GetPkNum());
  std::map<hash_entry,neighborhood*>::iterator it;

	//entryPtr=Tcl_FindHashEntry(&m_htable, (char *)key);
  if(m_htable.count(entry)>0)
	{
    it = m_htable.find(entry);
    m_htable.erase(it);
	}
	return;
}


void AquaSimVBVAPktHashTable::DeleteHash(AquaSimAddress source, unsigned int pkt_num)
{
	//Tcl_HashEntry *entryPtr;
	//neighborhood* hashPtr;
  hash_entry entry = std::make_pair (source,pkt_num);
  std::map<hash_entry,neighborhood*>::iterator it;

	//entryPtr=Tcl_FindHashEntry(&m_htable, (char *)key);
  if(m_htable.count(entry)>0)
	{
    it = m_htable.find(entry);
    m_htable.erase(it);
	}
	return;
}

void AquaSimVBVAPktHashTable::MarkNextHopStatus(AquaSimAddress senderAddr,
                                             unsigned int pk_num,
                                             unsigned int forwarder_id,
                                             unsigned int status)
{
	//Tcl_HashEntry *entryPtr;

	neighborhood* hashPtr;
	//unsigned int key[3];
	//int newPtr;
	// unsigned int forwarder_id=forwarder_id;

  hash_entry entry = std::make_pair (senderAddr,pk_num);

	//entryPtr = Tcl_CreateHashEntry(&m_htable, (char *)key, &newPtr);
  if(m_htable.count(entry)>0)
  {
		hashPtr=GetHash(senderAddr,pk_num);
		int m=hashPtr->number;

		for (int i=0; i<m; i++) {
			if ((hashPtr->neighbor[i].forwarder_id==forwarder_id)&&
			    (hashPtr->neighbor[i].status==FRESHED))
				hashPtr->neighbor[i].status=status;
		}
	}
	else
    NS_LOG_WARN("hashtable, the packet record doesn't exist");

	return;
}


void
AquaSimVBVAPktHashTable::PutInHash(VBHeader * vbh)
{
	//Tcl_HashEntry *entryPtr;
	neighborhood* hashPtr;
  AquaSimAddress addr;
  unsigned int addrAsInt;
  unsigned int pkNum;
	//unsigned int key[3];
	bool newPtr = true;

  addr=vbh->GetForwardAddr();
  addrAsInt=addr.GetAsInt();
  pkNum=vbh->GetPkNum();
  hash_entry entry = std::make_pair (addr,pkNum);
  std::map<hash_entry,neighborhood*>::iterator it;
	//key[0]=(vbh->sender_id).addr_;
	//key[1]=(vbh->sender_id).port_;
	//key[2]=vbh->pk_num;

	int k=pkNum-m_windowSize;
	if(k>0)
	{
		for (int i=0; i<k; i++)
		{
			pkNum=i;
      if(m_htable.count(entry)>0)
      {
        it = m_htable.find(entry);
        hashPtr = it->second;
        newPtr = false;
        m_htable.erase(it);
      }
		}
	}

  pkNum=vbh->GetPkNum();
	//entryPtr = Tcl_CreateHashEntry(&htable, (char *)key, &newPtr);
	if (!newPtr) {// the record already exist
    //hashPtr=GetHash(vbh->GetSenderAddr(),vbh->GetPkNum());
		int m=hashPtr->number;

		int k=0;
		while((hashPtr->neighbor[k].forwarder_id!=addrAsInt)&&(k<m)) k++;

		if(k==m) hashPtr->number++;
		if (k<MAX_NEIGHBOR) {
			hashPtr->neighbor[k].vec.start.x=0;
			hashPtr->neighbor[k].vec.start.y=0;
			hashPtr->neighbor[k].vec.start.z=0;

			hashPtr->neighbor[k].vec.end.x=0;
			hashPtr->neighbor[k].vec.end.y=0;
			hashPtr->neighbor[k].vec.end.z=0;

			hashPtr->neighbor[k].node.x=0;
			hashPtr->neighbor[k].node.y=0;
			hashPtr->neighbor[k].node.z=0;

			hashPtr->neighbor[k].forwarder_id=addrAsInt;
			hashPtr->neighbor[k].status=FRESHED;
		}
		else {
			for(int i=1; i<MAX_NEIGHBOR; i++)
			{
				hashPtr->neighbor[i-1].vec=hashPtr->neighbor[i].vec;
				hashPtr->neighbor[i-1].node=hashPtr->neighbor[i].node;

				hashPtr->neighbor[i-1].forwarder_id=hashPtr->neighbor[i].forwarder_id;
				hashPtr->neighbor[i-1].status= hashPtr->neighbor[i].status;
			}

			hashPtr->neighbor[MAX_NEIGHBOR-1].vec.start.x=0;
			hashPtr->neighbor[MAX_NEIGHBOR-1].vec.start.y=0;
			hashPtr->neighbor[MAX_NEIGHBOR-1].vec.start.z=0;

			hashPtr->neighbor[MAX_NEIGHBOR-1].vec.end.x=0;
			hashPtr->neighbor[MAX_NEIGHBOR-1].vec.end.y=0;
			hashPtr->neighbor[MAX_NEIGHBOR-1].vec.end.z=0;

			hashPtr->neighbor[MAX_NEIGHBOR-1].node.x=0;
			hashPtr->neighbor[MAX_NEIGHBOR-1].node.y=0;
			hashPtr->neighbor[MAX_NEIGHBOR-1].node.z=0;


			hashPtr->neighbor[MAX_NEIGHBOR-1].forwarder_id=addrAsInt;
			hashPtr->neighbor[MAX_NEIGHBOR-1].status=FRESHED;
		}
		return;
	}

	// the record does not exist

	hashPtr=new neighborhood;
	hashPtr->number=1;


	hashPtr->neighbor[0].vec.start.x=0;
	hashPtr->neighbor[0].vec.start.y=0;
	hashPtr->neighbor[0].vec.start.z=0;

	hashPtr->neighbor[0].vec.end.x=0;
	hashPtr->neighbor[0].vec.end.y=0;
	hashPtr->neighbor[0].vec.end.z=0;

	hashPtr->neighbor[0].node.x=0;
	hashPtr->neighbor[0].node.y=0;
	hashPtr->neighbor[0].node.z=0;

	hashPtr->neighbor[0].forwarder_id=addrAsInt;
	hashPtr->neighbor[0].status=FRESHED;

  m_htable.insert(std::pair<hash_entry,neighborhood*>(entry,hashPtr));
  //Tcl_SetHashValue(entryPtr, hashPtr);
}


void AquaSimVBVAPktHashTable::PutInHash(VBHeader * vbh, Vector3D* sp, Vector3D* tp, Vector3D* fp, unsigned int status)
{
	//Tcl_HashEntry *entryPtr;
	neighborhood* hashPtr;
	//unsigned int key[3];
  unsigned int pkNum;
  AquaSimAddress addr;
  unsigned int addrAsInt;
	int newPtr;

  addr=vbh->GetForwardAddr();
  addrAsInt=addr.GetAsInt();
  pkNum=vbh->GetPkNum();
  hash_entry entry = std::make_pair (addr,pkNum);
  std::map<hash_entry,neighborhood*>::iterator it;
	//key[0]=vbh->sender_id.addr_;
	//key[1]=vbh->sender_id.port_;
	//key[2]=vbh->pk_num;

	int k=pkNum-m_windowSize;
	if(k>0)
	{
		for (int i=0; i<k; i++) {
      pkNum=i;
      if(m_htable.count(entry)>0)
      {
        it = m_htable.find(entry);
        hashPtr = it->second;
        newPtr = false;
        m_htable.erase(it);
      }
		}
	}

  pkNum=vbh->GetPkNum();
	//entryPtr = Tcl_CreateHashEntry(&htable, (char *)key, &newPtr);
	if (!newPtr)// record already exists
	{
    //hashPtr=GetHash(vbh->GetSenderAddr(),vbh->GetPkNum());
		int m=hashPtr->number;

		int k=0;
		while((hashPtr->neighbor[k].forwarder_id!=addrAsInt)&&(k<m)) k++;
		if(k==m) hashPtr->number++;


		// printf("hash_table: this is not old item, there are %d item inside\n",m);
		if (k<MAX_NEIGHBOR) {
			hashPtr->neighbor[k].vec.start.x=sp->x;
			hashPtr->neighbor[k].vec.start.y=sp->y;
			hashPtr->neighbor[k].vec.start.z=sp->z;

			hashPtr->neighbor[k].vec.end.x=tp->x;
			hashPtr->neighbor[k].vec.end.y=tp->y;
			hashPtr->neighbor[k].vec.end.z=tp->z;

			hashPtr->neighbor[k].node.x=fp->x;
			hashPtr->neighbor[k].node.y=fp->y;
			hashPtr->neighbor[k].node.z=fp->z;


			hashPtr->neighbor[k].forwarder_id=addrAsInt;
			hashPtr->neighbor[k].status=status;
		}
		else {
			for(int i=1; i<MAX_NEIGHBOR; i++)
			{
				hashPtr->neighbor[i-1].vec=hashPtr->neighbor[i].vec;
				hashPtr->neighbor[i-1].node=hashPtr->neighbor[i].node;

				hashPtr->neighbor[i-1].forwarder_id=hashPtr->neighbor[i].forwarder_id;
				hashPtr->neighbor[i-1].status= hashPtr->neighbor[i].status;
			}

			hashPtr->neighbor[MAX_NEIGHBOR-1].vec.start=(*sp);
			hashPtr->neighbor[MAX_NEIGHBOR-1].vec.end=(*tp);
			hashPtr->neighbor[MAX_NEIGHBOR-1].node=(*fp);

			hashPtr->neighbor[MAX_NEIGHBOR-1].forwarder_id=addrAsInt;
			hashPtr->neighbor[MAX_NEIGHBOR-1].status=FRESHED;
		}

		return;
	}
	// record does not exist
	hashPtr=new neighborhood;
	hashPtr->number=1;

	hashPtr->neighbor[0].vec.start=(*sp);
	hashPtr->neighbor[0].vec.end=(*tp);
	hashPtr->neighbor[0].node=(*fp);


	hashPtr->neighbor[0].forwarder_id=addrAsInt;
	hashPtr->neighbor[0].status=status;

  m_htable.insert(std::pair<hash_entry,neighborhood*>(entry,hashPtr));
	//Tcl_SetHashValue(entryPtr, hashPtr);
	return;
}


void AquaSimVBVADataHashTable::Reset()
{
  m_htable.clear();
  /*
  unsigned int *hashPtr;
  Tcl_HashEntry *entryPtr;
  Tcl_HashSearch searchPtr;

  entryPtr = Tcl_FirstHashEntry(&htable, &searchPtr);
  while (entryPtr != NULL) {
    hashPtr = (unsigned int *)Tcl_GetHashValue(entryPtr);
     delete hashPtr;
    Tcl_DeleteHashEntry(entryPtr);
    entryPtr = Tcl_NextHashEntry(&searchPtr);
  }
  */
}

void AquaSimVBVADataHashTable::PutInHash(AquaSimAddress source, unsigned int pkt_num, unsigned int status)
{
	//Tcl_HashEntry *entryPtr;

	//int* hashPtr;
	unsigned int* valuePtr=0;

	valuePtr=new unsigned int;
	(*valuePtr)=status;

  hash_entry entry = std::make_pair (source,pkt_num);
	//entryPtr = Tcl_CreateHashEntry(&m_htable, (char*)key, &statusPtr);
	//Tcl_SetHashValue(entryPtr,valuePtr);
  m_htable.insert(std::pair<hash_entry,unsigned int*>(entry,valuePtr));
	return;
}


void AquaSimVBVADataHashTable::DeleteHash(AquaSimAddress source, unsigned int pkt_num)
{
	//Tcl_HashEntry *entryPtr;
	//unsigned int* hashPtr;
  hash_entry entry = std::make_pair (source,pkt_num);

	//entryPtr=Tcl_FindHashEntry(&m_htable, (char *)key);
  if(m_htable.count(entry)>0)
  {
    m_htable.erase(m_htable.find(entry));
	}
	return;
}

unsigned int*
AquaSimVBVADataHashTable::GetHash(AquaSimAddress source, unsigned int pkt_num)
{
  hash_entry entry = std::make_pair (source,pkt_num);
  std::map<hash_entry,unsigned int*>::iterator it;

  it = m_htable.find(entry);

  if (it == m_htable.end())
    return NULL;

  return (it->second);
}

NS_OBJECT_ENSURE_REGISTERED(AquaSimVBVA);

AquaSimVBVA::AquaSimVBVA() :
  m_miniDistance(20.0), m_miniThreshold(1.5),
  m_voidAvoidanceBuffer(15,0) /*m_receivingBuffer(10),*/
{
  m_positionUpdateTime=-1.0;
  m_pkCount = 0;
  //tracetarget = NULL;
  m_width=0;
  m_counter=0;
  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
}

TypeId
AquaSimVBVA::GetTypeId(void)
{
  static TypeId tid = TypeId ("ns3::AquaSimVBVA")
    .SetParent<AquaSimRouting> ()
    .AddConstructor<AquaSimVBVA> ()
    .AddAttribute ("ControlPacketSize", "Size of control packet.",
      IntegerValue(0),
      MakeIntegerAccessor(&AquaSimVBVA::m_controlPacketSize),
      MakeIntegerChecker<int>())
    .AddAttribute ("Width", "Width of VBF. Default is 100.",
      DoubleValue(100),
      MakeDoubleAccessor(&AquaSimVBVA::m_width),
      MakeDoubleChecker<double>())
  ;
  return tid;
}

bool AquaSimVBVA::Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
	if (GetNetDevice()->FailureStatus()==1) {
    NS_LOG_INFO("AquaSimVBVA " << GetNetDevice()->GetAddress() << " fails!!!!");
		packet=0;
		return false;
	}

  NS_LOG_FUNCTION(this << GetNetDevice()->GetAddress() << Simulator::Now().GetSeconds());

  VBHeader vbh;
  packet->PeekHeader(vbh);

	unsigned int msg_type =vbh.GetMessType();
	double t1=vbh.GetTs();
	//Vector3D * p1;
	AquaSimAddress source=vbh.GetSenderAddr();
	AquaSimAddress forwarder=vbh.GetForwardAddr();
	unsigned int pkt_num=vbh.GetPkNum();

	unsigned int* statusPtr=PacketStatusTable.GetHash(source,pkt_num);
	if(statusPtr&&((*statusPtr)==TERMINATED)) {
      NS_LOG_INFO("AquaSimVBVA " << GetNetDevice()->GetAddress() <<
        " this packet has been terminated");
		packet=0;
		return false;
	}

	/*
	     if(msg_type==BACKFLOODING){
	   // printf("vectrobasedforward node %d: recv backflooding  from the sender %d\n",here_.addr_,vbh->forward_agent_id.addr_);

	    unsigned int* packet_status=PacketStatusTable.GetHash(source,pkt_num);

	    if (((packet_status*)==FLOODED)||((packet_status*)==CENTERED)||((packet_status*)==SENT)) {
	   // printf("vectrobasedforward node %d:the data packet is flooded, termonated or center_sent\n",here_.addr_);
	        packet=0;
	       return;
	   }
	    ProcessBackFloodedPacket(packet);
	     return;
	     }
	 */

	if(msg_type==DATA_TERMINATION) {
// printf("vectrobasedforward node %d: recv DATA_TERMINATION  from the sender %d\n",here_.addr_,vbh->forward_agent_id.addr_);
		PacketStatusTable.PutInHash(source,pkt_num,TERMINATED);
		packet=0;
		return false;
	}

	if((msg_type==V_SHIFT)||(msg_type==V_SHIFT_DATA)) {
		//printf("vectrobasedforward node %d: recv V_SHIFT or V_D from the sender %d\n",here_.addr_,vbh->forward_agent_id.addr_);
		if(IsNewlyTouchedNode(source, pkt_num)) {
      NS_LOG_INFO("AquaSimVBVA " << GetNetDevice()->GetAddress() <<
        " is a newly touched node");

//  unsigned int* packet_status=PacketStatusTable.GetHash(source,pkt_num);

			PacketStatusTable.PutInHash(source,pkt_num,SUPPRESSED);
			ProcessCenteredPacket(packet);
			return true;
		} else packet=0;
	}

	if((msg_type==EXPENSION)||(msg_type==EXPENSION_DATA)) {
		//printf("vectrobasedforward node %d: recv EXPENSION  from the sender %d\n",here_.addr_,vbh->forward_agent_id.addr_);

		unsigned int* packet_status=PacketStatusTable.GetHash(source,pkt_num);

		if((!packet_status)||((*packet_status)==SUPPRESSED)) {
			PacketStatusTable.PutInHash(source,pkt_num,VOID_SUPPRESSED);
			ProcessCenteredPacket(packet);
			return true;
		} else packet=0;
	}

	if(msg_type==BACKPRESSURE) {
		//printf("vectrobasedforward node %d: receives a backpressure packet(%d) from %d\n",here_.addr_,pkt_num,vbh->forward_agent_id.addr_);
		ProcessBackpressurePacket(packet);
		return true;
	}

	if(msg_type==AS_DATA) {
    NS_LOG_INFO("AquaSimVBVA " << GetNetDevice()->GetAddress() <<
      " receives a DATA packet from " << vbh.GetForwardAddr());
		unsigned int* statusPtr= PacketStatusTable.GetHash(source,pkt_num);
		neighborhood *  packetPtr=PktTable.GetHash(source,pkt_num);

		if ((statusPtr)||(packetPtr)) {

			RecordPacket(&vbh);
			packet=0;
			return true;
		}

		//   if(t1>m_positionUpdateTime) {
		CalculatePosition(packet);
		m_positionUpdateTime=t1;
		// }

		ConsiderNew(packet);
		return true;
	}
  return false;
}


// this function assme that the end points of the vectors are the same
bool
AquaSimVBVA::IsSamePosition(const Vector3D* sp1, const Vector3D* sp2)
{
    double err=0.1;
    if(fabs(sp1->x-sp2->x)>err) return false;
    if(fabs(sp1->y-sp2->y)>err) return false;
    if(fabs(sp1->z-sp2->z)>err) return false;
    return true;
}

void
AquaSimVBVA::RecordPacket(VBHeader* vbh, unsigned int status)
{
      Vector3D sp,ep,fp;


      fp.x=vbh->GetExtraInfo().f.x;
      fp.y=vbh->GetExtraInfo().f.y;
      fp.z=vbh->GetExtraInfo().f.z;

      sp.x=vbh->GetExtraInfo().o.x;
      sp.y=vbh->GetExtraInfo().o.y;
      sp.z=vbh->GetExtraInfo().o.z;

      ep.x=vbh->GetExtraInfo().t.x;
      ep.y=vbh->GetExtraInfo().t.y;
      ep.z=vbh->GetExtraInfo().t.z;

      PktTable.PutInHash(vbh,&sp,&ep,&fp, status);

    return;
}


void AquaSimVBVA::ProcessCenteredPacket(Ptr<Packet> pkt)
{
	//  printf("vectorbased node %d: process centered packet\n",here_.addr_);
	if(!pkt) {
		NS_LOG_DEBUG("AquaSimVBVA node " << GetNetDevice()->GetAddress() << " data packet is empty");
		return;
	}

  VBHeader vbh;
  pkt->PeekHeader(vbh);

	AquaSimAddress source=vbh.GetSenderAddr();
	unsigned pkt_num=vbh.GetPkNum();

	Ptr<Packet> tpacket = Create<Packet>();

	Vector3D mp,tp,sp,fp;

	mp.x=GetNetDevice()->CX();
	mp.y=GetNetDevice()->CY();
	mp.z=GetNetDevice()->CZ();

	tp.x=vbh.GetExtraInfo().t.x;
	tp.y=vbh.GetExtraInfo().t.y;
	tp.z=vbh.GetExtraInfo().t.z;

	fp.x=vbh.GetExtraInfo().f.x;
	fp.y=vbh.GetExtraInfo().f.y;
	fp.z=vbh.GetExtraInfo().f.z;

	sp.x=vbh.GetExtraInfo().o.x;
	sp.y=vbh.GetExtraInfo().o.y;
	sp.z=vbh.GetExtraInfo().o.z;

	//printf("vectorbased: node(%d) sp (%f,%f,%f) tp (%f,%f,%f) and fp(%f,%f,%f)\n",here_.addr_,sp.x,sp.y,sp.z,tp.x,tp.y,tp.z,fp.x,fp.y,fp.z);


	double delay_factor=CalculateSelfCenteredDelay(&sp,&tp,&mp,&fp);
	if (delay_factor>1.2) {
		pkt=0;
		return;
	}


	if((vbh.GetMessType()==V_SHIFT)||(vbh.GetMessType()==EXPENSION)) {

		pkt=0;

		Ptr<Packet> p=m_voidAvoidanceBuffer.LookupCopy(source,pkt_num);

		if(!p) {
      NS_LOG_WARN("AquaSimVBVA: node " << GetNetDevice()->GetAddress() <<
        " can not find the corresponding packet in the buffer");
			return;
		}
		tpacket=p->Copy();

	} else{

		// in case this node is target
		if (GetNetDevice()->GetAddress()==vbh.GetTargetAddr())
		{
			SendDataTermination(pkt);
			PacketStatusTable.PutInHash(source,pkt_num,TERMINATED);
			DataForSink(pkt); // process it
			return;
		}

		tpacket=pkt->Copy();
		pkt=0;
	}

	if(!tpacket) {
    NS_LOG_WARN("AquaSimVBVA: node " << GetNetDevice()->GetAddress() <<
      " can not generate the corresponding packet");
		return;
	}
  tpacket->RemoveHeader(vbh);

  Vector3D o = Vector3D(mp.x,mp.y,mp.z);
	vbh.SetExtraInfo_o(o);
  vbh.SetExtraInfo_f(o);

	vbh.SetMessType(AS_DATA);

	double delay= sqrt(delay_factor)*DELAY*2.0;
	//     printf("vectorbased:node %d sets its timer for %f at %f\n", here_.addr_, delay,NOW);

  tpacket->AddHeader(vbh);
  Simulator::Schedule(Seconds(delay),&AquaSimVBVA::ProcessSelfcenteredTimeout,this,tpacket);

	// added by peng xie 20071118
	PktTable.DeleteHash(source,pkt_num);// to be used by timeout process
	return;
}

void AquaSimVBVA::ProcessBackpressurePacket(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this);
  //  printf("Vectorbasedvoidavoidance node %d process BackpressurePacket\n",here_.addr_);
  if(!pkt) {
    NS_LOG_DEBUG("No packet.)");
    // printf("Vectorbasedvoidavoidance node %d ProcessBackpressurePacket: the packet is empty\n",here_.addr_);
    return;
  }

  VBHeader vbh;
  pkt->PeekHeader(vbh);

  //unsigned int msg_type =vbh.GetMessType();

  AquaSimAddress  target_id, source, forwarder;
  int num=vbh.GetPkNum();
  unsigned int packet_status=SUPPRESSED;

  target_id=vbh.GetTargetAddr();
  source=vbh.GetSenderAddr();

  Vector3D tp, fp,mp;

   tp.x=vbh.GetExtraInfo().t.x;
   tp.y=vbh.GetExtraInfo().t.y;
   tp.z=vbh.GetExtraInfo().t.z;

   fp.x=vbh.GetExtraInfo().f.x;
   fp.y=vbh.GetExtraInfo().f.y;
   fp.z=vbh.GetExtraInfo().f.z;

   mp.x=GetNetDevice()->CX();
   mp.y=GetNetDevice()->CY();
   mp.z=GetNetDevice()->CZ();


   // added by peng xie 20071118
   if(!IsUpstreamNode(mp,fp,tp)){
     // not from upstream node, ignore it,
     pkt=0;
     return;
   }


   unsigned int * statusPtr= PacketStatusTable.GetHash(source,num);
  if(!statusPtr){
    NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
      " never process the data packet referenced by this backpressure packt");
       pkt=0;
       return;
  } else {
    packet_status=(*statusPtr);
  }

  // PktTable.MarkNextHopStatus(source,num,forwarder.addr_,DEAD);


  // addded by peng xie at 20071117
  if ((packet_status==SUPPRESSED)||(packet_status==TERMINATED)||(packet_status==VOID_SUPPRESSED)) {
    NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
      " this backpressure have been processed or not sent by this node");
    pkt=0;
       return;
  }


    neighborhood *hashPtr= PktTable.GetHash(source, num);

    if (!hashPtr){
      NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
        " there is no record for this backpressure ");
  pkt=0;
  return;
    }


      PktTable.MarkNextHopStatus(source,num,forwarder.GetAsInt(),DEAD);

    neighbornode* forwarder_list= hashPtr->neighbor;
    int num_of_forwarder=hashPtr->number;

    if(IsStuckNode(forwarder_list,&tp,num_of_forwarder,packet_status)){
      NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() << " is stuck node ");
      if((packet_status==FORWARDED)||(packet_status==CENTER_FORWARDED)){
        PacketStatusTable.PutInHash(source,num,FLOODED);

              Ptr<Packet> p = Create<Packet>();
           if(packet_status==FORWARDED) p=GenerateControlDataPacket(pkt,V_SHIFT_DATA);
	   else  p=GenerateControlDataPacket(pkt,EXPENSION_DATA);

             Ptr<Packet> pt=GenerateBackpressurePacket(pkt);
        	 pkt=0;

	   if(!p){
       NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
         " can not generate control data packet ");
	     return;
	   }
       if(!pt){
         NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
           " can not generate backpressure packet");
	     return;
	   }

          PktTable.DeleteHash(source,num);
          double d3=(GetNetDevice()->TransmitDistance())/ns3::SOUND_SPEED_IN_WATER;
            double d4=m_rand->GetValue()*JITTER;
	    //  double c=DELAY*sqrt(m_miniThreshold)+JITTER+d3*3+d4;
	     double c=DELAY*sqrt(3.0)*4.0+JITTER+d3*3+d4;
          Simulator::Schedule(Seconds(c),&AquaSimVBVA::ProcessBackpressureTimeout,this,pt);

        MACprepare(p);
        MACsend(p,0);
      } else {
        PacketStatusTable.PutInHash(source,num,TERMINATED);
        Ptr<Packet> p=GenerateBackpressurePacket(pkt);
	pkt=0;

        if(!p){
          NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
            " can not generate backpressure packet");
	     return;
	   }

        MACprepare(p);
        MACsend(p,0);
      }

      return;
    }
}


void AquaSimVBVA::ConsiderNew(Ptr<Packet> pkt)
{
  VBHeader vbh;
  pkt->PeekHeader(vbh);
	unsigned char msg_type =vbh.GetMessType();
	AquaSimAddress source=vbh.GetSenderAddr();
	unsigned int pkt_num=vbh.GetPkNum();

	double l;//,h;

	//neighborhood * hashPtr;  //unused
	AquaSimAddress from_nodeAddr;//, forward_nodeAddr, target_nodeID;

  Ptr<Packet> gen_pkt;
	VBHeader gen_vbh;

	Vector3D sp,ep,fp;

  sp.x=vbh.GetExtraInfo().o.x;
  sp.y=vbh.GetExtraInfo().o.y;
  sp.z=vbh.GetExtraInfo().o.z;

  ep.x=vbh.GetExtraInfo().t.x;
  ep.y=vbh.GetExtraInfo().t.y;
  ep.z=vbh.GetExtraInfo().t.z;

  fp.x=vbh.GetExtraInfo().f.x;
  fp.y=vbh.GetExtraInfo().f.y;
  fp.z=vbh.GetExtraInfo().f.z;

  NS_LOG_INFO("Consider New!");
	//   printf ("vectorbasedvoidavoidance:(id :%d) forward:(%d ,%d) sender is(%d,%d,%d), my position is (%f,%f,%f) forward position is (%f,%f,%f) at time %f  \n",here_.addr_, vbh->forward_agent_id.addr_, vbh->forward_agent_id.port_,vbh->sender_id.addr_,vbh->sender_id.port_,vbh->pk_num,node->X(),node->Y(),node->Z(),vbh->info.fx,vbh->info.fy,vbh->info.fz,NOW);

	switch (msg_type) {
	case INTEREST:
		// printf("Vectorbasedvoidavoidance:it is interest packet!\n");
		//hashPtr = PktTable.GetHash(vbh.GetSenderAddr(), vbh.GetPkNum());  //unused

		// Check if it comes from sink agent of  this node
		// If so we have to keep it in sink list

    from_nodeAddr = vbh.GetSenderAddr();
		//forward_nodeAddr = vbh.GetForwardAddr();
		//  printf("Vectorbasedvoidavoidance:it the from_nodeid is %d %d  and theb this node id is %d ,%d!\n", from_nodeID.addr_,from_nodeID.port_,THIS_NODE.addr_,THIS_NODE.port_ );

		if (GetNetDevice()->GetAddress() == from_nodeAddr) {

			MACprepare(pkt);
			MACsend(pkt,0);
			//      MACsend(pkt,m_rand->GetValue()*JITTER);
      NS_LOG_INFO("AquaSimVBVA: after MACprepare(pkt)");
		}
		else
		{
			CalculatePosition(pkt);
			//printf("vectorbasedvoidavoidance: This packet is from different node\n");
			if (IsTarget(pkt))
			{
				// If this node is target?
				l=Advance(pkt);

				//  send_to_demux(pkt,0);
				//  printf("vectorbasedvoidavoidance:%d send out the source-discovery \n",here_.addr_);
        pkt->RemoveHeader(vbh);
        vbh.SetMessType(SOURCE_DISCOVERY);
        pkt->AddHeader(vbh);
				SetForwardDelayTimer(pkt,l*JITTER);
				// !!! need to re-think
			}
			else{
				// CalculatePosition(pkt);
				// No the target forwared
				l=Advance(pkt);
        //h=Projection(pkt);  //never used...
				if (IsCloseEnough(pkt)) {
					// printf("vectorbasedvoidavoidance:%d I am close enough for the interest\n",here_.addr_);
					MACprepare(pkt);
					MACsend(pkt,m_rand->GetValue()*JITTER);//!!!! need to re-think
				}
				else {
					//  printf("vectorbasedvoidavoidance:%d I am not close enough for the interest  \n",here_.addr_);
					pkt=0;
				}
			}
		}
		// pkt=0;
		return;

	case TARGET_DISCOVERY:
// from other nodes hitted by the packet, it is supposed
// to be the one hop away from the sink

// printf("Vectorbasedvoidavoidance(%d,%d):it is target-discovery  packet(%d)! it target id is %d  coordinate is %f,%f,%f and range is %f\n",here_.addr_,here_.port_,vbh->pk_num,vbh->target_id.addr_,vbh->info.tx, vbh->info.ty,vbh->info.tz,vbh->range);
  if (GetNetDevice()->GetAddress()==vbh.GetTargetAddr()) {
			m_pkCount = 0;
			// AquaSimAddress *hashPtr= PktTable.GetHash(vbh->sender_id, vbh->pk_num);
			// Received this packet before ?
			// if (hashPtr == NULL) {

			CalculatePosition(pkt);
			DataForSink(pkt);
			//	 printf("Vectorbasedvoidavoidance: %d is the target\n", here_.addr_);
			// } //New data Process this data
			//
		} else  {pkt=0; }
		return;

	case SOURCE_DISCOVERY:
		pkt=0;
// other nodes already claim to be the source of this interest
		//   SourceTable.PutInHash(vbh);
		return;


	case DATA_READY:
		//  printf("Vectorbasedvoidavoidance(%d,%d):it is data ready packet(%d)! it target id is %d \n",here_.addr_,here_.port_,vbh->pk_num,vbh->target_id.addr_);
    from_nodeAddr = vbh.GetSenderAddr();
		if (GetNetDevice()->GetAddress() == from_nodeAddr) {
			// come from the same node, broadcast it
			MACprepare(pkt);
      MACsend(pkt,m_rand->GetValue()*JITTER);
			return;
		}
    CalculatePosition(pkt);
		if (GetNetDevice()->GetAddress()==vbh.GetTargetAddr())
    {
      NS_LOG_INFO("AquaSimVBVA:::target is " << GetNetDevice()->GetAddress());
			DataForSink(pkt); // process it
		}
		else{
			// printf("Vectorbasedvoidavoidance: %d is the not  target\n", here_.addr_);
			MACprepare(pkt);
			MACsend(pkt, m_rand->GetValue()*JITTER);
		}
		return;

	case AS_DATA:
		//    printf("Vectorbasedvoidavoidance(%d,%d):it is data packet(%d)! it target id is %d  coordinate is %f,%f,%f and range is %f\n",here_.addr_,here_.port_,vbh->pk_num,vbh->target_id.addr_,vbh->info.tx, vbh->info.ty,vbh->info.tz,vbh->range);

		// printf("Vectorbasedvoidavoidance(%d) the traget address is %d\n",THIS_NODE.addr_,vbh->sender_id.addr_);

    from_nodeAddr = vbh.GetSenderAddr();
		if (GetNetDevice()->GetAddress() == from_nodeAddr) {
			// come from the same node, broadcast it
			PacketStatusTable.PutInHash(source,pkt_num,CENTER_FORWARDED);
			m_voidAvoidanceBuffer.CopyNewPacket(pkt);

			double d3=GetNetDevice()->TransmitDistance()/ns3::SOUND_SPEED_IN_WATER;
			// Ptr<Packet> pt=pkt->copy();
			double c=2*DELAY+JITTER+d3*3;

			SetShiftTimer(pkt,c);

			MACprepare(pkt);
			MACsend(pkt,m_rand->GetValue()*JITTER);
			return;
		}
		if (GetNetDevice()->GetAddress() ==vbh.GetTargetAddr())
		{
			SendDataTermination(pkt);
			PacketStatusTable.PutInHash(source,pkt_num,TERMINATED);
			DataForSink(pkt); // process it
		}

		else{

			if (IsCloseEnough(pkt)) {
				RecordPacket(&vbh);
				m_voidAvoidanceBuffer.CopyNewPacket(pkt);
				double delay=CalculateDesirableness(pkt);
				PacketStatusTable.PutInHash(source, pkt_num,SUPPRESSED);// later possibly changed in forward_timeout

				double d2=(GetNetDevice()->TransmitDistance()-Distance(pkt))/ns3::SOUND_SPEED_IN_WATER;
				double d3=(GetNetDevice()->TransmitDistance())/ns3::SOUND_SPEED_IN_WATER;
				double d4=m_rand->GetValue()*JITTER;
				SetForwardDelayTimer(pkt,(sqrt(delay)*DELAY+d2*2+d3+d4));
			} else {
				// put the data packet into its buffer to wait for void-avoidance use
				//!!!!!!!!!!!!!!!  reconsider this action
				RecordPacket(&vbh);
				m_voidAvoidanceBuffer.AddNewPacket(pkt);
			}
		}
		break;

	default:
		pkt=0;
		break;
	}
}


bool AquaSimVBVA::IsNewlyTouchedNode(AquaSimAddress source, unsigned int pkt_num)
{
  unsigned int * statusPtr=PacketStatusTable.GetHash(source, pkt_num);
  if(statusPtr) return false;
  return true;

  /*
  unsigned int * statusPtr=PacketStatusTable.GetHash(source, pkt_num);
  if(statusPtr) return false;
  */

  /* old version of newly touched
  neighborhood*  ptr=PktTable.GetHash(source, pkt_num);
  if(!ptr) return true;
  int num=ptr->number;
  if((num==1)&&(ptr->neighbor[0].forwarder_id==forwarder.addr_)) return true;
  return false;
  */
}



void AquaSimVBVA::Reset()
{
  PktTable.Reset();
  /*
  for (int i=0; i<MAX_DATA_TYPE; i++) {
    routing_table[i].reset();
  }
  */
}


void AquaSimVBVA::Terminate()
{
  NS_LOG_DEBUG("AquaSimVBVA::Terminate: Node=" << GetNetDevice()->GetAddress() <<
    ": remaining energy=" << GetNetDevice()->EnergyModel()->GetEnergy() <<
    ", initial energy=" << GetNetDevice()->EnergyModel()->GetInitialEnergy());
}

void AquaSimVBVA::MACprepare(Ptr<Packet> pkt)
{
  AquaSimHeader ash;
  VBHeader vbh;
  pkt->RemoveHeader(ash);
  pkt->RemoveHeader(vbh);

  vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));

	ash.SetErrorFlag(false);
  // printf("vectorbased: the mac_Broadcast is:%d\n",MAC_BROADCAST);
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  //ash->addr_type() = NS_AF_ILINK;
  // ash.SetTxTime(Seconds(0));
  // printf("vectorbased: the address type is :%d and suppose to be %d and  nexthop %d MAC_BROAD %d\n", cmh->addr_type(),NS_AF_ILINK,cmh->next_hop(),MAC_BROADCAST);
  ash.SetDirection(AquaSimHeader::DOWN);
  // ash->ptype_==PT_UWVB;
  // printf("vectorbased: the packet type is :%d\n", cmh->ptype_);
  //  printf("ok\n");

  //if (node) printf("ok, node is not empty\n");
  //printf("vectorbasedvoidavoidance: inside MACprepare%d %d %d \n",node->X(),node->Y(),node->Z());


  // iph->src_ = here_;
  //iph->dst_.addr_ = MAC_BROADCAST;
  //iph->dst_.port_ = ROUTING_PORT;

  //  vbh->num_next = 1;
  // vbh->next_nodes[0] = MAC_BROADCAST;

  Vector3D f;
  if(!GetNetDevice()->GetSinkStatus()) {       //!! I add new part
    f = Vector3D(GetNetDevice()->CX(),
                  GetNetDevice()->CY(),
                  GetNetDevice()->CZ());
  }
  else{
    Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
    f = Vector3D(model->GetPosition().x,
                  model->GetPosition().y,
                  model->GetPosition().z);
}
vbh.SetExtraInfo_f(f);

pkt->AddHeader(ash);
pkt->AddHeader(vbh);

}


void AquaSimVBVA::MACsend(Ptr<Packet> pkt, double delay)
{
  AquaSimHeader ash;
  VBHeader vbh;
  //AquaSimPtTag ptag;
  pkt->RemoveHeader(ash);
  pkt->RemoveHeader(vbh);
  //pkt->RemovePacketTag(ptag);

  // ash->size() +=m_controlPacketSize;
  pkt->AddHeader(ash);
  pkt->AddHeader(vbh);
  Simulator::Schedule(Seconds(delay),&AquaSimRouting::SendDown,this,
                        pkt,ash.GetNextHop(),Seconds(0));
}

bool AquaSimVBVA::IsControlMessage(const Ptr<Packet> pkt)
{
  VBHeader vbh;
  pkt->PeekHeader(vbh);
  if ((vbh.GetMessType() == AS_DATA)||(vbh.GetMessType()==FLOODING))
      return false;
  else
      return true;
}

bool AquaSimVBVA::IsUpstreamNode(const Vector3D& mp, const Vector3D& fp, const Vector3D& tp)
{

 /*double dtx=tp.x-mp.x;
 double dty=tp.y-mp.y;
 double dtz=tp.z-mp.z;
  unused variables */

 // double mydistance= sqrt((dtx*dtx)+(dty*dty)+ (dtz*dtz));
 double dis=CalculateMappedDistance(&mp,&tp,&fp);
 if(dis>0) return true;
 else return false;
}



void AquaSimVBVA::DataForSink(Ptr<Packet> pkt)
{
  if (!SendUp(pkt))
    NS_LOG_WARN("DataForSink: Something went wrong when passing packet up to dmux.");
}

/*
void AquaSimVBVA::trace (char *fmt,...)
{
  va_list ap;

  if (!tracetarget)
    return;

  va_start (ap, fmt);
  vsprintf (tracetarget->pt_->buffer(), fmt, ap);
  tracetarget->pt_->dump ();
  va_end (ap);
}
*/

void AquaSimVBVA::SetForwardDelayTimer(Ptr<Packet> pkt, double c)
{
  NS_LOG_FUNCTION(this << GetNetDevice()->GetAddress() << Simulator::Now().GetSeconds());
  Simulator::Schedule(Seconds(c),&AquaSimVBVA::ProcessForwardTimeout,this,pkt);
}


void AquaSimVBVA::SetShiftTimer(Ptr<Packet> pkt, double c)
{
	if(!pkt) {
		// printf("Vectorbasedvoidavoidance: node(%d) the packet does exist in the set_shifter_timer \n",here_.addr_);
		return;
	}

  VBHeader vbh;
  pkt->PeekHeader(vbh);

	AquaSimAddress source=vbh.GetSenderAddr();
	unsigned int pkt_num=vbh.GetPkNum();
	Vector3D s_position,t_position;

	unsigned int* status=PacketStatusTable.GetHash(source,pkt_num);
	if(!status) {
		//   printf("Vectorbasedvoidavoidance: node(%d) the packet status does exist in the set_shifter_timer \n",here_.addr_);
		return;
	}


//  added by peng xiw 20071118


/*
   s_position.x=vbh->info.ox;
   s_position.y=vbh->info.oy;
   s_position.z=vbh->info.oz;
 */

	s_position.x=vbh.GetExtraInfo().f.x;
	s_position.y=vbh.GetExtraInfo().f.y;
	s_position.z=vbh.GetExtraInfo().f.z;

	t_position.x=vbh.GetExtraInfo().t.x;
	t_position.y=vbh.GetExtraInfo().t.y;
	t_position.z=vbh.GetExtraInfo().t.z;

	//printf("vectorbased: node(%d) set v-shift  timer sp (%f,%f,%f) tp (%f,%f,%f) \n",here_.addr_,vbh->info.ox,vbh->info.oy,vbh->info.oz,vbh->info.tx,vbh->info.ty,vbh->info.tz);

	Ptr<Packet> p=GenerateVectorShiftPacket(&source, pkt_num,&s_position,&t_position);

	if(!p) {
    NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
      " can not generate v_shift data");
		return;
	}
  VBHeader vbh1;
  p->PeekHeader(vbh1);
	if((*status)==CENTER_FORWARDED) vbh1.SetMessType(EXPENSION);

	//printf ("vectorbasedvoidavoidance (%d): sets void_avoidance timer at %f and delay is %f\n ",here_.addr_,NOW,c);
  Simulator::Schedule(Seconds(c),&AquaSimVBVA::ProcessVoidAvoidanceTimeout,this,p);
	return;
}



void AquaSimVBVA::ProcessBackpressureTimeout(Ptr<Packet> pkt)
{
	//printf ("vectorbasedvoidavoidance (%d): processp  back pressure timeout at %f\n ",here_.addr_,NOW);
	if(!pkt) {
		// printf ("vectorbasedvoidavoidance %d: back pressure packet is null\n ",here_.addr_);
		return;
	}

  VBHeader vbh;
  AquaSimHeader ash;
  pkt->PeekHeader(vbh);
  pkt->PeekHeader(ash);

	int size=ash.GetSize();

	AquaSimAddress source=vbh.GetSenderAddr();
	AquaSimAddress forward=vbh.GetForwardAddr();

	unsigned int pkt_num=vbh.GetPkNum();

	//neighbornode* forwarder_list;	//unused
	//int num_of_forwarder;	//unused

	unsigned int * statusPtr=PacketStatusTable.GetHash(source, pkt_num);
	if(!statusPtr) {
    NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
      " The packet is already terminated!");
		pkt=0;
		return;
	}

	if ((*statusPtr)==TERMINATED) {
    NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
      " The packet is already terminated!");
		pkt=0;
		return;
	}



	if(IsEndNode(source, pkt_num)) {
    NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
      " is an end node size is " << size);

		MACprepare(pkt);
		MACsend(pkt,0);
		PacketStatusTable.PutInHash(source,pkt_num,TERMINATED);
	} else {
    NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() << " is not a end node");
		pkt=0;
	}

	return;
}


void AquaSimVBVA::ProcessSelfcenteredTimeout(Ptr<Packet> pkt)
{
  // printf ("vectorbasedvoidavoidance(%d):self-centered timer expires! at %f\n ",here_.addr_,NOW);
  if(!pkt){}// printf ("vectorbasedvoidavoidance(%d): the packet doesn't exist \n ",here_.addr_);

    //neighbornode* forwarder_list;	//unused
    //int num_of_forwarder;	//unused
    AquaSimAddress source;
    AquaSimAddress forward;
    unsigned int pkt_num;
    VBHeader vbh;
    pkt->PeekHeader(vbh);

    source=vbh.GetSenderAddr();
    forward=vbh.GetForwardAddr();
    pkt_num=vbh.GetPkNum();

    unsigned int  *statusPtr=PacketStatusTable.GetHash(source, pkt_num);

    if(!statusPtr) {
      NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
        " The packet status is null!");
    pkt=0;
    return;
    }

    if(((*statusPtr)==TERMINATED)||((*statusPtr)==FLOODED))
    {
      NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
        " The packet is already terminated or self-center forwarded!");
    pkt=0;
    return;
     }

    /*
    neighborhood *hashPtr=PktTable.GetHash(source, pkt_num);
    int num=hashPtr->number;
    neighbornode* neighbor_list=hashPtr->neighbor;
    */

    if (!IsWorthFloodingForward(source,pkt_num))
     {
       NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
         " is not worth forwarding this packet");

     //????????????????????????
     //      PacketStatusTable.PutInHash(source,pkt_num, SUPPRESSED);
     //       PacketStatusTable.DeleteHash(source,pkt_num);
      pkt=0;
      return;
    }

    //        printf ("vectorbasedvoidavoidance(%d): is worth forwarding this packet\n ",here_.addr_);
          double d3=(GetNetDevice()->TransmitDistance())/ns3::SOUND_SPEED_IN_WATER;
          double d4=m_rand->GetValue()*JITTER;
          SetShiftTimer(pkt,(sqrt(m_miniThreshold)*DELAY*2+d3*3+d4));


	  /*
	Ptr<Packet> pt=GenerateBackpressurePacket(&source,pkt_num);
	PktTable.DeleteHash(source, pkt_num);

            double d3=(GetNetDevice()->TransmitDistance())/ns3::SOUND_SPEED_IN_WATER;

            double d4=m_rand->GetValue()*JITTER;

	    //   double c=DELAY*sqrt(m_miniThreshold)+JITTER+d3*3+d4;
            double c=DELAY*sqrt(3.0)*4.0+d3*3+d4;

        Simulator::Schedule(Seconds(c),&AquaSimVBVA::ProcessBackpressureTimeout,this,pt);
	  */

     if(*statusPtr==VOID_SUPPRESSED)PacketStatusTable.PutInHash(source,pkt_num, CENTER_FORWARDED);
     else PacketStatusTable.PutInHash(source,pkt_num, FORWARDED);

     MACprepare(pkt);
     MACsend(pkt,0);
       return;
}


void AquaSimVBVA::ProcessForwardTimeout(Ptr<Packet>  pkt)
{
  //printf("vectorbased: node (%d) pkt  self-adaption timeout at %f\n", here_.addr_,NOW);
  VBHeader vbh;
  pkt->PeekHeader(vbh);
 unsigned char msg_type =vbh.GetMessType();
 neighborhood  *hashPtr;
 //int c=0;	//unused
 double tdelay=CalculateDesirableness(pkt);
 //double td=tdelay;	//unused
 AquaSimAddress source=vbh.GetSenderAddr();
 unsigned int pkt_num=vbh.GetPkNum();
 //unsigned int status;	//unused
     //unsigned int  *statusPtr= PacketStatusTable.GetHash(source, pkt_num);	//unused
     int ncounter=0;

     Vector3D tsp,ttp,tmp,tfp;

     tsp.x=vbh.GetExtraInfo().o.x;
     tsp.y=vbh.GetExtraInfo().o.y;
     tsp.z=vbh.GetExtraInfo().o.z;


     ttp.x=vbh.GetExtraInfo().t.x;
     ttp.y=vbh.GetExtraInfo().t.y;
     ttp.z=vbh.GetExtraInfo().t.z;

     tfp.x=vbh.GetExtraInfo().f.x;
     tfp.y=vbh.GetExtraInfo().f.y;
     tfp.z=vbh.GetExtraInfo().f.z;

     tmp.x=GetNetDevice()->CX();
     tmp.y=GetNetDevice()->CY();
     tmp.z=GetNetDevice()->CZ();

     //     td=CalculateDelay(&tfp,&ttp,&tmp,&tfp);

     /*
     if(statusPtr){
    printf ("vectorbasedvoidavoidance(%d): The packet is already processed!\n ",here_.addr_);
  pkt=0;
  return;
     }
     */

     // printf("vectorbased: node (%d) pkt %d self-adaption timeout at %f\n", here_.addr_,pkt_num,NOW);
 switch (msg_type){
 case AS_DATA:
       hashPtr= PktTable.GetHash(vbh.GetSenderAddr(), vbh.GetPkNum());

	if (hashPtr) {
          int num_neighbor=hashPtr->number;
	  Vector3D mysp,myep;
          int i=0;

	  mysp.x=vbh.GetExtraInfo().o.x;
          mysp.y=vbh.GetExtraInfo().o.y;
          mysp.z=vbh.GetExtraInfo().o.z;

          myep.x=vbh.GetExtraInfo().t.x;
          myep.y=vbh.GetExtraInfo().t.y;
          myep.z=vbh.GetExtraInfo().t.z;


	       Vector3D  sp,fp;
	       tdelay=1000;

	       //printf("vectorbased: node (%d) self-adaption, num of neighbor is %d\n", here_.addr_,num_neighbor);
	         while (i<num_neighbor){
		     sp=hashPtr->neighbor[i].vec.start;
                     fp=hashPtr->neighbor[i].node;
		     //printf("vectorbased: node (%d) self-adaption, sp.x=  %f sp.y=%f and sp.z=%f, fp.x=%f, fp.y=%f and fp.z=%f, myposition is (%f,%f,%f)\n", here_.addr_,sp.x, sp.y,sp.z,fp.x, fp.y,fp.z,node->X(), node->Y(),node->Z());
		     double t2delay=CalculateDelay(pkt,&fp);
		     //printf("vectorbased: node (%d) self-adaption, t2delay is  %f\n", here_.addr_,t2delay);
		 if (t2delay<tdelay) tdelay=t2delay;
		 	 i++;
		 }
		 ncounter=i;
	}
		if(ncounter>0) ncounter--; // delete my first packet record

		m_priority=m_miniThreshold/pow(2.0,ncounter);
                // m_priority=m_miniThreshold;


               if(tdelay<=m_priority) {
		 // printf("vectorbased: node (%d) is still worth forwarding the data packet c=%d and tdelay=%f \n", here_.addr_,ncounter,tdelay);
double d3=(GetNetDevice()->TransmitDistance())/ns3::SOUND_SPEED_IN_WATER;
            double d4=m_rand->GetValue()*JITTER;

           SetShiftTimer(pkt,(sqrt(m_miniThreshold)*DELAY*2+d3*3+d4));
           PacketStatusTable.PutInHash(source, pkt_num,FORWARDED);
               MACprepare(pkt);
               MACsend(pkt,0);
		 } else{
		   //printf("vectorbased: node (%d) is not worth forwarding the data packet c=%d and tdelay=%f \n", here_.addr_,c,tdelay);
// PktTable.MarkNextHopStatus(vbh->sender_id, vbh->pk_num,forwarder_id, SUPPRESSED);//??
// PacketStatusTable.PutInHash(source, pkt_num,SUPPRESSED);
//  if(ncounter==0)     PacketStatusTable.DeleteHash(source, pkt_num);
//  if(td>2.0)     PacketStatusTable.DeleteHash(source, pkt_num);
	   pkt=0; //to much overlap, don't send
	       }
	break;
 default:
       break;
 }
}


//not necessary
void AquaSimVBVA::MakeCopy(Ptr<Packet> pkt)
{
   Ptr<Packet> p1=pkt->Copy();
   m_voidAvoidanceBuffer.AddNewPacket(pkt);
}


void AquaSimVBVA::SendFloodingPacket(Ptr<Packet> pkt)
{
  VBHeader vbh;
  pkt->RemoveHeader(vbh);

  vbh.SetMessType(FLOODING);

  Vector3D node = Vector3D(GetNetDevice()->CX(),
                            GetNetDevice()->CY(),
                            GetNetDevice()->CZ());
  vbh.SetExtraInfo_f(node);
  vbh.SetExtraInfo_o(node);

  vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
  pkt->AddHeader(vbh);
  //     printf ("vectorbasedvoidavoidance(%d): sends the flooding packet at %f !\n ",here_.addr_, NOW);
  MACprepare(pkt);
  MACsend(pkt,0);
  return;
}


void AquaSimVBVA::ProcessVoidAvoidanceTimeout(Ptr<Packet> pkt)
{
  //printf ("vectorbasedvoidavoidance(%d): void_avoidance timeout at  %f !\n ",here_.addr_, NOW);

  if(!pkt) {
    //printf ("vectorbasedvoidavoidance(%d): void_avoidance timeout at  %f ! and the packet is empty\n ",here_.addr_, NOW);
 return;
  }
  VBHeader vbh;
  pkt->PeekHeader(vbh);
    AquaSimAddress source=vbh.GetSenderAddr();
    AquaSimAddress forward=vbh.GetForwardAddr();
    unsigned int pkt_num=vbh.GetPkNum();

    Vector3D t_p;
    Vector3D sp;
    unsigned int  * statusPtr=0;


    t_p.x=vbh.GetExtraInfo().t.x;
    t_p.y=vbh.GetExtraInfo().t.y;
    t_p.z=vbh.GetExtraInfo().t.z;

    sp.x=vbh.GetExtraInfo().o.x;
    sp.y=vbh.GetExtraInfo().o.y;
    sp.z=vbh.GetExtraInfo().o.z;

    //neighbornode* forwarder_list;	//unused
    //int num_of_forwarder;	//unused

    // printf ("vectorbasedvoidavoidance(%d): the timer for v_shift expires at %f !\n ",here_.addr_, NOW);

    statusPtr= PacketStatusTable.GetHash(source, pkt_num);

     if(statusPtr && (((*statusPtr)==TERMINATED) ||((*statusPtr)==FLOODED)))  {
       NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
         " The packet is already terminated!");
 pkt=0;
  return;
     }


       if (IsVoidNode(source,pkt_num,&sp)){

     //         if (IsVoidNode(source,pkt_num)){
         NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() << " is void node");

        PacketStatusTable.PutInHash(source,pkt_num,FLOODED);

	// Ptr<Packet> pt=GenerateBackpressurePacket(pkt);

	// MACprepare(pkt);
        MACsend(pkt, 0);


        Ptr<Packet> pdata=m_voidAvoidanceBuffer.LookupCopy(source,pkt_num);

   if(!pdata){
     NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
       " the data packet referenced by this flooding packet does not exist");
     return;
   }

	//	Ptr<Packet> pt=GenerateBackpressurePacket(&source,pkt_num);

        Ptr<Packet> pt=GenerateBackpressurePacket(pdata);

	PktTable.DeleteHash(source, pkt_num);
        double d3=(GetNetDevice()->TransmitDistance())/ns3::SOUND_SPEED_IN_WATER;

            double d4=m_rand->GetValue()*JITTER;
	    //   double c=DELAY*sqrt(m_miniThreshold)+JITTER+d3*3+d4;
            double c=DELAY*sqrt(3.0)*4.0+d3*3+d4;

       if(!pt){
         NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
           " can not generate backpressure packet");
 return;
       } else {
         NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
          " set timer  backpressure packet(" << pkt_num << ") delay=" <<
          c << " at " << Simulator::Now().GetSeconds());
          Simulator::Schedule(Seconds(c),&AquaSimVBVA::ProcessBackpressureTimeout,this,pt);
      return;
       }
    } else{
      NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() << " is not a void node");
   pkt=0;
   return;
     }
}



void AquaSimVBVA::SendDataTermination(const Ptr<Packet> p)
{
  VBHeader vbh2;
  p->PeekHeader(vbh2);
     AquaSimAddress source=vbh2.GetSenderAddr();
     unsigned int pkt_num=vbh2.GetPkNum();

      DataTerminationPktTable.PutInHash(&vbh2);


      Ptr<Packet>  pkt=Create<Packet>();

      VBHeader vbh;
      AquaSimHeader ash;
      AquaSimPtTag ptag;
      //hdr_ip* iph = HDR_IP(pkt);


      ptag.SetPacketType(AquaSimPtTag::PT_UWVB);
      ash.SetSize(m_controlPacketSize*8);
      ash.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
      ash.SetDAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
      /*iph->src_=here_;
      iph->dst_.addr_=here_.addr_;
      iph->dst_.port_=255;
      */

      vbh.SetMessType(DATA_TERMINATION);
      vbh.SetPkNum(pkt_num);
      vbh.SetTs(Simulator::Now().ToDouble(Time::S));
      vbh.SetSenderAddr(source);
      vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));


      Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
      vbh.SetExtraInfo_o(model->GetPosition());
      vbh.SetExtraInfo_f(model->GetPosition());

        ash.SetErrorFlag(false);
        ash.SetNextHop(AquaSimAddress::GetBroadcast());
        //ash->addr_type() = NS_AF_ILINK;
        ash.SetDirection(AquaSimHeader::DOWN);

        pkt->AddHeader(ash);
        pkt->AddHeader(vbh);
        pkt->AddPacketTag(ptag);
        MACsend(pkt, 0);
        NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
          " send data termination " << pkt_num << " at " << Simulator::Now().GetSeconds());
}

// Is sp useful?
Ptr<Packet> AquaSimVBVA::GenerateVectorShiftPacket(const AquaSimAddress* source,
                                                    int pkt_num,
                                                    const Vector3D* sp,
                                                    const Vector3D* tp)
{
  //printf("Vectorbasedvoidavoidance node (%d) generates V_Shift\n",here_.addr_);

      Ptr<Packet> v_shift=Create<Packet>();

 if(!v_shift) {
   NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
     " can't not generate v_shift packet since the data packet is empty");
return NULL;
  }

  NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
    " generate v-shift sp (" << sp->x << "," << sp->y << "," << sp->z <<
    ") tp (" << tp->x << "," << tp->y << "," << tp->z << ")");

    VBHeader vbh;
    AquaSimHeader ash;
    AquaSimPtTag ptag;
    //hdr_ip* iph = HDR_IP(pkt);


    ptag.SetPacketType(AquaSimPtTag::PT_UWVB);
    ash.SetSize(m_controlPacketSize*8);
    ash.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    ash.SetDAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    /*iph->src_=here_;
    iph->dst_.addr_=here_.addr_;
    iph->dst_.port_=255;
    */

    vbh.SetMessType(V_SHIFT);
    vbh.SetPkNum(pkt_num);
    vbh.SetTs(0);
    vbh.SetSenderAddr(*source);
    vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));


    Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
    //vbh.SetExtraInfo_o(model->GetPosition());        // delete by peng xie 20071118
    vbh.SetExtraInfo_f(model->GetPosition());

    Vector3D spos = Vector3D(sp->x,sp->y,sp->z);
    Vector3D tpos = Vector3D(tp->x,tp->y,tp->z);
    vbh.SetExtraInfo_o(spos);
    vbh.SetExtraInfo_t(tpos);

    vbh.SetOriginalSource(*sp);

      ash.SetErrorFlag(false);
      ash.SetNextHop(AquaSimAddress::GetBroadcast());
      //ash->addr_type() = NS_AF_ILINK;
      ash.SetDirection(AquaSimHeader::DOWN);

      v_shift->AddHeader(ash);
      v_shift->AddHeader(vbh);
      v_shift->AddPacketTag(ptag);
	return v_shift;
}


Ptr<Packet> AquaSimVBVA::GenerateControlDataPacket(Ptr<Packet> packet, unsigned int type)
{
 if(!packet) {
   //printf("Vectorbasedvoidavoidance node (%d) can't not generate control data packet since the data packet is empty\n",here_.addr_);
return NULL;
  }

      Ptr<Packet> vD=packet->Copy();

if(!vD) {
  NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
    " can't not generate control data packet due to the failure of meme allocation");
return NULL;
  }

  VBHeader vbh;
  AquaSimHeader ash;
  AquaSimPtTag ptag;

  ptag.SetPacketType(AquaSimPtTag::PT_UWVB);

  /*
  iph->src_=here_;
  iph->dst_.addr_=here_.addr_;
  iph->dst_.port_=255;
  */
  vbh.SetMessType(type);
  vbh.SetTs(0);
  vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));

  ash.SetErrorFlag(false);
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  //ash->addr_type() = NS_AF_ILINK;
  ash.SetDirection(AquaSimHeader::DOWN);

  vD->AddHeader(ash);
  vD->AddHeader(vbh);
  vD->AddPacketTag(ptag);
	return vD;
}


Ptr<Packet> AquaSimVBVA::GenerateBackpressurePacket(Ptr<Packet> packet)
{

  if(!packet) {
    //printf("Vectorbasedvoidavoidance node (%d) can't not generate backpressure since the data packet is empty\n",here_.addr_);
return NULL;
  }
      Ptr<Packet> backpressure=packet->Copy();

 if(!backpressure) {
   NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
     " can't not generate backpressure due to the meme allocation");
return NULL;
  }

  VBHeader vbh;
  AquaSimHeader ash;
  AquaSimPtTag ptag;

  ptag.SetPacketType(AquaSimPtTag::PT_UWVBVA);

  ash.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
  ash.SetDAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
  /*iph->src_=here_;
  iph->dst_.addr_=here_.addr_;
  iph->dst_.port_=255;
  */
  vbh.SetMessType(BACKPRESSURE);
  vbh.SetTs(0);
  vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));

  ash.SetErrorFlag(false);
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  //ash->addr_type() = NS_AF_ILINK;
  ash.SetDirection(AquaSimHeader::DOWN);

  backpressure->AddHeader(ash);
  backpressure->AddHeader(vbh);
  backpressure->AddPacketTag(ptag);
	return backpressure;
}


double AquaSimVBVA::CalculateFloodingDesirableness(const Ptr<Packet> pkt)
{

   double d1=Distance(pkt);
   double dt=GetNetDevice()->TransmitDistance();
   double dr=dt-d1;
   if(dr<0) dr=0.0; // in case of location error
   double d2=dr/ns3::SOUND_SPEED_IN_WATER;

     return (dr/dt)*DELAY+2*d2;

}

double AquaSimVBVA::CalculateDesirableness(const Ptr<Packet> pkt)
{
  VBHeader vbh;
  pkt->PeekHeader(vbh);
    Vector3D sp,tp,fp, mp;


	sp.x=vbh.GetExtraInfo().o.x;
        sp.y=vbh.GetExtraInfo().o.y;
        sp.z=vbh.GetExtraInfo().o.z;


   tp.x=vbh.GetExtraInfo().t.x;
   tp.y=vbh.GetExtraInfo().t.y;
   tp.z=vbh.GetExtraInfo().t.z;

   fp.x=vbh.GetExtraInfo().f.x;
   fp.y=vbh.GetExtraInfo().f.y;
   fp.z=vbh.GetExtraInfo().f.z;


   mp.x=GetNetDevice()->CX();
   mp.y=GetNetDevice()->CY();
   mp.z=GetNetDevice()->CZ();

   return CalculateDelay(&sp,&tp,&mp,&fp);
}


bool AquaSimVBVA::IsStuckNode(const neighbornode* neighbor_list,const Vector3D* tp,
                              int num_of_neighbor, unsigned int status )
{

  Vector3D mp;

  mp.x=GetNetDevice()->CX();
  mp.y=GetNetDevice()->CY();
  mp.z=GetNetDevice()->CZ();

  double mydis=Distance(tp,&mp);
  //  unsigned int * statusPtr= PacketStatusTable.GetHash(source,pk_num);


    for(int i=0;i<num_of_neighbor;i++){
	Vector3D fp=neighbor_list[i].node;
        double tmp_dis=Distance(tp, &fp);
        if ((tmp_dis<mydis)&&(neighbor_list[i].status!=DEAD)) return false;
    }
    if((status==FORWARDED)||(status==CENTER_FORWARDED)) return true;
    if(status==FLOODED){
    for(int i=0;i<num_of_neighbor;i++){
	Vector3D fp=neighbor_list[i].node;
        Vector3D sp=neighbor_list[i].vec.start;
        if ((IsSamePosition(&fp,&sp))&&(neighbor_list[i].status!=DEAD)) return false;
    }
    }
    return true;
}


bool AquaSimVBVA::IsWorthFloodingForward(AquaSimAddress source, int pkt_num)
{
  NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
    " is determining if it worth flooding forward");

    neighborhood *hashPtr=PktTable.GetHash(source, pkt_num);
    if(!hashPtr) return true;

    int num=hashPtr->number;
     neighbornode* neighbor_list=hashPtr->neighbor;

    // to check if there is some one self-centered sending the packet before me??
    for(int i=0;i<num;i++){
	Vector3D fp=neighbor_list[i].node;
        Vector3D sp=neighbor_list[i].vec.start;
        // added by peng xie
	unsigned fid=neighbor_list[i].forwarder_id;

	//added by peng xie 20071118
     if(IsSamePosition(&fp,&sp)&&((fid!=source))) return false;
    }
    return true;
}



bool AquaSimVBVA::IsEndNode(AquaSimAddress source,int pkt_num)
{
  NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
    " is determining if it is an end node");

    neighborhood *hashPtr= PktTable.GetHash(source, pkt_num);
    if(!hashPtr) return true;

     neighbornode* neighbor_list= hashPtr->neighbor;
     int num_of_neighbor=hashPtr->number;

    for(int i=0;i<num_of_neighbor;i++){
	Vector3D fp=neighbor_list[i].node;
        Vector3D op=neighbor_list[i].vec.start;
        //unsigned int forwarder_id=neighbor_list[i].forwarder_id;	//unused
	//printf("vectorbased: node(%d)  fp.x=%f,fp.y=%f and pf.z=%f op.x=%f, op.y=%f and op.z=%f\n",here_.addr_,fp.x,fp.y,fp.z,op.x,op.y,op.z);

        if (IsSamePosition(&op,&fp)) return false;
    }
    return true;
}




bool AquaSimVBVA::IsVoidNode(AquaSimAddress source, int pkt_num,const Vector3D* sp)
{
  NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
    " is determining if it is void node");

    neighborhood *hashPtr= PktTable.GetHash(source, pkt_num);

    if (!hashPtr) return true;

    neighbornode* neighbor_list= hashPtr->neighbor;
    int num_of_neighbor=hashPtr->number;
     Vector3D mp;
     //  Vector3D sp=neighbor_list[0].vec.start;
      Vector3D tp=neighbor_list[0].vec.end;

     mp.x=GetNetDevice()->CX();
    mp.y=GetNetDevice()->CY();
    mp.z=GetNetDevice()->CZ();
    NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() << " my position ("
		<< mp.x << "," << mp.y << "," << mp.z << ") sp is ("
		<< sp->x << "," << sp->y << "," << sp->z << ") tp is("
		<< tp.x << "," << tp.y << "," << tp.z << ")" );
 double myadvance=CalculateMappedDistance(sp,&tp,&mp);

     NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
       " is determining if it is void node, # of neighbor is " << num_of_neighbor <<
       " dis is " << myadvance); //"
    if(num_of_neighbor<=1) return true; // I only has one packet record in my hashtable
    for(int i=0;i<num_of_neighbor;i++){
      Vector3D fp=neighbor_list[i].node;

      double advance=CalculateMappedDistance(sp,&tp,&fp);

      //   printf("neighbor position (%f %f %f) dis is %f\n",fp.x,fp.y,fp.z,advance);
        if (advance>myadvance) return false;
    }
    return true;
}


// added by peng xie at 20071106 sp is start point and tp is the target

double AquaSimVBVA::CalculateMappedDistance(const Vector3D*  sp,
                                            const Vector3D* tp,
                                            const Vector3D* fp)
{

  if(IsSamePosition(sp, fp)) return 0.0;
 double fx=sp->x;
 double fy=sp->y;
 double fz=sp->z;

 double dx=fp->x-fx;
 double dy=fp->y-fy;
 double dz=fp->z-fz;


 double tx=tp->x;
 double ty=tp->y;
 double tz=tp->z;

 double dtx=tx-fx;
 double dty=ty-fy;
 double dtz=tz-fz;

 double dp=dx*dtx+dy*dty+dz*dtz;

 double d=sqrt((dx*dx)+(dy*dy)+ (dz*dz));
 double l=sqrt((dtx*dtx)+(dty*dty)+ (dtz*dtz));
 double cos_theta=dp/(d*l);

    double mdis=d*cos_theta;
      return mdis;
}

bool AquaSimVBVA::IsVoidNode(AquaSimAddress source, int pkt_num)
{
  NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
    " is determining if it is void node");

    neighborhood *hashPtr= PktTable.GetHash(source, pkt_num);

    if (!hashPtr) return true;

    neighbornode* neighbor_list= hashPtr->neighbor;
    int num_of_neighbor=hashPtr->number;

    Vector3D mp,tp;

    mp.x=GetNetDevice()->CX();
    mp.y=GetNetDevice()->CY();
    mp.z=GetNetDevice()->CZ();

    tp=neighbor_list[0].vec.end;

    double mydis=Distance(&tp,&mp);
    for(int i=0;i<num_of_neighbor;i++){
	Vector3D fp=neighbor_list[i].node;
        double tmp_dis=Distance(&tp, &fp);
        if (tmp_dis<mydis) return false;
    }
    return true;
}

double AquaSimVBVA::CalculateSelfCenteredDelay(const Vector3D* sp,
                                                const Vector3D*  tp,
                                                const Vector3D* myp,
                                                const Vector3D* fp)
{
  NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
    " sp(" << sp->x << "," << sp->y << "," << sp->z << ") tp (" <<
     tp->x << "," << tp->y << "," << tp->z << ") fp (" <<
     fp->x << "," << fp->y << "," << fp->z << ")");

 double fx=fp->x;
 double fy=fp->y;
 double fz=fp->z;

 double dx=myp->x-fx;
 double dy=myp->y-fy;
 double dz=myp->z-fz;


 double tx=tp->x;
 double ty=tp->y;
 double tz=tp->z;

 double dtx=tx-fx;
 double dty=ty-fy;
 double dtz=tz-fz;

 double dp=dx*dtx+dy*dty+dz*dtz;

 double p=Projection(fp,tp,myp);
 double d=sqrt((dx*dx)+(dy*dy)+ (dz*dz));
 double l=sqrt((dtx*dtx)+(dty*dty)+ (dtz*dtz));
 double cos_theta=dp/(d*l);
   double delay=(1.0-(p/m_width)) +((GetNetDevice()->TransmitDistance()-d*cos_theta)/GetNetDevice()->TransmitDistance());

 //double delay=(1.0-(p/m_width));
NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
  " projection is " << p << ", and cos is " << cos_theta << ", and d is " <<
  d << " l is " << l);
   if(delay<0.0) delay=0.0; // in case the location error, which may result in negative delay
      return sqrt(delay);
   //  return sqrt(delay)*DELAY*2.0;
}



double  AquaSimVBVA::Distance(const Vector3D* sp, const Vector3D* tp)
{
 double fx=sp->x;
 double fy=sp->y;
 double fz=sp->z;

 double tx=tp->x;
 double ty=tp->y;
 double tz=tp->z;

 double dtx=tx-fx;
 double dty=ty-fy;
 double dtz=tz-fz;

 return sqrt((dtx*dtx)+(dty*dty)+ (dtz*dtz));
}

double AquaSimVBVA::CalculateDelay(const Vector3D* sp,const Vector3D*  tp,
                                    const Vector3D* myp, const Vector3D* fp)
{

 double fx=fp->x;
 double fy=fp->y;
 double fz=fp->z;

 double dx=myp->x-fx;
 double dy=myp->y-fy;
 double dz=myp->z-fz;


 double tx=tp->x;
 double ty=tp->y;
 double tz=tp->z;

 double dtx=tx-fx;
 double dty=ty-fy;
 double dtz=tz-fz;

 double dp=dx*dtx+dy*dty+dz*dtz;

 double p=Projection(sp,tp,myp);
 double d=sqrt((dx*dx)+(dy*dy)+ (dz*dz));
 double l=sqrt((dtx*dtx)+(dty*dty)+ (dtz*dtz));
 double cos_theta=dp/(d*l);
   double delay=(p/m_width) +((GetNetDevice()->TransmitDistance()-d*cos_theta)/GetNetDevice()->TransmitDistance());

// printf("vectorbased: node(%d) projection is %f, and cos is %f, and d is %f)\n",here_.addr_,p, cos_theta, d);
   if(delay<0.0) delay=0.0; // in case the location error, which may result in negative delay
   return delay;
}


double AquaSimVBVA::Projection(const Vector3D* sp, const Vector3D* tp,const Vector3D* p)
{
// two projection functions should be merged later
 double tx=tp->x;
 double ty=tp->y;
 double tz=tp->z;


 double ox=sp->x;
 double oy=sp->y;
 double oz=sp->z;

 double x=p->x;
 double y=p->y;
 double z=p->z;

 double wx=tx-ox;
 double wy=ty-oy;
 double wz=tz-oz;

 double vx=x-ox;
 double vy=y-oy;
 double vz=z-oz;

 double cross_product_x=vy*wz-vz*wy;
 double cross_product_y=vz*wx-vx*wz;
 double cross_product_z=vx*wy-vy*wx;

 double area=sqrt(cross_product_x*cross_product_x+
          cross_product_y*cross_product_y+cross_product_z*cross_product_z);
 double length=sqrt((tx-ox)*(tx-ox)+(ty-oy)*(ty-oy)+ (tz-oz)*(tz-oz));
 // printf("vectorbasedvoidavoidance: the area is %f and length is %f\n",area,length);
 return area/length;
}


void AquaSimVBVA::CalculatePosition(Ptr<Packet> pkt)
{
  Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();

  GetNetDevice()->CX()=model->GetPosition().x;
	GetNetDevice()->CY()=model->GetPosition().y;
	GetNetDevice()->CZ()=model->GetPosition().z;

  /*
 hdr_uwvbva     *vbh  = HDR_UWVBVA(pkt);
 double fx=vbh->info.fx;
 double fy=vbh->info.fy;
 double fz=vbh->info.fz;

 double dx=vbh->info.dx;
 double dy=vbh->info.dy;
 double dz=vbh->info.dz;

 node->CX_=fx+dx;
 node->CY_=fy+dy;
 node->CZ_=fz+dz;
 printf("vectorbasedvoidavoidance: my position is computed as (%f,%f,%f) dx=%f dy=%f and dz=%f \n",node->CX_, node->CY_,node->CZ_,dx,dy,dz);
  */
}

double AquaSimVBVA::CalculateDelay(Ptr<Packet> pkt,Vector3D* p1)
{
  VBHeader vbh;
  pkt->PeekHeader(vbh);
 double fx=p1->x;
 double fy=p1->y;
 double fz=p1->z;

 double dx=GetNetDevice()->CX()-fx;
 double dy=GetNetDevice()->CY()-fy;
 double dz=GetNetDevice()->CZ()-fz;

 	double tx=vbh.GetExtraInfo().t.x;
 	double ty=vbh.GetExtraInfo().t.y;
 	double tz=vbh.GetExtraInfo().t.z;

 double dtx=tx-fx;
 double dty=ty-fy;
 double dtz=tz-fz;

 double dp=dx*dtx+dy*dty+dz*dtz;

 // double a=Advance(pkt);
 double p=Projection(pkt);
 double d=sqrt((dx*dx)+(dy*dy)+ (dz*dz));
 double l=sqrt((dtx*dtx)+(dty*dty)+ (dtz*dtz));
 double cos_theta=dp/(d*l);
 // double delay=(TRANSMISSION_DISTANCE-d*cos_theta)/TRANSMISSION_DISTANCE;
   double delay=(p/m_width) +((GetNetDevice()->TransmitDistance()-d*cos_theta)/GetNetDevice()->TransmitDistance());
 // double delay=(p/m_width) +((TRANSMISSION_DISTANCE-d)/TRANSMISSION_DISTANCE)+(1-cos_theta);
  //printf("vectorbased: node(%d) projection is %f, and cos is %f, and d is %f)\n",here_.addr_,p, cos_theta, d);
   return delay;
}


double AquaSimVBVA::Distance(const Ptr<Packet> pkt)
{
  VBHeader vbh;
  pkt->PeekHeader(vbh);
  double tx=vbh.GetExtraInfo().f.x;
	double ty=vbh.GetExtraInfo().f.y;
	double tz=vbh.GetExtraInfo().f.z;
 // printf("vectorbased: the target is %lf,%lf,%lf \n",tx,ty,tz);
 double x=GetNetDevice()->CX(); //change later
 double y=GetNetDevice()->CY();// printf(" Vectorbasedforward: I am in advanced\n");
 double z=GetNetDevice()->CZ();
 NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
   " the forwarder is " << tx << "," << ty << "," << tz << " and my coordinates are " <<
   x << "," << y << "," << z);
 return sqrt((tx-x)*(tx-x)+(ty-y)*(ty-y)+ (tz-z)*(tz-z));
}

double AquaSimVBVA::Advance(Ptr<Packet> pkt)
{
  VBHeader vbh;
  pkt->PeekHeader(vbh);
  double tx=vbh.GetExtraInfo().t.x;
	double ty=vbh.GetExtraInfo().t.y;
	double tz=vbh.GetExtraInfo().t.z;
 // printf("vectorbased: the target is %lf,%lf,%lf \n",tx,ty,tz);
 double x=GetNetDevice()->CX(); //change later
 double y=GetNetDevice()->CY();// printf(" Vectorbasedforward: I am in advanced\n");
 double z=GetNetDevice()->CZ();
 // printf("the target is %lf,%lf,%lf and my coordinates are %lf,%lf,%lf\n",tx,ty,tz,x,y,z);
 return sqrt((tx-x)*(tx-x)+(ty-y)*(ty-y)+ (tz-z)*(tz-z));
}


double AquaSimVBVA::Projection(Ptr<Packet> pkt)
{
  VBHeader vbh;
  pkt->PeekHeader(vbh);
	double tx=vbh.GetExtraInfo().t.x;
	double ty=vbh.GetExtraInfo().t.y;
	double tz=vbh.GetExtraInfo().t.z;

 double ox=vbh.GetExtraInfo().o.x;
 double oy=vbh.GetExtraInfo().o.y;
 double oz=vbh.GetExtraInfo().o.z;

 double x=GetNetDevice()->CX();
 double y=GetNetDevice()->CY();
 double z=GetNetDevice()->CZ();

 double wx=tx-ox;
 double wy=ty-oy;
 double wz=tz-oz;

 double vx=x-ox;
 double vy=y-oy;
 double vz=z-oz;

 double cross_product_x=vy*wz-vz*wy;
 double cross_product_y=vz*wx-vx*wz;
 double cross_product_z=vx*wy-vy*wx;

 double area=sqrt(cross_product_x*cross_product_x+
          cross_product_y*cross_product_y+cross_product_z*cross_product_z);
 double length=sqrt((tx-ox)*(tx-ox)+(ty-oy)*(ty-oy)+ (tz-oz)*(tz-oz));
 return area/length;
}

bool AquaSimVBVA::IsTarget(Ptr<Packet> pkt)
{
  VBHeader vbh;
  pkt->PeekHeader(vbh);
  if (vbh.GetTargetAddr().GetAsInt()==0) {
    //  printf("vectorbased: advanced is %lf and my range is %f\n",advance(pkt),vbh->range);
    return (Advance(pkt)<vbh.GetRange());
  }
  else return(GetNetDevice()->GetAddress()==vbh.GetTargetAddr());
}

bool AquaSimVBVA::IsCloseEnough(Ptr<Packet> pkt)
{
  VBHeader vbh;
  pkt->PeekHeader(vbh);
  //double range=vbh.GetRange();  //unused

  Vector3D sp, tp,p;

  sp.x=vbh.GetExtraInfo().o.x;
  sp.y=vbh.GetExtraInfo().o.y;
  sp.z=vbh.GetExtraInfo().o.z;

  tp.x=vbh.GetExtraInfo().t.x;
  tp.y=vbh.GetExtraInfo().t.y;
  tp.z=vbh.GetExtraInfo().t.z;

  p.x=GetNetDevice()->CX();
  p.y=GetNetDevice()->CY();
  p.z=GetNetDevice()->CZ();

  NS_LOG_WARN("AquaSimVBVA: " << GetNetDevice()->GetAddress() <<
    " The projection is " << Projection(&sp,&tp,&p));
 if ((Projection(&sp,&tp,&p)<=m_width))  return true;
 return false;
}
