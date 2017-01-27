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

#include "aqua-sim-mac-uwan.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-mac.h"
#include "aqua-sim-pt-tag.h"
//#include "vbf/vectorbasedforward.h"

#include "ns3/double.h"
#include "ns3/log.h"

#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimUwan");
NS_OBJECT_ENSURE_REGISTERED(AquaSimUwan);

AquaSimUwan_WakeTimer::~AquaSimUwan_WakeTimer()
{
  m_mac=0;
  //most likely memory leak
}

void
AquaSimUwan_WakeTimer::expire()
{
  m_mac->Wakeup(m_ScheT->nodeId_);
}

void
AquaSimUwan_SleepTimer::expire()
{
  m_mac->Sleep();
}


void
AquaSimUwan_PktSendTimer::expire()
{
  m_mac->TxPktProcess(this);
}

void
AquaSimUwan_StartTimer::expire()
{
  m_mac->Start();
}

class AquaSimUwan;

Time AquaSimUwan::m_initialCyclePeriod = Seconds(10.0);
Time AquaSimUwan::m_maxPropTime = Seconds(3000.0/1500.0); //TODO Should be AquaSimMac::Device()->GetPhy()->GetTransRange()
Time AquaSimUwan::m_maxTxTime = Seconds(0.0);
Time AquaSimUwan::m_listenPeriod = Seconds(0.0);		//the length of listening to the channel after transmission.
Time AquaSimUwan::m_helloTxLen = Seconds(0.0);
Time AquaSimUwan::m_wakePeriod = Seconds(0.0);

AquaSimUwan::AquaSimUwan():
		/*pkt_send_timer(this),*/
		m_sleepTimer(this), m_startTimer(this),
		m_wakeSchQueue(this)
{
	m_cycleCounter = 1;
	m_numPktSend = 0;
  m_sleepTimer.SetFunction(&AquaSimUwan_SleepTimer::expire,&m_sleepTimer);
  m_startTimer.SetFunction(&AquaSimUwan_StartTimer::expire,&m_startTimer);
	m_startTimer.Schedule(Seconds(0.001));
	m_nextHopNum = 0;
}

AquaSimUwan::~AquaSimUwan()
{
}

TypeId
AquaSimUwan::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimUwan")
    .SetParent<AquaSimMac>()
    .AddConstructor<AquaSimUwan>()
    .AddAttribute ("AvgCyclePeriod", "Time of average cycle period.",
      TimeValue(MilliSeconds(50)), // = 1 / data_rate, where data_rate = 0.02
      MakeTimeAccessor(&AquaSimUwan::m_avgCyclePeriod),
      MakeTimeChecker ())
    .AddAttribute ("StdCyclePeriod", "Time of std cycle period.",
      TimeValue(MilliSeconds(1)),
      MakeTimeAccessor(&AquaSimUwan::m_stdCyclePeriod),
      MakeTimeChecker ())
    ;
  return tid;
}

void
AquaSimUwan::SendInfo()
{
  //TODO get file print to function properly
  //FILE* result_f = fopen("send.data", "a");
	//fprintf(result_f, "MAC(%d) : num_send = %d\n", index_, m_numPktSend);
  NS_LOG_FUNCTION(this << m_device->GetNode() << m_numPktSend);
	//fclose(result_f);
}


void
AquaSimUwan::SendFrame(Ptr<Packet> p, bool IsMacPkt, Time delay)
{
  AquaSimHeader ash;
  p->RemoveHeader(ash);
  ash.SetDirection(AquaSimHeader::DOWN);
  ash.SetTxTime( Seconds(ash.GetSize() * m_encodingEfficiency/m_bitRate));
  p->AddHeader(ash);

	AquaSimUwan_PktSendTimer *tmp = new AquaSimUwan_PktSendTimer(this);
	tmp->SetTxTime(ash.GetTxTime());
	tmp->m_p = p;
  tmp->SetFunction(&AquaSimUwan_PktSendTimer::expire,tmp);
  tmp->Schedule(delay);
	m_pktSendTimerSet.insert(tmp);

	//pkt_send_timer.SetTxTime(HDR_CMN(p)->txtime());
	//pkt_send_timer.m_p = p;
	//pkt_send_timer.resched(delay);  //set transmission status when this timer expires

	//Scheduler::instance().schedule(downtarget_, p, delay+0.0001);  //phy->Recv(p)
	/*if( !IsMacPkt ) {
		Scheduler::instance().schedule(&callback_handler,
						&callback_event, delay_time+UWAN_CALLBACK_DELAY);
	}*/
	//callback_handler ?????????
}


