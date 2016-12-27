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

#include "aqua-sim-mac-copemac.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-mac.h"
#include "aqua-sim-pt-tag.h"
//#include "vbf/vectorbasedforward.h" //not used.

#include "ns3/log.h"
#include "ns3/integer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimCopeMac");



void
PktWareHouse::Insert2PktQs(Ptr<Packet> pkt)
{
  AquaSimHeader asHeader;
  pkt->PeekHeader(asHeader);

  PktElem* temp = new PktElem(pkt);

  m_cachedPktNum ++;
  if( Queues[asHeader.GetNextHop()].head == NULL ) {
      Queues[asHeader.GetNextHop()].tail = temp;
      Queues[asHeader.GetNextHop()].head = temp;
  }
  else {
      Queues[asHeader.GetNextHop()].tail->next_ = temp;
      Queues[asHeader.GetNextHop()].tail = temp;
  }
}

bool
PktWareHouse::DeletePkt(AquaSimAddress Recver, int SeqNum)
{
  PktElem* pos = Queues[Recver].head;
  PktElem* pre_pos = NULL;
  AquaSimHeader asHeader;

  while ( pos != NULL ) {
      pos->pkt_->PeekHeader(asHeader);
      if( asHeader.GetUId() == SeqNum ) {
	if( pre_pos == NULL ) {
	    Queues[Recver].head = pos->next_;
	}
	else {
	    pre_pos->next_ = pos->next_;
	}

	pos->pkt_=0;
	//pos->pkt_ == NULL;
	delete pos;
	m_cachedPktNum --;
	return true;
      }
      pre_pos = pos;
      pos = pos->next_;
  }

  return false;
}



/**********************RevQueues******************************************/
NS_OBJECT_ENSURE_REGISTERED(RevQueues);

RevElem::RevElem()
{
  m_sendTimer = NULL;
  next = NULL;
}

RevElem::RevElem(int RevID_, Time StartTime_, Time EndTime_,
				 AquaSimAddress Reservor_, RevType rev_type_):
	StartTime(StartTime_), EndTime(EndTime_),
	Reservor(Reservor_), rev_type(rev_type_), RevID(RevID_)
{
  m_sendTimer = NULL;
  next = NULL;
}

RevElem::~RevElem()
{
  if( m_sendTimer != NULL ) {
      if( m_sendTimer->IsRunning() ) {
	  m_sendTimer->Cancel();
      }
      delete m_sendTimer;
      m_sendTimer = NULL;
  }
}

NS_OBJECT_ENSURE_REGISTERED(PktSendTimer);

PktSendTimer::~PktSendTimer()
{
}

void
PktSendTimer::PktSendTimerExpire()
{
  m_mac->PreSendPkt(m_pkt);
}

TypeId
PktSendTimer::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::PktSendTimer")
      ;
  return tid;
}


TypeId
RevQueues::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::RevQueues")
    ;
  return tid;
}



RevQueues::RevQueues(Ptr<AquaSimCopeMac> mac): mac_(mac)
{
  Head_ = NULL;
  //m_sendTimer =
}


RevQueues::~RevQueues()
{
  RevElem* tmp = NULL;

  while( Head_ != NULL ) {
      tmp = Head_;
      Head_ = Head_->next;
      delete tmp;
  }
}

void
RevQueues::ClearExpired(Time ExpireTime)
{
  RevElem* tmp = NULL;

  while( Head_ != NULL && Head_->EndTime < ExpireTime + BACKOFF_DELAY_ERROR ) {
      tmp = Head_;
      Head_ = Head_->next;
      delete tmp;
  }
}


bool
RevQueues::Push(int RevID, Time StartTime, Time EndTime,
			AquaSimAddress Reservor, RevType rev_type, Ptr<Packet> pkt)
{
  ClearExpired(Simulator::Now());

  //insert

  RevElem* tmp = new RevElem(RevID, StartTime, EndTime, Reservor, rev_type);

  if( pkt != NULL ) {
      tmp->m_sendTimer = new PktSendTimer(mac_, pkt);
  }

  if( Head_ == NULL ) {
      Head_ = tmp;
      return true;
  }

  RevElem* pre_pos = Head_;
  RevElem* pos = Head_->next;

  while( pos != NULL ) {
      if( pos->EndTime < EndTime ) {
	  pre_pos = pos;
	  pos = pos->next;
      }
      else {
	  break;
      }
  }

  pre_pos->next = tmp;
  tmp->next = pos;
  return true;
}


bool
RevQueues::CheckAvailability(Time StartTime, Time EndTime, RevType rev_type)
{
  //recheck the code here.
  ClearExpired(Simulator::Now());

  RevElem* pos = Head_;

  while( pos != NULL ) {
    if( ((pos->StartTime > StartTime) && (pos->StartTime < EndTime) )
	    || ( (pos->EndTime > StartTime)&&(pos->EndTime<EndTime) ) ) {
	return false;
    }

    pos = pos->next;
  }
  return true;
}


void
RevQueues::DeleteRev(int RevID)
{
  RevElem* pos = Head_;

  if( Head_->RevID == RevID ) {
    Head_ = Head_->next;
    delete pos;
    return;
  }

  pos = Head_->next;
  RevElem* pre_pos = Head_;

  while( pos != NULL ) {
    if( pos->RevID == RevID ) {
	pre_pos->next = pos->next;

	delete pos;
	return;
    }
    pre_pos = pos;
    pos = pos->next;
  }

}


void
RevQueues::UpdateStatus(int RevID, RevType new_type)
{
  RevElem* pos = Head_;
  Time send_time;

  while( pos != NULL ) {
    if( pos->RevID == RevID ) {
      pos->rev_type = new_type;
      send_time = pos->StartTime - Simulator::Now() + mac_->m_guardTime/2;
      if( send_time < 0.0 ) {
	  NS_LOG_WARN("UpdateStatus: handshake time takes too long, cancel sending");
	  DeleteRev(RevID);
	  return;
      }

      if( send_time > 0.0 && pos->m_sendTimer != NULL) {
	  pos->m_sendTimer->SetFunction(&PktSendTimer::PktSendTimerExpire,pos->m_sendTimer);
	  pos->m_sendTimer->Schedule(send_time);

      }
      return;
    }
    pos = pos->next;
  }
}

