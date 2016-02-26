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

#include "aqua-sim-mac-sfama.h"
#include "aqua-sim-header.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimSFama");
NS_OBJECT_ENSURE_REGISTERED(AquaSimSFama);


/*expire functions and handle functions*/
void
AquaSimSFama_Wait_Send_Timer::expire()
{
	m_mac->WaitSendTimerProcess(m_pkt);
	m_pkt = NULL;  /*reset pkt_*/
}

void AquaSimSFama_Wait_Reply_Timer::expire()
{
	m_mac->WaitReplyTimerProcess();
}


void AquaSimSFama_Backoff_Timer::expire()
{
	m_mac->BackoffTimerProcess();
}

void AquaSimSFama_DataSend_Timer::expire()
{
	m_mac->DataSendTimerProcess();
}


AquaSimSFama::AquaSimSFama():m_status(IDLE_WAIT), m_guardTime(0.00001),
    m_slotLen(0), m_isInRound(false), m_isInBackoff(false),
    m_maxBackoffSlots(4), m_maxBurst(1), m_dataSendingInterval(0.0000001),
  m_waitSendTimer(this), m_waitReplyTimer(this),
  m_backoffTimer(this), m_datasendTimer(this)
{
  m_slotNumHandler = 0;
  //TODO add to typeid once created.
  //bind("guard_time_", &guard_time_);
	//bind("m_maxBackoffSlots", &m_maxBackoffSlots);
	//bind("m_maxBurst", &m_maxBurst);

  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();

  Simulator::Schedule(Seconds(0.05) /*callback delay*/, &AquaSimSFama::InitSlotLen, this);
}

TypeId
AquaSimSFama::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimSFama")
    .SetParent<AquaSimMac>()
    .AddConstructor<AquaSimSFama>()
    //.AddAttribute("MaxBurst", "The maximum number of packet burst. default is 1",
     // IntegerValue(1),
	//    MakeIntegerAccessor (&AquaSimFama::m_maxBurst),
      //MakeIntegerChecker<int>())
    ;
  return tid;
}


void
AquaSimSFama::RecvProcess(Ptr<Packet> p)
{
  SFamaHeader SFAMAh;
  AlohaHeader mach;   //TODO change this...
  p->PeekHeader(SFAMAh);
  p->PeekHeader(mach);

  NS_LOG_DEBUG("Time:" << Seconds(Simulator::Now()) << ",node:" << m_device->GetNode() <<
                ",node " << mach.GetDA() << " recv from node " << mach.GetSA());

		switch( SFAMAh.GetPType() ) {
			case SFamaHeader::SFAMA_RTS:
				ProcessRTS(p);
				break;
			case SFamaHeader::SFAMA_CTS:
				ProcessCTS(p);
				break;
			case SFamaHeader::SFAMA_DATA:
				ProcessDATA(p);
				break;
			case SFamaHeader::SFAMA_ACK:
				ProcessACK(p);
				break;
			default:
				/*unknown packet type. error happens*/
        NS_LOG_WARN("RecvProcess: unknown packet type.");
				break;
		}

  p=0;
}


void
AquaSimSFama::TxProcess(Ptr<Packet> p)
{
	//hdr_cmn* cmh = hdr_cmn::access(p);
	//hdr_SFAMA* SFAMAh = hdr_SFAMA::access(p);

  //callback to higher level, should be implemented differently
	//Scheduler::instance().schedule(&callback_handler, &callback_event, 0.0001 /*callback delay*/);

	FillDATA(p);
/*
	SFAMAh->SA = index_;
	SFAMAh->DA = cmh->next_hop();

	cmh->error() = 0;
	cmh->size() += hdr_SFAMA::getSize(hdr_SFAMA::SFAMA_DATA);
	cmh->txtime() = getTxtimeByPktSize(cmh->size());
*/
#ifdef AquaSimSFama_DEBUG
  NS_LOG_DEBUG("TxProcess(before)");
  NS_LOG_DEBUG(PrintAllQ());
#endif
	m_CachedPktQ.push(p);
#ifdef AquaSimSFama_DEBUG
  NS_LOG_DEBUG("TxProcess(after)");
  NS_LOG_DEBUG(PrintAllQ());
#endif
	if( m_CachedPktQ.size() == 1 && GetStatus() == IDLE_WAIT ) {
		PrepareSendingDATA();
	}
}