void
AquaSimUwan::TxPktProcess(AquaSimUwan_PktSendTimer* pkt_send_timer)
{
	Ptr<Packet> p = pkt_send_timer->m_p;
	pkt_send_timer->m_p = NULL;
	if( m_device->GetTransmissionStatus() == SEND
			|| m_device->GetTransmissionStatus() == RECV ) {
		//if the status is not IDLE (SEND or RECV), the scheduled event cannot be
		//execute on time. Thus, drop the packet.
    NS_LOG_WARN("Schedule failure. Dropping packet:" << p);
    /*if(drop_)
			drop_->recv(p,"Schedule Failure");
		else*/
      p=0;
		m_pktSendTimerSet.erase(pkt_send_timer);
		return;
	}

	//m_device->SetTransmissionStatus(SEND);
	AquaSimHeader ashLocal;
	p->RemoveHeader(ashLocal);
	ashLocal.SetTxTime(pkt_send_timer->GetTxTime());
	p->AddHeader(ashLocal);

	SendDown(p);
	m_pktSendTimerSet.erase(pkt_send_timer);
}


Ptr<Packet>
AquaSimUwan::MakeSYNCPkt(Time CyclePeriod, AquaSimAddress Recver)
{
	Ptr<Packet> p = Create<Packet>();
  UwanSyncHeader hdr_s;
  AquaSimHeader ash;
  MacHeader mach;
  AquaSimPtTag ptag;

  //Assuming this shouldnt be negative...
  if (CyclePeriod.IsNegative()) {
    hdr_s.SetCyclePeriod(-1*CyclePeriod.ToDouble(Time::S));
  }
  else {
    hdr_s.SetCyclePeriod(CyclePeriod.ToDouble(Time::S));
  }

	ash.SetSize(hdr_s.GetSize());
  ash.SetNextHop(Recver);  //the sent packet??
  ash.SetDirection(AquaSimHeader::DOWN);
  //ash.addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_UWAN_SYNC);

	mach.SetDA(Recver);
	mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );

  p->AddHeader(hdr_s);
  p->AddHeader(mach);
  p->AddHeader(ash);
  p->AddPacketTag(ptag);

	return p;
}


Ptr<Packet>
AquaSimUwan::FillMissingList(Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this);

  AquaSimHeader ash;
  p->RemoveHeader(ash);
	std::set<AquaSimAddress> ML_;
	std::set_difference(m_neighbors.begin(), m_neighbors.end(),
		m_CL.begin(), m_CL.end(),
		std::insert_iterator<std::set<AquaSimAddress> >(ML_, ML_.begin()));

  Buffer buff;
  Buffer::Iterator i = buff.Begin();
  buff.AddAtEnd(sizeof(size_t));
  buff.AddAtEnd(sizeof(uint16_t)*ML_.size());

  i = buff.Begin();
  i.WriteU8(ML_.size());

  for( std::set<AquaSimAddress>::iterator pos=ML_.begin();
       pos != ML_.end(); pos++)
  {
    i.WriteU16((*pos).GetAsInt());
    //p->AddAtEnd(Create<Packet>((uint8_t*)(*pos).GetAsInt(),sizeof(AquaSimAddress)));
  }
  p->AddAtEnd(Create<Packet>(buff.PeekData(),buff.GetSize()));
  p->AddHeader(ash);
  /*uint32_t size = sizeof(uint) + ML_.size()*sizeof(AquaSimAddress);
  uint8_t *data = new uint8_t[size];
  ash.SetSize(ash.GetSize() + 8*size );

  *(uint*)data = ML_.size();
  data += sizeof(uint);

  for( std::set<AquaSimAddress>::iterator pos=ML_.begin();
       pos != ML_.end(); pos++)
  {
      *(AquaSimAddress*)data = *pos;
      data += sizeof(AquaSimAddress);
  }
  Ptr<Packet> tempPacket = Create<Packet>(data,size);
  p->AddAtEnd(tempPacket);
  */

  return p;
}