void
RevQueues::PrintRevQueue()
{

  RevElem* pos = Head_;
  /*char file_name[30];
  strcpy(file_name, "schedule_");
  file_name[strlen(file_name)+1] = '\0';
  file_name[strlen(file_name)] = char(mac_->index_+'0');
  FILE* stream = fopen(file_name, "a");
  */
  while( pos != NULL ) {
      NS_LOG_INFO("PrintRevQueue: Node(" << pos->Reservor << "): " <<
		  pos->RevID << "[" << pos->StartTime << ":" <<
		  pos->EndTime << "] type:" << pos->rev_type);
    //fprintf(stream, "node(%d): %d[%f:%f] type:%d\t", pos->Reservor,
	//    pos->RevID, pos->StartTime, pos->EndTime, pos->rev_type);
    pos = pos->next;
  }
  /*fprintf(stream, "\n");
  fclose(stream);
  */
}

Time
RevQueues::GetValidStartTime(Time Interval, Time SinceTime)
{
  Time now = SinceTime;
  RevElem* pos = Head_;
  RevElem* pre_pos = NULL;
  Time Lowerbound, Upperbound;

  while( pos!=NULL && pos->StartTime < now ) {
    pre_pos = pos;
    pos = pos->next;
  }

  Lowerbound = now;

  if( pos == NULL ) {
      return SinceTime + Seconds(0.0001) - Simulator::Now();
  }
  else {
      Upperbound = pos->StartTime;
  }

  while( Upperbound - Lowerbound < Interval ) {
      pre_pos = pos;
      pos = pos->next;
      Lowerbound = pre_pos->EndTime;
      if( pos == NULL )
	  break;
      Upperbound = pos->StartTime;
  }

  return Lowerbound + Seconds(0.0001) - Simulator::Now();
}


/**********************COPEMac*************************************/
NS_OBJECT_ENSURE_REGISTERED(AquaSimCopeMac);


void AquaSimCopeMac::BackoffHandler(Ptr<Packet> pkt)
{
  m_backoffCounter++;
  if(m_backoffCounter < COPEMAC_MAXIMUMCOUNTER) {
      TxProcess(pkt);
      pkt=0;
  }
  else
    {
      m_backoffCounter=0; //clear
      NS_LOG_INFO("BackoffHandler: too many backoffs");
      /*if( mac_->drop_ )
	      mac_->drop_->recv(pkt_, "Backoff too many times");
      else*/
      pkt=0;
    }
}

int AquaSimCopeMac::RevID = 0;
AquaSimCopeMac::AquaSimCopeMac():
    DataSendTimer(Timer::CANCEL_ON_DESTROY),
    RevAckAccumTimer(Timer::CANCEL_ON_DESTROY),
    DataAckAccumTimer(Timer::CANCEL_ON_DESTROY),
    m_NDInterval(6),m_dataAccuPeriod(10),m_revAckAccumTime(1),
    m_dataAckAccumTime(1), m_RevQ(this), m_nextHop(0),
    m_neighborId(0), m_majorIntervalLB(2),/* MajorIntervalUB(3),IntervalStep(0.1),*/
    m_dataStartTime(15), m_guardTime(0.01),m_NDWin(2.0),
    m_NDReplyWin(2.0), m_ackTimeOut(10),
    m_pktSize(200), m_isParallel(1), m_NDProcessMaxTimes(3), m_backoffCounter(0)

{
  Simulator::Schedule(Seconds(0.002), &AquaSimCopeMac::NDProcessInitor, this); // start the nd process
  Simulator::Schedule(Seconds(0.001), &AquaSimCopeMac::Start, this);
}

AquaSimCopeMac::~AquaSimCopeMac()
{
}


void
AquaSimCopeMac::Start()
{
  //m_device->SetTransmissionStatus(NIDLE);

  m_dataStartTime = 3*m_NDInterval + Seconds(7);
  Time MaxRTT = Seconds(2*1000/1500.0);
  m_ackTimeOut = m_revAckAccumTime + m_dataAckAccumTime + 2*MaxRTT + Seconds(5); //5 is time error

  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
  Simulator::Schedule(m_dataAccuPeriod+Seconds(m_rand->GetValue()),&AquaSimCopeMac::DataSendTimerExpire,this);
  //Random::seed_heuristically();
}

TypeId
AquaSimCopeMac::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimCopeMac")
      .SetParent<AquaSimMac>()
      .AddConstructor<AquaSimCopeMac>()
      .AddAttribute("NDInterval", "The interval between two successive ND process",
	TimeValue(MilliSeconds(1)),
	MakeTimeAccessor (&AquaSimCopeMac::m_NDInterval),
	MakeTimeChecker ())
     .AddAttribute("DataAccuPeriod", "The period of the data pulse.",
	TimeValue(MilliSeconds(1)),
	MakeTimeAccessor (&AquaSimCopeMac::m_dataAccuPeriod),
	MakeTimeChecker ())
     .AddAttribute("RevAckAccuTime", "The accumulated time of Rev ACK.",
	TimeValue(Seconds(1)),
	MakeTimeAccessor (&AquaSimCopeMac::m_revAckAccumTime),
	MakeTimeChecker ())
     .AddAttribute("DataAckAccumTime", "The accumulated time of Data ACK.",
	TimeValue(Seconds(1)),
	MakeTimeAccessor (&AquaSimCopeMac::m_dataAckAccumTime),
	MakeTimeChecker ())
     .AddAttribute("MajorIntervalLB", "Lower bound of major interval.",
	TimeValue(MilliSeconds(1)),
	MakeTimeAccessor (&AquaSimCopeMac::m_majorIntervalLB),
	MakeTimeChecker ())
     /*.AddAttribute("MajorIntervalUB", "Upper bound of major interval.",
	TimeValue(MilliSeconds(1)),
	MakeTimeAccessor (&AquaSimCopeMac::m_majorIntervalUB),
	MakeTimeChecker ())*/
     .AddAttribute("GuardTime", "Guard time",
	TimeValue(MilliSeconds(1)),
	MakeTimeAccessor (&AquaSimCopeMac::m_guardTime),
	MakeTimeChecker ())
     .AddAttribute("IsParallel", "Is parallel (default is 0).",
	IntegerValue(0),
	MakeIntegerAccessor (&AquaSimCopeMac::m_isParallel),
	MakeIntegerChecker<int>())
    ;
  return tid;
}