void
AquaSimSFama::InitSlotLen()
{
  double transmitDistance = 3000.0; /*static for now but should be fixed in the future TODO
        UnderwaterChannel::Transmit_Distance() */
  SFamaHeader SFAMA;
	m_slotLen = m_guardTime +
		GetTxTime(SFAMA.GetSize(SFamaHeader::SFAMA_CTS) +
		transmitDistance/1500.0).ToDouble(Time::S);
}

double
AquaSimSFama::GetTime2ComingSlot(double t)
{
	double numElapseSlot = t/m_slotLen;

	return m_slotLen*(1+numElapseSlot)-t;
}

Ptr<Packet>
AquaSimSFama::MakeRTS(Address recver, int slot_num)
{
	Ptr<Packet> rts_pkt = Create<Packet>();
  AquaSimHeader ash;
  SFamaHeader SFAMAh;
  AlohaHeader mach;   //TODO change this...

	//ash->size() = hdr_SFAMA::getSize(hdr_SFAMA::SFAMA_RTS);
	ash.SetTxTime(GetTxTime(SFAMAh.GetSize(SFamaHeader::SFAMA_RTS)) );
	ash.SetErrorFlag(false);
	ash.SetDirection(AquaSimHeader::DOWN);
	//ash->ptype() = PT_SFAMA;

	mach.SetSA(m_device->GetAddress());
	mach.SetDA(recver);

	SFAMAh.SetPType(SFamaHeader::SFAMA_RTS);
	//SFAMAh->SA = index_;
	//SFAMAh->DA = recver;
	SFAMAh.SetSlotNum(slot_num);

	//rts_pkt->next_ = NULL;

  rts_pkt->AddHeader(ash);
  rts_pkt->AddHeader(SFAMAh);
  rts_pkt->AddHeader(mach);
	return rts_pkt;
}


Ptr<Packet>
AquaSimSFama::MakeCTS(Address rts_sender, int slot_num)
{
	Ptr<Packet> cts_pkt = Create<Packet>();
  AquaSimHeader ash;
  SFamaHeader SFAMAh;
  AlohaHeader mach;   //TODO change this...

	//ash->size() = hdr_SFAMA::getSize(hdr_SFAMA::SFAMA_CTS);
	ash.SetTxTime(GetTxTime(SFAMAh.GetSize(SFamaHeader::SFAMA_CTS)) );
	ash.SetErrorFlag(false);
	ash.SetDirection(AquaSimHeader::DOWN);
	//ash->ptype() = PT_SFAMA;

	mach.SetSA(m_device->GetAddress());
	mach.SetDA(rts_sender);

	SFAMAh.SetPType(SFamaHeader::SFAMA_CTS);
	//SFAMAh->SA = index_;
	//SFAMAh->DA = rts_sender;
	SFAMAh.SetSlotNum(slot_num);

  //cts_pkt->next_ = NULL;

	cts_pkt->AddHeader(ash);
	cts_pkt->AddHeader(SFAMAh);
	cts_pkt->AddHeader(mach);
	return cts_pkt;
}


Ptr<Packet>
AquaSimSFama::FillDATA(Ptr<Packet> data_pkt)
{
  AquaSimHeader ash;
  SFamaHeader SFAMAh;
  AlohaHeader mach;   //TODO change this...
  data_pkt->RemoveHeader(ash);
  data_pkt->RemoveHeader(SFAMAh);
  data_pkt->RemoveHeader(mach);

  //ash->size() = hdr_SFAMA::getSize(hdr_SFAMA::SFAMA_DATA);
  ash.SetTxTime(GetTxTime(SFAMAh.GetSize(SFamaHeader::SFAMA_DATA)) );
  ash.SetErrorFlag(false);
  ash.SetDirection(AquaSimHeader::DOWN);

  mach.SetSA(m_device->GetAddress());
  mach.SetDA(ash.GetNextHop());

  SFAMAh.SetPType(SFamaHeader::SFAMA_DATA);
  //SFAMAh->SA = index_;
  //SFAMAh->DA = ash.GetNextHop();

  data_pkt->AddHeader(ash);
  data_pkt->AddHeader(SFAMAh);
  data_pkt->AddHeader(mach);
	return data_pkt;
}