Ptr<Packet>
AquaSimUwan::FillSYNCHdr(Ptr<Packet> p, Time CyclePeriod)
{
  UwanSyncHeader hdr_s;
  AquaSimHeader ash;
  MacHeader mach;
  p->RemoveHeader(ash);
  p->RemoveHeader(mach);
  p->RemoveHeader(hdr_s);

  hdr_s.SetCyclePeriod(CyclePeriod.ToDouble(Time::S));

  ash.SetSize(ash.GetSize() + hdr_s.GetSize());
  p->AddHeader(hdr_s);
  p->AddHeader(mach);
  p->AddHeader(ash);

  return p;
}


void
AquaSimUwan::Wakeup(AquaSimAddress node_id)
{
	Time now = Simulator::Now();

	if( m_device->GetTransmissionStatus() == SLEEP )
		PowerOn();

	m_wakeSchQueue.ClearExpired(now);

	if( node_id == m_device->GetAddress() ) {
		//generate the time when this node will send out next packet
		++m_cycleCounter;
		m_cycleCounter = m_cycleCounter% 10;

		switch( m_cycleCounter ) {
			case 0:
				//This node would keep awake for m_initialCyclePeriod.
				//And this is set in Start().
				SYNCSchedule();  //node keeps awake in this period
				return;
/*
			case 9:
				m_nextCyclePeriod = m_initialCyclePeriod + now;
				break;
*/
			default:
				//not this node's SYNC period, just send out the data packet
				m_nextCyclePeriod = GenNxCyclePeriod();
				break;
		}

    Time maxPropTime = Seconds(2*m_maxPropTime.ToDouble(Time::S));
		if( ! m_wakeSchQueue.CheckGuardTime(m_nextCyclePeriod, maxPropTime, m_maxTxTime) ) {
			m_nextCyclePeriod =
				m_wakeSchQueue.GetAvailableSendTime(now+m_wakePeriod,
										m_nextCyclePeriod, maxPropTime, m_maxTxTime);
		}

		m_wakeSchQueue.Push(m_nextCyclePeriod, AquaSimAddress::ConvertFrom(m_device->GetAddress()) , m_nextCyclePeriod-now);
		//m_wakeSchQueue.Print(2*m_maxPropTime, m_maxTxTime, true, AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
		if( m_packetQueue.empty() )
			SendFrame(MakeSYNCPkt(m_nextCyclePeriod-now),true);
		else
			SendoutPkt(m_nextCyclePeriod);
	}
	else {
		m_CL.erase(node_id);
	}

	//set the sleep timer
	if( m_sleepTimer.IsRunning() )
		m_sleepTimer.Cancel();

	SetSleepTimer(m_wakePeriod);

}


void
AquaSimUwan::Sleep()
{
	//if( setWakeupTimer() ) {   //This node set the timer to wake up itself
	//	Poweroff();
	//	((UnderwaterSensorNode*)Node_)->SetTransmissionStatus(SLEEP);
	//}
	PowerOff();
}



void
AquaSimUwan::SetSleepTimer(Time Interval)
{
  m_sleepTimer.Schedule(Interval);
}



Time
AquaSimUwan::GenNxCyclePeriod()
{
	//return m_nextCyclePeriod + Random::normal(m_avgCyclePeriod, m_stdCyclePeriod);
  double avgCyclePeriod = m_avgCyclePeriod.ToDouble(Time::S);
  double stdCyclePeriod = m_stdCyclePeriod.ToDouble(Time::S);

  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
	return m_nextCyclePeriod + Seconds(m_rand->GetValue(avgCyclePeriod-stdCyclePeriod,
						avgCyclePeriod+stdCyclePeriod));
}