bool
AquaSimCopeMac::TxProcess(Ptr<Packet> pkt)
{
  AquaSimHeader ash;
  CopeHeader ch;
  pkt->RemoveHeader(ash);
  pkt->RemoveHeader(ch);
  //vbf header

  ash.SetSize(m_pktSize);
  ash.SetTxTime(GetTxTime(ash.GetSerializedSize()+ch.GetSerializedSize()));

  if( m_propDelays.size() == 0 ) {
      NS_LOG_INFO("TxProcess: Node=" << m_device->GetNode() << "doesn't have neighbor.");
      pkt=0;
      return false;
  }

  ash.SetErrorFlag(false);
  ash.SetDirection(AquaSimHeader::DOWN);
  ch.SetDA(ash.GetNextHop());
  ch.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );

  pkt->AddHeader(ch);
  pkt->AddHeader(ash);
  //insert packet into packet warehouse
  m_PktWH.Insert2PktQs(pkt);
  /*if (DataSendTimer.IsRunning()) {
   *	Simulator::Schedule(m_dataAccuPeriod+Seconds(m_rand->GetValue()),&AquaSimCopeMac::DataSendTimerExpire,this);
  }*/

  //callback to higher level, should be implemented differently
  //Scheduler::instance().schedule(&callback_handler,&callback_event, COPEMAC_CALLBACK_DELAY);
  return true;
}


bool
AquaSimCopeMac::RecvProcess(Ptr<Packet> pkt)
{
	AquaSimHeader ash;
	CopeHeader ch;
	AquaSimPtTag ptag;
	pkt->RemoveHeader(ash);
	pkt->PeekHeader(ch);
  pkt->AddHeader(ash);
	pkt->PeekPacketTag(ptag);

	AquaSimAddress dst = ch.GetDA();
	//AquaSimAddress src = ch.GetSA();

	if( ash.GetErrorFlag() )
	{
		//NS_LOG_WARN("RecvProcess: node" << m_device->GetNode() <<
		//	  " gets a corrupted packet at " << Simulator::Now().GetSeconds());
		/*if(drop_)
		                drop_->recv(pkt,"Error/Collision");
		   else*/
		pkt=0;
		return false;
	}

	if ( dst == m_device->GetAddress() || dst == AquaSimAddress::GetBroadcast() ) {
		if( ptag.GetPacketType() == AquaSimPtTag::PT_OTMAN ) {
			switch( ch.GetPType() ) {
			case CopeHeader::COPE_ND:
				ProcessND(pkt);
				break;
			case CopeHeader::COPE_ND_REPLY:
				ProcessNDReply(pkt);
				break;
			case CopeHeader::MULTI_REV:
				ProcessMultiRev(pkt);
				break;
			case CopeHeader::MULTI_REV_ACK:
				ProcessMultiRevAck(pkt);
				break;
			case CopeHeader::MULTI_DATA_ACK:
				ProcessDataAck(pkt);
				break;
			default:
				;
			}
		}
	}
	else {
		//DATA Packet
		NS_LOG_INFO("RecvProcess: recv data pkt");
		RecordDataPkt(pkt);
		SendUp(pkt);         //record the data received.!!!!
		//Packet::free(pkt);
		PrintResult();
	}


pkt=0;
return true;
}

void
AquaSimCopeMac::PreSendPkt(Ptr<Packet> pkt, Time delay)
{
  if( delay.IsNegative() )
      delay = Seconds(0.0001);
  AquaSimHeader ash;
  pkt->RemoveHeader(ash);
  ash.SetDirection(AquaSimHeader::DOWN);
  pkt->AddHeader(ash);
  Simulator::Schedule(delay,&AquaSimCopeMac::SendPkt,this,pkt);

}

void
AquaSimCopeMac::SendPkt(Ptr<Packet> pkt)
{
  AquaSimHeader ash;
  CopeHeader ch;
  pkt->RemoveHeader(ash);

  ash.SetTxTime(GetTxTime(ash.GetSerializedSize()+ch.GetSerializedSize()));

  switch( m_device->GetTransmissionStatus() ) {
    case SLEEP:
	PowerOn();
    case NIDLE:
	//m_device->SetTransmissionStatus(SEND);
	ash.SetTimeStamp(Simulator::Now());
	ash.SetDirection(AquaSimHeader::DOWN);
	pkt->AddHeader(ash);
	SendDown(pkt);
	m_backoffCounter=0;	//clear
	break;
    case RECV:
	/*printf("node %d backoff at %f\n", index_, NOW);
	if( m_backoffPkt == NULL ) {
		m_backoffPkt = pkt;
		Simulator::Schedule(0.01+m_rand->GetValue()*COPEMAC_BACKOFF_TIME, &AquaSimCopeMac::BackoffHandler,this, m_backoffPkt);
	}
	else {*/
	  pkt=0;
	/*}*/
	break;
    default:
	//status is SEND
      NS_LOG_INFO("SendPkt: Node=" << m_device->GetNode() << " send data too fast");
      pkt=0;
  }

  return;
}


Ptr<Packet>
AquaSimCopeMac::MakeND()
{
  AquaSimHeader ash;
  CopeHeader ch;
  AquaSimPtTag ptag;

  ch.SetPType(CopeHeader::COPE_ND);
  ch.SetDA(AquaSimAddress::GetBroadcast());
  ch.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  //hdr_o->hdr.ndh.send_time_ = NOW;

  ash.SetSize(ch.size());
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetDirection(AquaSimHeader::DOWN);
  //ash->addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_OTMAN);

  //fill the neighbors that this node already knows the delay
    //empty packet will suffice
  Ptr<Packet> pkt = Create<Packet>();

  uint32_t size = sizeof(uint)+ sizeof(AquaSimAddress)*m_propDelays.size();
  uint8_t *data = new uint8_t[size];
  //uint data_size = sizeof(uint)+ sizeof(AquaSimAddress)*m_propDelays.size();
  //Ptr<Packet> pkt = Create<Packet>(data_size);
  //ash.SetSize(ash.GetSize() + size);

  //unsigned char* walk = (unsigned char*)pkt->accessdata();
  *(uint*)data = m_propDelays.size();
  data += sizeof(uint);

  for( std::map<AquaSimAddress, Time>::iterator pos=m_propDelays.begin();
   pos != m_propDelays.end(); pos++)
  {
    *((AquaSimAddress*)data) = pos->first;
    data += sizeof(AquaSimAddress);
  }

  Ptr<Packet> tempPacket = Create<Packet>(data,size);
  pkt->AddAtEnd(tempPacket);

  NS_LOG_DEBUG("MakeND: packet size=" << pkt->GetSize());

  pkt->AddHeader(ch);
  pkt->AddHeader(ash);
  pkt->AddPacketTag(ptag);
  return pkt;
}