Ptr<Packet>
AquaSimSFama::MakeACK(Address data_sender)
{
  Ptr<Packet> ack_pkt = Create<Packet>();
  AquaSimHeader ash;
  SFamaHeader SFAMAh;
  AlohaHeader mach;   //TODO change this...

  //ash->size() = hdr_SFAMA::getSize(hdr_SFAMA::SFAMA_ACK);
  ash.SetTxTime(GetTxTime(SFAMAh.GetSize(SFamaHeader::SFAMA_ACK)) );
  ash.SetErrorFlag(false);
  ash.SetDirection(AquaSimHeader::DOWN);
	//ash->ptype() = PT_SFAMA;

  mach.SetSA(m_device->GetAddress());
  mach.SetDA(data_sender);

  SFAMAh.SetPType(SFamaHeader::SFAMA_ACK);
  //SFAMAh->SA = index_;
  //SFAMAh->DA = data_sender;

  ack_pkt->AddHeader(ash);
  ack_pkt->AddHeader(SFAMAh);
  ack_pkt->AddHeader(mach);
	return ack_pkt;
}


/*process all kinds of packets*/

void
AquaSimSFama::ProcessRTS(Ptr<Packet> rts_pkt)
{
  SFamaHeader SFAMAh;
  AlohaHeader mach;   //TODO change this...
  rts_pkt->PeekHeader(SFAMAh);
  rts_pkt->PeekHeader(mach);

	double time2comingslot = GetTime2ComingSlot(Simulator::Now().ToDouble(Time::S));

	if( mach.GetDA() == m_device->GetAddress() ) {
		if ( (GetStatus() == IDLE_WAIT ||
			GetStatus() == WAIT_SEND_RTS ||
			GetStatus() == BACKOFF_FAIR )   		) {

				StopTimers();
				SetStatus(WAIT_SEND_CTS);
				//reply a cts
				m_waitSendTimer.m_pkt = MakeCTS(mach.GetSA(), SFAMAh.GetSlotNum());
        m_waitSendTimer.SetFunction(&AquaSimSFama_Wait_Send_Timer::expire,m_waitSendTimer);
        m_waitSendTimer.Schedule(Seconds(time2comingslot));
		}
	}
	else {
		//do backoff
		double backoff_time = time2comingslot + 1 /*for cts*/+
			SFAMAh.GetSlotNum()*m_slotLen /*for data*/+ 1 /*for ack*/;

		StopTimers();
		SetStatus(BACKOFF);

    m_backoffTimer.SetFunction(&AquaSimSFama_Backoff_Timer::expire,m_backoffTimer);
    m_backoffTimer.Schedule(Seconds(backoff_time));
	}
}

void
AquaSimSFama::ProcessCTS(Ptr<Packet> cts_pkt)
{
  SFamaHeader SFAMAh;
  AlohaHeader mach;   //TODO change this...
  cts_pkt->PeekHeader(SFAMAh);
  cts_pkt->PeekHeader(mach);

	double time2comingslot = GetTime2ComingSlot(Simulator::Now().ToDouble(Time::S));

	if( mach.GetDA() == m_device->GetAddress() && GetStatus() == WAIT_RECV_CTS ) {

		//send DATA
		StopTimers();
		SetStatus(WAIT_SEND_DATA);
		//send the packet
		m_waitSendTimer.m_pkt = NULL;
    m_waitSendTimer.SetFunction(&AquaSimSFama_Wait_Send_Timer::expire,m_waitSendTimer);
    m_waitSendTimer.Schedule(Seconds(time2comingslot));

		double wait_time = (1+SFAMAh.GetSlotNum())*m_slotLen+time2comingslot;
		if( time2comingslot < 0.1 ) {
			wait_time += m_slotLen;
		}

    m_waitReplyTimer.SetFunction(&AquaSimSFama_Wait_Reply_Timer::expire,m_waitReplyTimer);
    m_waitReplyTimer.Schedule(Seconds(wait_time));
	}
	else {
		//do backoff
		double backoff_time = SFAMAh.GetSlotNum()*m_slotLen /*for data*/+
			1 /*for ack*/+time2comingslot;

		StopTimers();
		SetStatus(BACKOFF);

    m_backoffTimer.SetFunction(&AquaSimSFama_Backoff_Timer::expire,m_backoffTimer);
    m_backoffTimer.Schedule(Seconds(backoff_time));
	}

}