/*
 *process the incoming packet
 */
bool
AquaSimUwan::RecvProcess(Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this);
  UwanSyncHeader SYNC_h;
  AquaSimHeader ash;
  MacHeader mach;
  AquaSimPtTag ptag;
  p->PeekPacketTag(ptag);
  p->RemoveHeader(ash);
  p->RemoveHeader(mach);
  p->RemoveHeader(SYNC_h);

	AquaSimAddress dst = mach.GetDA();
	AquaSimAddress src = mach.GetSA();

    if( ash.GetErrorFlag() )
    {
     	//printf("broadcast:node %d  gets a corrupted packet at  %f\n",index_,NOW);
      NS_LOG_WARN("RecvProcess: Error/Collision. Dropping packet:" << p);
      /*if(drop_)
			drop_->recv(p,"Error/Collision");
     	else*/
      p=0;
     	return false;
    }

	m_neighbors.insert(src);		//update the neighbor list
	m_CL.insert(src);			//update the contact list

	SYNC_h.SetCyclePeriod(SYNC_h.GetCyclePeriod() - PRE_WAKE_TIME);
  p->AddHeader(SYNC_h);
  p->AddHeader(mach);
  p->AddHeader(ash);

	if( (ptag.GetPacketType() == AquaSimPtTag::PT_UWAN_HELLO) ||
        (ptag.GetPacketType() == AquaSimPtTag::PT_UWAN_SYNC) ) {
		//the process to hello packet is same to SYNC packet
		m_wakeSchQueue.Push(Seconds(SYNC_h.GetCyclePeriod())+Simulator::Now(), src,
                          Seconds(SYNC_h.GetCyclePeriod()));
		//m_wakeSchQueue.Print(2*m_maxPropTime, m_maxTxTime, false, index_);
	}
	else {
		/*
		 * it must be data packet. we should extract the SYNC hdr & missing list
		 */
			/*either unicasted or broadcasted*/
			//need overhearing!
			//update the schedule queue
			//then send packet to upper layers
			if( m_device->GetAddress() == dst )
        NS_LOG_INFO("RecvProcess: node(" << m_device->GetNode() << ")" );
				//printf("node(%d) recv %s\n", index_, packet_info.name(cmh->ptype()));

			m_wakeSchQueue.Push(Seconds(SYNC_h.GetCyclePeriod())+Simulator::Now(), src,
                            Seconds(SYNC_h.GetCyclePeriod()) );
			//m_wakeSchQueue.Print(2*m_maxPropTime, m_maxTxTime, false, index_);
      p->Print(std::cout);

			//extract Missing list
			ProcessMissingList(p, src);  //hello is sent to src in this function

			if( dst == m_device->GetAddress() || dst == AquaSimAddress::GetBroadcast() ) {
				SendUp(p);
				return true;
			}

	}
	//packet sent to other nodes will be freed
  p=0;
	return false;
}

/*
 * process the outgoing packet
 */
bool
AquaSimUwan::TxProcess(Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this);

	/* because any packet which has nothing to do with this node is filtered by
	 * RecvProcess(), p must be qualified packet.
	 * Simply cache the packet to simulate the pre-knowledge of next transmission time
	 */

  AquaSimHeader ash;
  p->RemoveHeader(ash);
  ash.SetSize(1600);
  p->AddHeader(ash);

  m_packetQueue.push(p);
  //callback to higher level, should be implemented differently
	//Scheduler::instance().schedule(&callback_handler,&callback_event, UWAN_CALLBACK_DELAY);
  return true;
}