void
AquaSimCopeMac::ProcessND(Ptr<Packet> pkt)
{
  AquaSimHeader ash;
  CopeHeader ch;
  pkt->RemoveHeader(ash);
  pkt->PeekHeader(ch);
  pkt->AddHeader(ash);

  //printf("node %d recve ND from node %d\n", index_, mh->macSA());

  uint32_t size = pkt->GetSize();
  uint8_t *data = new uint8_t[size];
  pkt->CopyData(data,size);
  uint node_num = *((uint*)data);

  //unsigned char* walk = (unsigned char*)pkt->accessdata();
  //uint node_num_ = *((uint*)walk);
  //walk += sizeof(uint);

  for( uint i=0; i<node_num; i++) {
      if( m_device->GetAddress() == *((AquaSimAddress*)data) ) {
	    return;
      }
      data += sizeof(AquaSimAddress);
  }

  m_PendingND[ch.GetSA()].nd_sendtime = ash.GetTimeStamp();
  m_PendingND[ch.GetSA()].nd_recvtime = Simulator::Now() - ash.GetTxTime();

  return;
}


Ptr<Packet>
AquaSimCopeMac::MakeNDReply()
{
  if( m_PendingND.size() == 0 ) {
      NS_LOG_WARN("MakeNDReply: no pending nd at node=" << m_device->GetNode());
      return NULL;
  }

  Ptr<Packet> pkt = Create<Packet>();
  AquaSimHeader ash;
  CopeHeader ch;
  AquaSimPtTag ptag;

  ch.SetPType(CopeHeader::COPE_ND_REPLY);
  //hdr_o->hdr.nd_reply_h.nd_send_time_ = m_ndDepartNeighborTime[NDSender];
  //hdr_o->hdr.nd_reply_h.delay_at_receiver = NOW - m_ndReceiveTime[NDSender];

  ash.SetSize(ch.size());
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetDirection(AquaSimHeader::DOWN);
  //ash->addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_OTMAN);

  //uint data_size = sizeof(uint) + (2*sizeof(Time)+sizeof(AquaSimAddress))*m_PendingND.size();
  //pkt->allocdata( data_size);
  //unsigned char* walk = (unsigned char*)pkt->accessdata();
  uint32_t size = (2*sizeof(Time)+sizeof(AquaSimAddress))*m_PendingND.size();
  uint8_t *data = new uint8_t[size];
  //ash.SetSize(ash.GetSize() + size);
  *(uint*)data = m_PendingND.size();
  data += sizeof(uint);
  for( std::map<AquaSimAddress, NDRecord>::iterator pos = m_PendingND.begin();
	  pos != m_PendingND.end(); pos++) {
	*(AquaSimAddress*)data = pos->first;
	data += sizeof(AquaSimAddress);
	*(Time*)data = pos->second.nd_sendtime;
	data += sizeof(Time);
	*(Time*)data = pos->second.nd_recvtime;
	data += sizeof(Time);
  }
  Ptr<Packet> tempPacket = Create<Packet>(data,size);
  pkt->AddAtEnd(tempPacket);

  ch.SetDA(ash.GetNextHop());
  ch.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );

  pkt->AddHeader(ch);
  pkt->AddHeader(ash);
  pkt->AddPacketTag(ptag);

  ProcessNDReply(pkt);

  return pkt;
}


void
AquaSimCopeMac::ProcessNDReply(Ptr<Packet> pkt)
{
  AquaSimHeader ash;
  CopeHeader ch;
  pkt->RemoveHeader(ash);
  pkt->PeekHeader(ch);
  pkt->AddHeader(ash);
  NS_LOG_FUNCTION(this << m_device->GetNode() << ch.GetSA() << Simulator::Now().GetSeconds());

  uint32_t size = pkt->GetSize();
  uint8_t *data = new uint8_t[size];
  pkt->CopyData(data,size);
  uint rec_num = *((uint*)data);
  /*unsigned char* walk = (unsigned char*)pkt->accessdata();
  uint rec_num = *(uint*)walk;
  walk += sizeof(uint);
   */

  NDRecord tmp;
  for( uint i=0; i<rec_num; i++) {
      if( (*(AquaSimAddress*)data) == m_device->GetAddress() ) {
	  data += sizeof(AquaSimAddress);
	  tmp.nd_sendtime = *(Time*)data;
	  data += sizeof(Time);
	  tmp.nd_recvtime = *(Time*)data;
	  data += sizeof(Time);
	  m_propDelays[ch.GetSA()] = ((Simulator::Now()-tmp.nd_sendtime) -
		  (ash.GetTimeStamp()-tmp.nd_recvtime-ash.GetTxTime()))/2.0;
	  break;
      }
      data += sizeof(AquaSimAddress);
      data += sizeof(Time)*2;
  }
}



void
AquaSimCopeMac::StartHandShake()
{
  /*if( !m_PktWH.IsEmpty() ) {*/
    Ptr<Packet> pkt = MakeMultiRev();
    if( pkt == NULL )
	return;
    AquaSimHeader ash;
    pkt->PeekHeader(ash);
    CtrlPktInsert(pkt, m_RevQ.GetValidStartTime(ash.GetTxTime()) );
    //PreSendPkt(pkt, Random::uniform()*COPE_REV_DELAY);
  /*}*/
}