void
AquaSimSFama::ProcessDATA(Ptr<Packet> data_pkt)
{
  AlohaHeader mach;   //TODO change this...
  data_pkt->PeekHeader(mach);

	if( mach.GetDA() == m_device->GetAddress() && GetStatus() == WAIT_RECV_DATA ) {
		//send ACK
		StopTimers();
		SetStatus(WAIT_SEND_ACK);

		m_waitSendTimer.m_pkt = MakeACK(mach.GetSA());
    m_waitSendTimer.SetFunction(&AquaSimSFama_Wait_Send_Timer::expire,m_waitSendTimer);
    m_waitSendTimer.Schedule(Seconds(GetTime2ComingSlot(Simulator::Now().ToDouble(Time::S))));

		/*send packet to upper layer*/
    SFamaHeader SFAMAh;
    AquaSimHeader ash;
    data_pkt->RemoveHeader(ash);
    data_pkt->PeekHeader(SFAMAh);
    //ash.setsize(SFAMAh.GetSize(SFamaHeader::SFAMA_DATA));
    data_pkt->AddHeader(ash);
		SendUp(data_pkt->Copy()); /*the original one will be released*/
	}
	else {
		//do backoff
		double backoff_time = 1+GetTime2ComingSlot(Simulator::Now().ToDouble(Time::S)) /*for ack*/;

		StopTimers();
		SetStatus(BACKOFF);

    m_backoffTimer.SetFunction(&AquaSimSFama_Backoff_Timer::expire,m_backoffTimer);
    m_backoffTimer.Schedule(Seconds(backoff_time));
	}
}


void
AquaSimSFama::ProcessACK(Ptr<Packet> ack_pkt)
{
  AlohaHeader mach;   //TODO change this...
  ack_pkt->PeekHeader(mach);

  NS_LOG_DEBUG("ProcessACK(before)");
#ifdef AquaSimSFama_DEBUG
  NS_LOG_DEBUG(PrintAllQ());
#endif
  NS_LOG_DEBUG("ProcessACK: Status is " << GetStatus());

	if( mach.GetDA() == m_device->GetAddress() && GetStatus() == WAIT_RECV_ACK ) {
		StopTimers();
		SetStatus(IDLE_WAIT);

		//release data packets have been sent successfully
		ReleaseSentPkts();

		//start to prepare for sending next DATA packet
		PrepareSendingDATA();
	}
	/*
	 * consider the multi-hop case, cannot stop timers here
	else {
		StopTimers();
		SetStatus(IDLE_WAIT);

		//release data packets have been sent successfully
		PrepareSendingDATA();
	}
	*/
  NS_LOG_DEBUG("ProcessACK(after)");
#ifdef AquaSimSFama_DEBUG
  NS_LOG_DEBUG(PrintAllQ());
#endif
}


void
AquaSimSFama::StopTimers()
{
	m_waitSendTimer.Cancel();
	m_waitReplyTimer.Cancel();
	m_backoffTimer.Cancel();
}


void
AquaSimSFama::ReleaseSentPkts()
{
	Ptr<Packet> tmp = Create<Packet>();

	while( !m_sendingPktQ.empty() ) {
		tmp = m_sendingPktQ.front();
		m_sendingPktQ.pop();
		tmp=0;
	}
}