void
AquaSimUwan::SYNCSchedule(bool initial)
{
  NS_LOG_FUNCTION(this);

	//time is not well scheduled!!!!!
	Time now = Simulator::Now();
	m_nextCyclePeriod = m_initialCyclePeriod + now;
	if( initial ) {
    Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
		Time RandomDelay = Seconds(m_rand->GetValue(0.0, m_initialCyclePeriod.ToDouble(Time::S)) );
		m_wakeSchQueue.Push(m_nextCyclePeriod+RandomDelay, AquaSimAddress::ConvertFrom(m_device->GetAddress()) , m_nextCyclePeriod+RandomDelay-now);
		//m_wakeSchQueue.Print(2*m_maxPropTime, m_maxTxTime, true, index_);
		SendFrame(MakeSYNCPkt(m_nextCyclePeriod-now), true, RandomDelay);
		return;
	}

	//m_nextCyclePeriod = GenNxCyclePeriod();
	//check whether next cycle period is available.
	if( ! m_wakeSchQueue.CheckGuardTime(m_nextCyclePeriod,
                Seconds(2*m_maxPropTime.ToDouble(Time::S)), m_maxTxTime) ) {
		//if it overlaps with others, re-generate a cycle period
		m_nextCyclePeriod = m_wakeSchQueue.GetAvailableSendTime(now+m_wakePeriod,
						m_nextCyclePeriod, Seconds(2*m_maxPropTime.ToDouble(Time::S)), m_maxTxTime);
	}

	m_wakeSchQueue.Push(m_nextCyclePeriod, AquaSimAddress::ConvertFrom(m_device->GetAddress()) , m_nextCyclePeriod-now);
	//m_wakeSchQueue.Print(2*m_maxPropTime, m_maxTxTime, true, index_);
	SendFrame(MakeSYNCPkt(m_nextCyclePeriod-now),true);
}


void
AquaSimUwan::Start()
{
	//init WakeSchQueue. Before sleep, Wake Schedule Queue will pop this value.
	//m_wakeSchQueue.Push(0.0, index_, -1); //the timer will not start
	//m_device->SetTransmissionStatus(NIDLE);
  UwanSyncHeader hdr_s;

	SYNCSchedule(true);
	m_maxTxTime = Seconds(1610*m_encodingEfficiency/m_bitRate);
	m_helloTxLen = Seconds(hdr_s.GetSize()*8*m_encodingEfficiency/m_bitRate);
	m_listenPeriod = Seconds(10*m_helloTxLen.ToDouble(Time::S)) +
                    Seconds(2*m_maxPropTime.ToDouble(Time::S)) + m_maxTxTime;
	m_wakePeriod = m_listenPeriod + m_maxTxTime;
}


/*
 * send out one packet from upper layer
 */
void
AquaSimUwan::SendoutPkt(Time NextCyclePeriod)
{
  NS_LOG_FUNCTION(this);

	if( m_packetQueue.empty() ) {
			return; /*because there is no packet, this node cannot sendout packet.
					 * This is due to the stupid idea proposed by the authors of this protocol.
					 * They think mac protocol cannot when it will sendout the next packet.
					 * However, even a newbie knows it is impossible.
					 */
	}

	//get a packet cached in the queue
	Ptr<Packet> pkt = m_packetQueue.front();
	m_packetQueue.pop();
	m_numPktSend++;
	//SendInfo();

  AquaSimHeader ash;
  UwanSyncHeader hdr_s;
  MacHeader mach;
  pkt->RemoveHeader(ash);
  pkt->RemoveHeader(mach);
  pkt->RemoveHeader(hdr_s);
	//hdr_uwvb* vbh = hdr_uwvb::access(pkt);
	/*next_hop() is set in IP layerequal to the */

	//fill the SYNC & Missing list header
	FillSYNCHdr(pkt, NextCyclePeriod-Simulator::Now());
	//whether backoff?
	FillMissingList(pkt);

  ash.SetDirection(AquaSimHeader::DOWN);
  //ash.addr_type()=NS_AF_ILINK;

	//AquaSimAddress next_hop = AquaSimAddress::GetBroadcast();  //not used...

	if( m_neighbors.size() != 0 ) {
		std::set<AquaSimAddress>::iterator pos = m_neighbors.begin();
		for(uint i=0; i<m_nextHopNum; i++, pos++);
		ash.SetNextHop(*pos);
		m_nextHopNum = (m_nextHopNum+1)%m_neighbors.size();
		//vbh->target_id.addr_ = ash.GetNextHop();
	}

	mach.SetDA(ash.GetNextHop());
	mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );

  pkt->AddHeader(hdr_s);
  pkt->AddHeader(mach);
  pkt->AddHeader(ash);

	SendFrame(pkt, false);
}