//int AquaSimCopeMac::round2Slot(Time time)
//{
//	return int( (time-NOW)/TimeSlotLen_ );
//}
//
//Time AquaSimCopeMac::Slot2Time(int SlotNum, Time BaseTime)
//{
//	return BaseTime+SlotNum*TimeSlotLen_;
//}
//
//Time AquaSimCopeMac::round2RecverSlotBegin(Time time, AquaSimAddress recver)
//{
//	//return int( (time-NOW)
//	int SlotNum = int((time+m_propDelays[recver])/TimeSlotLen_);
//	if( time+ m_propDelays[recver] - SlotNum*TimeSlotLen_  > 0.00001 )
//		SlotNum++;
//	return SlotNum*TimeSlotLen_ - m_propDelays[recver];
//}

Ptr<Packet>
AquaSimCopeMac::MakeMultiRev()
{
  //m_RevQ.printRevQueue();
  Ptr<Packet> pkt = Create<Packet>();
  AquaSimHeader ash;
  CopeHeader ch;
  AquaSimPtTag ptag;

  ch.SetPType(CopeHeader::MULTI_REV);
  ch.SetDA(AquaSimAddress::GetBroadcast());
  ch.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );

  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetDirection(AquaSimHeader::DOWN);
  //ash->addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_OTMAN);

  uint rev_num = 0;
  std::map<AquaSimAddress, PktList>::iterator pos;
  for( pos = m_PktWH.Queues.begin(); pos != m_PktWH.Queues.end(); pos++ )
  {
    if( pos->second.head != NULL ) {
	rev_num ++;
    }
  }

  if( rev_num == 0 ) {
      pkt=0;
      return NULL;
  }
  /*
   * The format of entry in multi-rev is
   *		AquaSimAddress requestor;
   *		Time	PktLen;
   *		int	MajorRevID;
   *		Time	MajorStartTime;
   *		int	BackupRevID;
   *		Time	BackupStartTime;
   */
  //fill the neighbors to which this node already knows the delay
  uint ithneighbor = m_neighborId%rev_num;
  if( !m_isParallel ) {
      rev_num = 1;
  }

  uint32_t size = sizeof(uint)+ sizeof(Time)+ (sizeof(AquaSimAddress)+sizeof(Time)*3+sizeof(int)*2)*rev_num;
  uint8_t *data = new uint8_t[size];

  //pkt->allocdata( sizeof(uint)+ sizeof(Time)+ (sizeof(AquaSimAddress)+sizeof(Time)*3+sizeof(int)*2)*rev_num);
  //unsigned char* walk = (unsigned char*)pkt->accessdata();
  *(uint*)data = rev_num;
  data += sizeof(uint);
  *(Time*)data = Simulator::Now();   //record the time when filling the packet
  data += sizeof(Time);


  PktElem* tmp;
  uint i=0;
  for( pos = m_PktWH.Queues.begin();pos != m_PktWH.Queues.end(); pos++ )
  {
      if( (!m_isParallel) && (i<ithneighbor) ) {
	  i++;
	  continue;
      }

      if( pos->second.head != NULL ) {

	  *((AquaSimAddress*)data) = pos->first;
	  data += sizeof(AquaSimAddress);
	  tmp = pos->second.head;

	  Ptr<Packet> TmpPkt = pos->second.head->pkt_->Copy();
	  AquaSimHeader ashTemp;
	  TmpPkt->PeekHeader(ashTemp);

	  Time MajorInterval, BackupInterval;  //both refer to the start time of the interval
	  Time PktLen_ = ashTemp.GetTxTime() + m_guardTime; //guardtime is used to avoid collision

	  //Time LowerInterval = m_majorIntervalLB;
	  //Time UpperInterval = MajorIntervalUB;
	  //get major interval
	  //Time SinceTime= round2RecverSlotBegin(NOW, pos->first);
	  MajorInterval = m_RevQ.GetValidStartTime(PktLen_, Simulator::Now()+m_majorIntervalLB);
	  /*do{
		  counter++;
		  if( counter == 100 ) {
			  UpperInterval += IntervalStep;
			  counter = 0;
		  }
		  MajorInterval =	Random::uniform(LowerInterval,
										  UpperInterval);
	  }while(!m_RevQ.checkAvailability(NOW+MajorInterval,
		  NOW+MajorInterval+PktLen_, PRE_REV) );*/


	  //get backup send interval
	  BackupInterval = m_RevQ.GetValidStartTime(PktLen_, Simulator::Now()+MajorInterval + PktLen_);
	  /*Time tmpInterval = MajorBackupInterval;
	  do{
		  counter++;
		  if( counter == 100 ) {
			  tmpInterval += IntervalStep;
			  counter = 0;
		  }
		  BackupInterval = MajorInterval+PktLen_ +
						   Random::uniform(tmpInterval);
	  }while( !m_RevQ.checkAvailability(NOW+BackupInterval,
						  NOW+BackupInterval+PktLen_, PRE_REV) );*/

	  RevID++;
	  m_RevQ.Push(RevID, Simulator::Now()+MajorInterval, Simulator::Now()+MajorInterval+PktLen_,
				  AquaSimAddress::ConvertFrom(m_device->GetAddress()) , PRE_REV, TmpPkt->Copy());

	  *(Time*)data = PktLen_;   //already includes m_guardTime
	  data += sizeof(Time);
	  *(int*)data = RevID;
	  data += sizeof(int);
	  *(Time*)data = MajorInterval;
	  data += sizeof(Time);

	  RevID++;
	  m_RevQ.Push(RevID, Simulator::Now()+BackupInterval,
		      Simulator::Now()+BackupInterval+PktLen_,
		      AquaSimAddress::ConvertFrom(m_device->GetAddress()) , PRE_REV, TmpPkt);
	  *(int*)data = RevID;
	  data += sizeof(int);
	  *(Time*)data = BackupInterval;   //this is the backup time slot. One of the 10 slots after major slot
	  data += sizeof(Time);


	  //remove the packet from PacketWH_ and insert it into
	  //AckWaitingList
	  pos->second.head = tmp->next_;
	  InsertAckWaitingList(tmp->pkt_->Copy(), m_ackTimeOut);
	  AquaSimHeader ashTmp;
	  tmp->pkt_->PeekHeader(ashTmp);
	  m_PktWH.DeletePkt(pos->first, ashTmp.GetUId());

	  if( !m_isParallel ) {
		  break;
	  }
      }
  }

  //node id use 8 bits, first time slot use 10bits, backup time slot use 4 bits

  ash.SetSize((4+(8+8+8+8))*rev_num/8);

  m_neighborId = (m_neighborId+1)%m_propDelays.size();
  //m_RevQ.printRevQueue();

  Ptr<Packet> tempPacket = Create<Packet>(data,size);
  pkt->AddAtEnd(tempPacket);
  pkt->AddHeader(ch);
  pkt->AddHeader(ash);
  pkt->AddPacketTag(ptag);

  return pkt;
}