void
AquaSimSFama::PrepareSendingDATA()
{
	std::queue<Ptr<Packet> > tmpQ_;
	Ptr<Packet> tmp_pkt;
	Address recver_addr;
	int pkt_num = 1;

	if( m_sendingPktQ.empty() && m_CachedPktQ.empty() ) {
		return;
	}

	if( !m_sendingPktQ.empty() && GetStatus() == IDLE_WAIT  ) {
      AlohaHeader mach;   //TODO change this...
      m_sendingPktQ.front()->PeekHeader(mach);
      recver_addr = mach.GetDA();

	} else if( !m_CachedPktQ.empty() && GetStatus() == IDLE_WAIT ) {
    NS_LOG_DEBUG("PrepareSendingDATA(before)");
#ifdef AquaSimSFama_DEBUG
    NS_LOG_DEBUG(PrintAllQ());
#endif
    AlohaHeader mach;   //TODO change this...

		tmp_pkt = m_CachedPktQ.front();
    tmp_pkt->PeekHeader(mach);
    recver_addr = mach.GetDA();
		m_CachedPktQ.pop();
		m_sendingPktQ.push(tmp_pkt);
		pkt_num = 1;

		/*get at most m_maxBurst DATA packets with same receiver*/
		while( (pkt_num < m_maxBurst) && (!m_CachedPktQ.empty()) ) {
			tmp_pkt = m_CachedPktQ.front();
			m_CachedPktQ.pop();

			if( recver_addr == mach.GetDA() ) {
				m_sendingPktQ.push(tmp_pkt);
				pkt_num ++;
			}
			else {
				tmpQ_.push(tmp_pkt);
			}

		}

		//make sure the rest packets are stored in the original order
		while( !m_CachedPktQ.empty() ) {
			tmpQ_.push(m_CachedPktQ.front());
			m_CachedPktQ.pop();
		}

		while( !tmpQ_.empty() ) {
			m_CachedPktQ.push(tmpQ_.front());
			tmpQ_.pop();
		}

  NS_LOG_DEBUG("PrepareSendingDATA(after)");
#ifdef AquaSimSFama_DEBUG
  NS_LOG_DEBUG(PrintAllQ());
#endif
	}

  SFamaHeader SFAMAh;
	double additional_txtime = GetPktTrainTxTime()-
			GetTxTime(SFAMAh.GetSize(SFamaHeader::SFAMA_CTS)).ToDouble(Time::S);


	ScheduleRTS(recver_addr, (additional_txtime/m_slotLen)+1 /*for ceil*/
	+1/*the basic slot*/ );
}

double
AquaSimSFama::GetPktTrainTxTime()
{
	double txtime = 0.0;

	int q_size = m_sendingPktQ.size();
  NS_LOG_DEBUG("GetPktTrainTxTime(before) \n");
#ifdef AquaSimSFama_DEBUG
	NS_LOG_DEBUG(PrintAllQ());
#endif
  AquaSimHeader ash;
	for(int i=0; i<q_size; i++ ) {
    m_sendingPktQ.front()->PeekHeader(ash);
    txtime += ash.GetTxTime().ToDouble(Time::S);
		m_sendingPktQ.push(m_sendingPktQ.front());
		m_sendingPktQ.pop();
	}
NS_LOG_DEBUG("GetPktTrainTxTime(after) \n");
#ifdef AquaSimSFama_DEBUG
NS_LOG_DEBUG(PrintAllQ());
#endif

	txtime += (q_size-1)*m_dataSendingInterval;

	return txtime;
}


void
AquaSimSFama::ScheduleRTS(Address recver, int slot_num)
{
	double backoff_time = RandBackoffSlots()*m_slotLen+GetTime2ComingSlot(Simulator::Now().ToDouble(Time::S));
	SetStatus(WAIT_SEND_RTS);
	m_waitSendTimer.m_pkt = MakeRTS(recver, slot_num);
  m_waitSendTimer.SetFunction(&AquaSimSFama_Wait_Send_Timer::expire,m_waitSendTimer);
  m_waitSendTimer.Schedule(Seconds(backoff_time));
}


int
AquaSimSFama::RandBackoffSlots()
{
	return (int)m_rand->GetValue(0.0,(double)m_maxBackoffSlots);
}


void
AquaSimSFama::SendPkt(Ptr<Packet> pkt)
{
  AquaSimHeader ash;
  SFamaHeader SFAMAh;
  AlohaHeader mach;   //TODO change this...
  pkt->RemoveHeader(ash);
  pkt->PeekHeader(SFAMAh);

	ash.SetDirection(AquaSimHeader::DOWN);

	Time txtime=ash.GetTxTime();

	//status_handler.is_ack() = false;
	if( SFAMAh.GetPType() == SFamaHeader::SFAMA_CTS ) {
    m_slotNumHandler = SFAMAh.GetSlotNum();
	}
	/*else if ( SFAMAh->packet_type == hdr_SFAMA::SFAMA_ACK ) {
		status_handler.is_ack() = true;
	}*/

	switch( m_device->TransmissionStatus() ) {
		case SLEEP:
			PowerOn();
		case RECV:
			NS_LOG_INFO("RECV-SEND Collision!!!!!");
			pkt=0;
			//do backoff??
			break;
		case NIDLE:
			m_device->SetTransmissionStatus(SEND);
			ash.SetTimeStamp(Simulator::Now());
			pkt->PeekHeader(mach);
			NS_LOG_DEBUG(Seconds(Simulator::Now()) << ": node " << mach.GetSA() <<
					       " send to node " << mach.GetDA() );
			pkt->AddHeader(ash);
			SendDown(pkt);
			Simulator::Schedule(txtime, &AquaSimSFama::SlotInitHandler,this);
			break;
		default:
			//status is SEND, send too fast
      NS_LOG_INFO("Node:" << m_device->GetNode() << " send data too fast");
			pkt=0;
			break;
	}

	return;
}