void
AquaSimUwan::ProcessMissingList(Ptr<Packet> pkt, AquaSimAddress src)
{
  NS_LOG_FUNCTION(this);

  uint8_t *data;
  pkt->CopyData(data,pkt->GetSize());
  /*
	uint node_num_ = *((uint*)data);
	data += sizeof(uint);*/
  int nodeNum = data[0];
  AquaSimAddress tmp_addr;

	for(int i=1; i<nodeNum; i++ ) {
		tmp_addr = data[i];//*((AquaSimAddress*)data);
		if( m_device->GetAddress() == tmp_addr ) {
			//make and send out the hello packet
			Ptr<Packet> p = Create<Packet>();
      UwanSyncHeader hdr_s;
      AquaSimHeader ash;
      MacHeader mach;
      AquaSimPtTag ptag;

      hdr_s.SetCyclePeriod(m_nextCyclePeriod.ToDouble(Time::S) -
			     Simulator::Now().ToDouble(Time::S) );

			ash.SetNextHop(src);
      ash.SetDirection(AquaSimHeader::DOWN);
      //ash.addr_type()=NS_AF_ILINK;
      ptag.SetPacketType(AquaSimPtTag::PT_UWAN_HELLO);
			ash.SetSize(hdr_s.GetSize());

      mach.SetDA(src);
    	mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );

      p->AddHeader(hdr_s);
      p->AddHeader(mach);
      p->AddHeader(ash);

      p->AddPacketTag(ptag);
			//rand the sending slot and then send out
      Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
      double randHelloDelay = m_rand->GetValue(0,10)*m_helloTxLen.ToDouble(Time::S);
			SendFrame(p, true, Seconds(randHelloDelay));   //hello should be delayed!!!!!!
			return;
		}
		//data += sizeof(AquaSimAddress);
	}

}

void AquaSimUwan::DoDispose()
{
  while(!m_packetQueue.empty()) {
    m_packetQueue.front()=0;
    m_packetQueue.pop();
  }
  for (std::set<AquaSimUwan_PktSendTimer *>::iterator it=m_pktSendTimerSet.begin(); it!=m_pktSendTimerSet.end(); ++it) {
    delete (*it);
  }
  m_pktSendTimerSet.clear();
  m_rand=0;
  AquaSimMac::DoDispose();
}

NS_OBJECT_ENSURE_REGISTERED(ScheduleQueue);

TypeId
ScheduleQueue::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::ScheduleQueue")
    ;
  return tid;
}

void
ScheduleQueue::Push(Time SendTime, AquaSimAddress node_id, Time Interval)
{
	ScheduleTime* newElem = new ScheduleTime(SendTime, node_id, m_mac);
	newElem->timer_.SetFunction(&AquaSimUwan_WakeTimer::expire,&newElem->timer_);
	newElem->Start(Interval);

	ScheduleTime* pos = m_head->next_;
	ScheduleTime* pre_pos = m_head;

	//find the position where new element should be insert
	while( pos != NULL ) {
		if( pos->SendTime_ > SendTime ) {
			break;
		}
		else {
			pos = pos->next_;
			pre_pos = pre_pos->next_;
		}

	}
	/*
	 * insert new element after pre_pos
	 */
	newElem->next_ = pos;
	pre_pos->next_ = newElem;
}


//get the top element, but not pop it
ScheduleTime*
ScheduleQueue::Top()
{
	return m_head->next_;
}

//pop the top element
void
ScheduleQueue::Pop()
{
	if( m_head->next_ != NULL ) {

		ScheduleTime* tmp = m_head->next_;
		m_head->next_ = m_head->next_->next_;

    //may not be necessary due to CANCEL_ON_DESTROY
		if( tmp->timer_.IsRunning() ) {
			tmp->timer_.Cancel();
		}

		delete tmp;
	}
}