void
AquaSimCopeMac::ProcessMultiRev(Ptr<Packet> pkt)
{
  //m_RevQ.printRevQueue();
  //TRANSMIT THE TIME IN AVOIDING ITEM TO MY OWN TIME VIEW

  AquaSimHeader ash;
  CopeHeader ch;
  pkt->RemoveHeader(ash);
  pkt->PeekHeader(ch);
  pkt->AddHeader(ash);

  uint32_t size = pkt->GetSize();
  uint8_t *data = new uint8_t[size];
  pkt->CopyData(data,size);
  //unsigned char* walk = (unsigned char*)pkt->accessdata();
  uint rev_num = *(uint*)data;
  data += sizeof(uint);

  Time delta_time = ash.GetTimeStamp() - (*(Time*)data);
  //reservation time = majorinterval -delta_time +NOW
  data += sizeof(Time);

  /*store the rev requests in Pending Revs and
   *process it when RevAckAccumTimer expires
   */

  Time PktLen_;
  for(uint i=0; i<rev_num; i++) {
    if( *((AquaSimAddress*)data) == m_device->GetAddress() ) {
	data += sizeof(AquaSimAddress);

	if( RevAckAccumTimer.IsExpired() ) {
	    RevAckAccumTimer.SetFunction(&AquaSimCopeMac::RevAckAccumTimerExpire,this);
      RevAckAccumTimer.Schedule(m_revAckAccumTime);
	}

	RevReq* tmp = new RevReq();
	tmp->requestor = ch.GetSA();
	//tmp->Sincetime = map2OwnTime(cmh->ts_, mh->macSA());

	PktLen_ = *((Time*)data);
	data += sizeof(Time);
	tmp->acceptedRevID = *((int*)data);
	data += sizeof(int);

	//covert the time based on this node's timeline
	tmp->StartTime = *((Time*)data)-delta_time + Simulator::Now();
	data += sizeof(Time);
	tmp->EndTime = tmp->StartTime + PktLen_;

	//check major slot
	if( m_RevQ.CheckAvailability(tmp->StartTime,tmp->EndTime, RECVING) ) {
	    tmp->rejectedRevID = *((int*)data);
	    data += sizeof(int)+sizeof(Time);

	    m_RevQ.Push(RevID, tmp->StartTime, tmp->EndTime,
		    tmp->requestor, RECVING, pkt);
	}
	else{
	    tmp->rejectedRevID = tmp->acceptedRevID;
	    tmp->acceptedRevID = *((int*)data);
	    data += sizeof(int);

	    //covert the time based on this node's timeline
	    tmp->StartTime = *((Time*)data)-delta_time + Simulator::Now();
	    data += sizeof(Time);
	    tmp->EndTime = tmp->StartTime + PktLen_;

	    if( m_RevQ.CheckAvailability(tmp->StartTime, tmp->EndTime, RECVING) ) {

		m_RevQ.Push(RevID, tmp->StartTime, tmp->EndTime,
			tmp->requestor, RECVING, pkt);
	    }
	    else {
		//give a wrong rev time interval, so the requestor will know both are wrong
		tmp->StartTime = Seconds(-1);
		tmp->EndTime = Seconds(-1);
	    }

	}
	m_pendingRevs.push_back(tmp);

    }
    else{
	//this entry has nothing to do with this node. skip it
	data += sizeof(AquaSimAddress);
	data += 3*sizeof(Time);
	data += 2*sizeof(int);
    }
  }
  //m_RevQ.PrintRevQueue();
}


/*
 * MultiRevAck format
 *	AquaSimAddress	requestor;
 *	Time	StartTime;  //related to current time
 *	Time	EndTime;
 *	int	acceptedRevID;
 *	int	rejectedRevID;
 */
Ptr<Packet>
AquaSimCopeMac::MakeMultiRevAck()
{
  //overhear neighbors rev
  Ptr<Packet> pkt = Create<Packet>();
  AquaSimHeader ash;
  CopeHeader ch;
  AquaSimPtTag ptag;

  ch.SetPType(CopeHeader::MULTI_REV_ACK);
  ch.SetDA(AquaSimAddress::GetBroadcast());
  ch.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );

  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetDirection(AquaSimHeader::DOWN);
  //ash->addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_OTMAN);


  uint32_t size = sizeof(uint)+ sizeof(Time)+ (sizeof(AquaSimAddress)+2*sizeof(int)+2*sizeof(Time))*m_pendingRevs.size();
  uint8_t *data = new uint8_t[size];

  //pkt->allocdata( sizeof(uint)+ sizeof(Time)+ (sizeof(AquaSimAddress)+2*sizeof(int)+2*sizeof(Time))*m_pendingRevs.size() );
  //unsigned char* walk = (unsigned char*)pkt->accessdata();
  *(uint*)data = m_pendingRevs.size();
  data += sizeof(uint);
  *(Time*)data = Simulator::Now();
  data += sizeof(Time);

  for( std::vector<RevReq*>::iterator pos=m_pendingRevs.begin();
	      pos != m_pendingRevs.end(); pos++)
  {
      *(AquaSimAddress*)data = (*pos)->requestor;  //ack to whom
      data += sizeof(AquaSimAddress);
      *(Time*)data = (*pos)->StartTime - Simulator::Now();
      data += sizeof(Time);
      *(Time*)data = (*pos)->EndTime - Simulator::Now();
      data += sizeof(Time);
      *(int*)data = (*pos)->acceptedRevID;
      data += sizeof(int);
      *(int*)data = (*pos)->rejectedRevID;
      data += sizeof(int);
  }

  //schedule the rev req.
  //node id use 10 bits, first time slot use 10bits, backup time slot use 4 bits
  //ch->size() = (m_pendingRevs.size()*(10+10+6+4))/8;
  ash.SetSize(m_pendingRevs.size()*(8+10+6+4)/8);

  Ptr<Packet> tempPacket = Create<Packet>(data,size);
  pkt->AddAtEnd(tempPacket);
  pkt->AddHeader(ch);
  pkt->AddHeader(ash);
  pkt->AddPacketTag(ptag);
  return pkt;
}