void
AquaSimSFama::StatusProcess(int slotnum)
{
	m_device->SetTransmissionStatus(NIDLE);

	switch(GetStatus()) {
	  case WAIT_SEND_RTS:
		slotnum = 1;
		SetStatus(WAIT_RECV_CTS);
		break;
	  case WAIT_SEND_CTS:
		//slotnum += 1;
		SetStatus(WAIT_RECV_DATA);
		break;
	  case WAIT_SEND_DATA:
		//cannot reach here
		slotnum = 1;
		SetStatus(WAIT_RECV_ACK);
		//wait_reply time has been scheduled.
		return;
	  case WAIT_SEND_ACK:
		WaitReplyTimerProcess(true); //go to next round
		return;
	  default:
		#ifdef AquaSimSFama_DEBUG
		switch (GetStatus() ) {
		  case IDLE_WAIT:
      NS_LOG_WARN("Node:" << m_device->GetNode() << ": status error:IDLE_WAIT");
		  break;
		  case WAIT_RECV_CTS:
      NS_LOG_WARN("Node:" << m_device->GetNode() << ": status error:WAIT_RECV_CTS");
			break;
		  case WAIT_RECV_DATA:
      NS_LOG_WARN("Node:" << m_device->GetNode() << ": status error:WAIT_RECV_DATA");
			break;
		  case BACKOFF:
      NS_LOG_WARN("Node:" << m_device->GetNode() << ": status error:BACKOFF");
			break;
		}
		#endif
		break;

	}

// 	Time time2comingslot = getTime2ComingSlot(Simulator::Now());
// 	if ( time2comingslot <  0.1 ) {
// 	  slotnum++;
// 	}

	//if( ! status_handler.is_ack() ) {
  m_waitReplyTimer.SetFunction(&AquaSimSFama_Wait_Reply_Timer::expire,m_waitReplyTimer);
  m_waitReplyTimer.Schedule(Seconds(m_slotLen*slotnum+GetTime2ComingSlot(Simulator::Now().ToDouble(Time::S))));
	//}
	/*else {
	  status_handler.is_ack() = false;
	}*/

	return;
}

void
AquaSimSFama::BackoffTimerProcess()
{
	SetStatus(IDLE_WAIT);
	PrepareSendingDATA();
}

void
AquaSimSFama::WaitSendTimerProcess(Ptr<Packet> pkt)
{
	if( NULL == pkt ) {
    m_datasendTimer.SetFunction(&AquaSimSFama_DataSend_Timer::expire,m_datasendTimer);
    m_datasendTimer.Schedule(Seconds(0.00001));
	}
	else {
		SendPkt(pkt);
	}
}

void
AquaSimSFama::WaitReplyTimerProcess(bool directcall)
{
	/*do backoff*/
	double backoff_time = RandBackoffSlots()*m_slotLen + GetTime2ComingSlot(Simulator::Now().ToDouble(Time::S));
	#ifdef AquaSimSFama_DEBUG
	if( !directcall )
    NS_LOG_WARN(Seconds(Simulator::Now()) << " node " << m_device->GetNode() << "TIME OUT!!!!!");
	#endif  //AquaSimSFama_DEBUG
	if( directcall ) {
		SetStatus(BACKOFF_FAIR);
    m_backoffTimer.SetFunction(&AquaSimSFama_Backoff_Timer::expire,m_backoffTimer);
    m_backoffTimer.Schedule(Seconds(GetTime2ComingSlot(Simulator::Now().ToDouble(Time::S))));
	} else {
		SetStatus(BACKOFF);
    m_backoffTimer.SetFunction(&AquaSimSFama_Backoff_Timer::expire,m_backoffTimer);
    m_backoffTimer.Schedule(Seconds(backoff_time));
	}

}