bool
ScheduleQueue::CheckGuardTime(Time SendTime, Time GuardTime, Time MaxTxTime)
{
	ScheduleTime* pos = m_head->next_;
	ScheduleTime* pre_pos = m_head;

	while( pos != NULL && SendTime > pos->SendTime_ ) {
		pos = pos->next_;
		pre_pos = pre_pos->next_;
	}

	/*now, pos->SendTime > SendTime > pre_pos->SendTime
	 *start to check the sendtime.
	 */
	if( pos == NULL ) {
		if( pre_pos == m_head )
			return true;
		else {
			if( SendTime - pre_pos->SendTime_ >= GuardTime )
				return true;
			else
				return false;
		}
	}
	else {
		if( pre_pos == m_head ) {
			if( (pos->SendTime_ - SendTime) > (GuardTime + MaxTxTime) )
				return true;
			else
				return false;
		}
		else {
			if( ((pos->SendTime_ - SendTime) > (GuardTime + MaxTxTime))
				&& (SendTime - pre_pos->SendTime_ >= GuardTime) )
				return true;
			else
				return false;
		}
	}
}



Time
ScheduleQueue::GetAvailableSendTime(Time StartTime,
							Time OriginalSchedule, Time GuardTime, Time MaxTxTime)
{
	ScheduleTime* pos = m_head->next_;
	ScheduleTime* pre_pos = m_head;

	Time DeltaTime = Seconds(0.0);
	while( pos != NULL && StartTime > pos->SendTime_ ) {
		pos = pos->next_;
		pre_pos = pre_pos->next_;
	}

	while( pos != NULL ) {
		DeltaTime = pos->SendTime_ - pre_pos->SendTime_ -
                        (Seconds(2*GuardTime.ToDouble(Time::S)) + MaxTxTime);
		if( DeltaTime.IsPositive() ) {
      Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
      double min = pre_pos->SendTime_.ToDouble(Time::S) +
                    GuardTime.ToDouble(Time::S) +
                    MaxTxTime.ToDouble(Time::S);
      double max = pre_pos->SendTime_.ToDouble(Time::S) +
                    DeltaTime.ToDouble(Time::S);
      return Seconds(rand->GetValue(min,max));
		}

		pos = pos->next_;
		pre_pos = pre_pos->next_;
	}

	//there is no available interval, so the time out of range of this queue is returned.
	/*
	 * Before calling this function, OriginalSchedule collides with other times,
	 * so originalSchedule is at most pre_pos->SendTime_ + GuardTime + MaxTxTime.
	 * Otherwise, it cannot collides.
	 */
	return pre_pos->SendTime_ + MaxTxTime + GuardTime ;
}


void
ScheduleQueue::ClearExpired(Time CurTime)
{
	ScheduleTime* NextSch = NULL;
	while( (NextSch = Top()) && NextSch->SendTime_ < CurTime ) {
		Pop();
	}
}


void
ScheduleQueue::Print(Time GuardTime, Time MaxTxTime, bool IsMe, AquaSimAddress index)
{
	ScheduleTime* pos = m_head->next_;
	/*char file_name[30];
	strcpy(file_name, "schedule_");
	file_name[strlen(file_name)+1] = '\0';
	file_name[strlen(file_name)] = char(index+'0');
	FILE* stream = fopen(file_name, "a");
	*/
	if( IsMe )
	  NS_LOG_INFO("I send ");
	while( pos != NULL ) {
	    NS_LOG_INFO("(" << pos->SendTime_ << "--" <<
			pos->SendTime_+MaxTxTime << ", " <<
			pos->SendTime_+GuardTime+MaxTxTime);
	   	//fprintf(stream, "(%f--%f, %f) ", pos->SendTime_,
		//	pos->SendTime_+MaxTxTime, pos->SendTime_+GuardTime+MaxTxTime);
		pos = pos->next_;
	}
	//fprintf(stream, "\n");
	//fclose(stream);
}

} // namespace ns3