void
AquaSimCopeMac::ProcessMultiRevAck(Ptr<Packet> pkt)
{
  //m_RevQ.printRevQueue();
  //overhear neighbors revack
  //change the pre_rev to SENDING, and delete the other slot rev
  //start the the timer in the rev_elem
  AquaSimHeader ash;
  CopeHeader ch;
  pkt->RemoveHeader(ash);
  pkt->PeekHeader(ch);
  pkt->AddHeader(ash);

  uint32_t size = pkt->GetSize();
  uint8_t *data = new uint8_t[size];
  pkt->CopyData(data,size);
  //unsigned char* walk = (unsigned char*)pkt->accessdata();
  uint rev_num = *(uint*)data;
  data += sizeof(uint);
  Time delta_time = ash.GetTimeStamp() - (*(Time*)data);
  data += sizeof(Time);

  RevReq* tmp = new RevReq();
  for(uint i=0; i<rev_num; i++) {

      tmp->requestor = *(AquaSimAddress*)data;
      data += sizeof(AquaSimAddress);
      tmp->StartTime = *(Time*)data;
      data += sizeof(Time);
      tmp->EndTime = *(Time*)data;
      data += sizeof(Time);
      tmp->acceptedRevID = *(int*)data;
      data += sizeof(int);
      tmp->rejectedRevID = *(int*)data;
      data += sizeof(int);

      if( tmp->StartTime > 0 ) {
	  if( tmp->requestor == m_device->GetAddress() ) {
	      //start timer in updateStatus
	      m_RevQ.UpdateStatus(tmp->acceptedRevID, SENDING);
	      m_RevQ.DeleteRev(tmp->rejectedRevID);
	  }
	  else {
	      m_RevQ.Push(tmp->acceptedRevID, Simulator::Now()+tmp->StartTime -delta_time - m_propDelays[ch.GetSA()],
				      Simulator::Now()+tmp->EndTime -delta_time - m_propDelays[ch.GetSA()],
				      ch.GetSA(), AVOIDING, pkt);
	  }
      }
      else if( tmp->requestor == m_device->GetAddress() ) {
	  m_RevQ.DeleteRev(tmp->acceptedRevID);
	  m_RevQ.DeleteRev(tmp->rejectedRevID);
      }

  }
  delete tmp;
  //m_RevQ.printRevQueue();
}

void
AquaSimCopeMac::RecordDataPkt(Ptr<Packet> pkt)
{
  AquaSimHeader ash;
  CopeHeader ch;
  pkt->RemoveHeader(ash);
  pkt->PeekHeader(ch);
  pkt->AddHeader(ash);

  DataAck* tmp = new DataAck;
  tmp->Sender = ch.GetSA();
  tmp->SeqNum = ash.GetUId();
  m_pendingDataAcks.push_back(tmp);

  m_sucDataNum[ch.GetSA()]++;
  // startAckTimer
  if( DataAckAccumTimer.IsExpired() ) {
      DataAckAccumTimer.SetFunction(&AquaSimCopeMac::DataAckAccumTimerExpire,this);
      DataAckAccumTimer.Schedule(m_dataAckAccumTime);
  }
}


Ptr<Packet>
AquaSimCopeMac::MakeDataAck()
{
  Ptr<Packet> pkt = Create<Packet>();
  AquaSimHeader ash;
  CopeHeader ch;
  AquaSimPtTag ptag;

  ch.SetPType(CopeHeader::MULTI_DATA_ACK);
  ch.SetDA(AquaSimAddress::GetBroadcast());
  ch.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );

  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetDirection(AquaSimHeader::DOWN);
  //ash->addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_OTMAN);

  uint DataAckNum = m_pendingDataAcks.size();

  //ch->size() = DataAckNum*((10+10)/8);
  ash.SetSize(DataAckNum*((10+10)/8));


  uint32_t size = sizeof(uint)+DataAckNum*(sizeof(AquaSimAddress)+sizeof(int));
  uint8_t *data = new uint8_t[size];

  //pkt->allocdata(sizeof(uint)+DataAckNum*(sizeof(AquaSimAddress)+sizeof(int)));
  //unsigned char* walk = (unsigned char*)pkt->accessdata();
  *(uint*)data = DataAckNum;
  data += sizeof(uint);


  for(std::vector<DataAck*>::iterator pos = m_pendingDataAcks.begin();
	  pos != m_pendingDataAcks.end(); pos++ )
  {
      *(AquaSimAddress*)data = (*pos)->Sender;
      data += sizeof(AquaSimAddress);
      *(int*)data = (*pos)->SeqNum;
      data += sizeof(int);
  }

  Ptr<Packet> tempPacket = Create<Packet>(data,size);
  pkt->AddAtEnd(tempPacket);
  pkt->AddHeader(ch);
  pkt->AddHeader(ash);
  pkt->AddPacketTag(ptag);
  return pkt;
}


void
AquaSimCopeMac::ProcessDataAck(Ptr<Packet> pkt)
{
  uint32_t size = pkt->GetSize();
  uint8_t *data = new uint8_t[size];
  pkt->CopyData(data,size);
  //unsigned char* walk = (unsigned char*)pkt->accessdata();
  uint AckNum = *(uint*)data;
  data += sizeof(uint);

  /*store the rev requests in Pending Revs and
   *process it when RevAckAccumTimer expires
   */
  int pkt_id;
  for(uint i=0; i<AckNum; i++) {
      if( *((AquaSimAddress*)data) == m_device->GetAddress() ) {
	  data += sizeof(AquaSimAddress);
	  /*AquaSimAddress recver = hdr_mac::access(pkt)->macSA();
	  if( m_PktWH.DeletePkt(recver, *(int*)data) ) {
	      if( m_sucDataNum.count(recver) == 0 )
		  m_sucDataNum[recver] = 1;
	      else
		  m_sucDataNum[recver]++;
	  }

	  data += sizeof(int);	*/
	  pkt_id = (*(int*)data);
	  if( m_AckWaitingList.count(pkt_id) != 0 ) {
	      if( m_AckWaitingList[pkt_id].m_ackWaitTimer.IsRunning() ) {
		  m_AckWaitingList[pkt_id].m_ackWaitTimer.Cancel();
	      }
	      if( m_AckWaitingList[pkt_id].m_pkt!= NULL ) {
		  m_AckWaitingList[pkt_id].m_pkt=0;
	      }
	  }
	  data += sizeof(int);
      }
      else{
	  data += sizeof(AquaSimAddress);
	  data += sizeof(int);
      }
  }
}