void
AquaSimSFama::DataSendTimerProcess()
{
  NS_LOG_DEBUG("DataSendTimerProcess(before) \n");
#ifdef AquaSimSFama_DEBUG
  NS_LOG_DEBUG(PrintAllQ());
#endif

	if( !m_sendingPktQ.empty() ) {
		Ptr<Packet> pkt = m_sendingPktQ.front();
    AquaSimHeader ash;
    pkt->PeekHeader(ash);
		Time txtime = ash.GetTxTime();
		m_BackupSendingPktQ.push(pkt);

		m_sendingPktQ.pop();

		SendDataPkt(pkt->Copy());

    m_datasendTimer.SetFunction(&AquaSimSFama_DataSend_Timer::expire,m_datasendTimer);
    m_datasendTimer.Schedule(Seconds(m_dataSendingInterval)+txtime);
	}
	else {
	  while( !m_BackupSendingPktQ.empty() ) {
		//push all packets into m_sendingPktQ. After getting ack, release them
		m_sendingPktQ.push(m_BackupSendingPktQ.front());
		m_BackupSendingPktQ.pop();
	  }
	  //status_handler.is_ack() = false;
    Simulator::Schedule(Seconds(0.0000001),&AquaSimSFama::SlotInitHandler,this);
	  /*
		UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
		n->SetTransmissionStatus(IDLE);
		m_waitReplyTimer.resched(m_slotLen+getTime2ComingSlot(Simulator::Now()));
			switch(getStatus()) {
			  case WAIT_SEND_RTS:
				setStatus(WAIT_RECV_CTS);
				break;
			  case WAIT_SEND_CTS:
				setStatus(WAIT_RECV_DATA);
				break;
			  case WAIT_SEND_DATA:
				setStatus(WAIT_RECV_ACK);
				break;
			  default:
				printf("status error!\n");
				break;

			}
			*/
	}
  NS_LOG_DEBUG("DataSendTimerProcess(after) \n");
#ifdef AquaSimSFama_DEBUG
  NS_LOG_DEBUG(PrintAllQ());
#endif
}


void
AquaSimSFama::SendDataPkt(Ptr<Packet> pkt)
{
  AquaSimHeader ash;
  AlohaHeader mach;   //TODO change this...
  pkt->RemoveHeader(ash);

	ash.SetDirection(AquaSimHeader::DOWN);

	switch( m_device->TransmissionStatus() ) {
		case SLEEP:
			PowerOn();
		case RECV:
      NS_LOG_INFO("RECV-SEND Collision!!!!!");
      pkt=0;
			//do backoff??
			break;
		case NIDLE:
			m_device->SetTransmissionStatus(SEND);
			ash.SetTimeStamp(Simulator::Now());
      pkt->PeekHeader(mach);
      NS_LOG_DEBUG(Seconds(Simulator::Now()) << ": node " << mach.GetSA() <<
            " send to node " << mach.GetDA() );
      pkt->AddHeader(ash);
			SendDown(pkt);
			break;
		default:
			//status is SEND, send too fast
      NS_LOG_INFO("Node:" << m_device->GetNode() << " send data too fast");
      pkt=0;
			break;
	}

	return;
}

void
AquaSimSFama::SetStatus(enum AquaSimSFama_Status status)
{
	/*if( status == BACKOFF ) {
	  status = status; //????
	}*/
    m_status = status;
}

enum AquaSimSFama_Status
AquaSimSFama::GetStatus()
{
    return m_status;
}

void
AquaSimSFama::SlotInitHandler()
{
  StatusProcess(m_slotNumHandler);
}

#ifdef AquaSimSFama_DEBUG
void
AquaSimSFama::PrintAllQ()
{
  NS_LOG_INFO("Time " << Seconds(Simulator::Now()) << " node " <<
                m_device->GetNode() << ". m_CachedPktQ:");
  NS_LOG_INFO(m_CachedPktQ);
  NS_LOG_INFO("m_sendingPktQ:");
  NS_LOG_INFO(m_sendingPktQ);
}

void
AquaSimSFama::PrintQ(std::queue< Ptr<Packet> >& my_q)
{
	std::queue<Ptr<Packet> > tmp_q;
  AquaSimHeader ash;
  while(!my_q.empty()) {
    my_q.front()->PeekHeader(ash);
    printf("%d\t", ash.GetUId());
		tmp_q.push(my_q.front());
		my_q.pop();
	}

	while(!tmp_q.empty()) {
	  my_q.push(tmp_q.front());
	  tmp_q.pop();
	}
}

#endif

} //namespace ns3