Time
AquaSimCopeMac::Map2OwnTime(Time SenderTime, AquaSimAddress Sender)
{
  return SenderTime+m_propDelays[Sender];
}

void
AquaSimCopeMac::PrintResult()
{
  int totalPkt = 0;
  std::map<AquaSimAddress, int>::iterator pos = m_sucDataNum.begin();
  for(; pos != m_sucDataNum.end(); pos++) {
      totalPkt += pos->second;
  }
  NS_LOG_INFO("PrintResults: Node=" << m_device->GetNode() <<
	      " receive " << totalPkt << " packets.");
}

void
AquaSimCopeMac::ClearAckWaitingList()
{
  std::set<int> DelList;
  for(std::map<int, AckWaitTimer>::iterator pos = m_AckWaitingList.begin();
		  pos!=m_AckWaitingList.end(); pos++) {
      if( pos->second.m_pkt == NULL )
	  DelList.insert(pos->first);
  }

  for(std::set<int>::iterator pos = DelList.begin();
		pos!=DelList.end(); pos++) {
	m_AckWaitingList.erase(*pos);
  }
}

void
AquaSimCopeMac::InsertAckWaitingList(Ptr<Packet> p, Time delay)
{
  ClearAckWaitingList();
  AquaSimHeader ash;
  p->PeekHeader(ash);
  int uid_ = ash.GetUId();
  m_AckWaitingList[uid_].m_mac = this;
  m_AckWaitingList[uid_].m_pkt = p;
  m_AckWaitingList[uid_].m_ackWaitTimer.SetFunction(&AquaSimCopeMac::AckWaitTimerExpire,this);
  m_AckWaitingList[uid_].m_ackWaitTimer.SetArguments(m_AckWaitingList[uid_].m_pkt);
  m_AckWaitingList[uid_].m_ackWaitTimer.Schedule(delay);
}


void
AquaSimCopeMac::AckWaitTimerExpire(Ptr<Packet> pkt)
{
  //int pkt_id = HDR_CMN(pkt_)->uid();
  m_PktWH.Insert2PktQs(pkt);
  pkt = 0;
}


void
AquaSimCopeMac::CtrlPktInsert(Ptr<Packet> ctrl_p, Time delay)
{
  Simulator::Schedule(delay,&AquaSimCopeMac::SendPkt,this,ctrl_p);	//current work around...

  /*Timer tmp = Timer (Timer::CANCEL_ON_DESTROY);
  tmp.SetFunction(&AquaSimCopeMac::SendPkt);
  tmp.SetArguments(ctrl_p);
  tmp.Schedule(delay);
  m_ctrlQ.push(tmp);*/
}

void
AquaSimCopeMac::ClearExpiredElem()
{
  Timer tmp;
  while( (m_ctrlQ.size() >0) && ((m_ctrlQ.front()).IsExpired()) ) {
      tmp = m_ctrlQ.front();
      m_ctrlQ.pop();
      tmp.Remove();
  }
}

void
AquaSimCopeMac::PrintDelayTable()
{
  //FILE* stream = fopen("distance", "a");

  std::map<AquaSimAddress, Time>::iterator pos=m_propDelays.begin();
  for(;pos != m_propDelays.end(); pos++) {
      NS_LOG_INFO("PrintDelayTable: " << pos->second*1500);
  }

  //fclose(stream);
}

void
AquaSimCopeMac::NDProcessInitor()
{
  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
  CtrlPktInsert(MakeND(), FemtoSeconds(m_rand->GetValue(0.0,m_NDWin)));
  if( m_NDProcessMaxTimes > 0 ) {
      Simulator::Schedule(FemtoSeconds(m_NDWin+0.9), &AquaSimCopeMac::NDProcessInitor, this);
  }
  m_NDProcessMaxTimes --;
}

void
AquaSimCopeMac::DataSendTimerExpire()
{
  //is below calling self?
  //resched(mac_->m_dataAccuPeriod+Random::uniform(2.0));
  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
  Simulator::Schedule(m_dataAccuPeriod+Seconds(m_rand->GetValue(0.0,2.0)),&AquaSimCopeMac::DataSendTimerExpire,this);
  StartHandShake();
}

void
AquaSimCopeMac::RevAckAccumTimerExpire()
{
  if( m_pendingRevs.size() != 0 ) {
      Ptr<Packet> tmp = MakeMultiRevAck();
      //PreSendPkt();
      AquaSimHeader ash;
      tmp->PeekHeader(ash);
      CtrlPktInsert(tmp, m_RevQ.GetValidStartTime(ash.GetTxTime()));
      m_pendingRevs.clear();
  }
}

void
AquaSimCopeMac::DataAckAccumTimerExpire()
{
  Ptr<Packet> tmp = MakeDataAck();
  //PreSendPkt(MakeDataAck());
  AquaSimHeader ash;
  tmp->PeekHeader(ash);
  CtrlPktInsert(tmp,m_RevQ.GetValidStartTime(ash.GetTxTime()));
  m_pendingDataAcks.clear();
}

void AquaSimCopeMac::DoDispose()
{
  m_backoffPkt=0;
  m_rand=0;
  for (std::map<int,AckWaitTimer>::iterator iter = m_AckWaitingList.begin(); iter != m_AckWaitingList.end(); ++iter) {
    (iter->second).m_pkt=0;
    (iter->second).m_mac=0;
  }
  m_AckWaitingList.clear();
  for (std::vector<RevReq*>::iterator itRev = m_pendingRevs.begin() ; itRev != m_pendingRevs.end(); ++itRev) {
    delete *itRev;
    *itRev=0;
  }
  for (std::vector<DataAck*>::iterator itData = m_pendingDataAcks.begin() ; itData != m_pendingDataAcks.end(); ++itData) {
    delete *itData;
    *itData=0;
  }
  AquaSimMac::DoDispose();
}

} // namespace ns3
