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
#include "aqua-sim-header-mac.h"
#include "aqua-sim-pt-tag.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/integer.h"

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


AquaSimSFama::AquaSimSFama():m_status(IDLE_WAIT), m_guardTime(1), // change guard_time from 0.00001
    m_slotLen(0), m_isInRound(false), m_isInBackoff(false),
    m_maxBackoffSlots(4), m_maxBurst(1), m_dataSendingInterval(0.0000001),
  m_waitSendTimer(this), m_waitReplyTimer(this),
  m_backoffTimer(this), m_datasendTimer(this)
{
	NS_LOG_FUNCTION(this);

	m_rand = CreateObject<UniformRandomVariable> ();
  m_slotNumHandler = 0;

  Simulator::Schedule(Seconds(0.05) /*callback delay*/, &AquaSimSFama::InitSlotLen, this);
}

AquaSimSFama::~AquaSimSFama()
{
}

TypeId
AquaSimSFama::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimSFama")
    .SetParent<AquaSimMac>()
    .AddConstructor<AquaSimSFama>()
    .AddAttribute("GuardTime", "The guard time in double. Default is 0.00001",
//      DoubleValue(0.00001),
      DoubleValue(1), // change guard_time from 0.00001 due to a previous bug in slot_length calculation
      MakeDoubleAccessor(&AquaSimSFama::m_guardTime),
      MakeDoubleChecker<double>())
    .AddAttribute("MaxBackoffSlots", "The maximum number of backoff slots. default is 4",
      IntegerValue(4),
      MakeIntegerAccessor (&AquaSimSFama::m_maxBackoffSlots),
      MakeIntegerChecker<int>())
    .AddAttribute("MaxBurst", "The maximum number of packets in the train. Default is 1",
      IntegerValue(1),
      MakeIntegerAccessor(&AquaSimSFama::m_maxBurst),
      MakeIntegerChecker<int>())
	.AddAttribute("packet_size", "Data packet size, bytes",
	  DoubleValue(50),
	  MakeDoubleAccessor(&AquaSimSFama::m_packet_size),
	  MakeDoubleChecker<double>())
    ;
  return tid;
}

int64_t
AquaSimSFama::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_rand->SetStream(stream);
  return 1;
}

bool
AquaSimSFama::RecvProcess(Ptr<Packet> p)
{
	AquaSimHeader ash;
  SFamaHeader SFAMAh;
  MacHeader mach;

// 	p->RemoveHeader(ash);
//   p->RemoveHeader(SFAMAh);
//   p->PeekHeader(mach);
// 	p->AddHeader(SFAMAh);
// 	p->AddHeader(ash);

  p->RemoveHeader(ash);
  p->RemoveHeader(mach);
  p->RemoveHeader(SFAMAh);
  
  p->AddHeader(mach);
  p->AddHeader(SFAMAh);
  p->AddHeader(ash);


  NS_LOG_DEBUG("Time:" << Simulator::Now().GetSeconds() << ",node:" << m_device->GetNode() <<
                ",node " << mach.GetDA() << " recv from node " << mach.GetSA());

//  	  std::cout << "PACKET TYPE: " << SFAMAh.GetPType() << "\n";

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
	return true;

}


bool
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
	return true;
}


void
AquaSimSFama::InitSlotLen()
{
  SFamaHeader SFAMA;
//	m_slotLen = m_guardTime +
//		GetTxTime(100 +					// SFAMA.GetSize(SFamaHeader::SFAMA_CTS
//		Device()->GetPhy()->GetTransRange()/1500.0).ToDouble(Time::S);

//	m_slotLen = m_guardTime +
//			GetTxTime(SFAMA.GetSize(SFamaHeader::SFAMA_DATA)).ToDouble(Time::S) + Device()->GetPhy()->GetTransRange()/1500.0;

//  std::cout << "PACKET SIZE: " << m_packet_size << "\n";
//  std::cout << "TRANS RANGE: " << Device()->GetPhy()->GetTransRange() << "\n";

  m_slotLen = GetTxTime(m_packet_size).ToDouble(Time::S) + Device()->GetPhy()->GetTransRange()/1500.0 +
			0.5*Device()->GetPhy()->GetTransRange()/1500.0; // guard_time = half of propagation delay

//  m_slotLen = GetTxTime(m_packet_size).ToDouble(Time::S) + Device()->GetPhy()->GetTransRange()/1500.0 + 0.0000001;

//  m_slotLen = GetTxTime(m_packet_size).ToDouble(Time::S) + Device()->GetPhy()->GetTransRange()/1500.0 + 5;


//	std::cout << "SLOT LENGTH: " << m_slotLen << "\n";
}

double
AquaSimSFama::GetTime2ComingSlot(double t)
{
	// Slot number cannot be float!
//	double numElapseSlot = t/m_slotLen;
	uint32_t numElapseSlot = t/m_slotLen;

//	std::cout << "NumElapseSlot: " << numElapseSlot << "\n";

	return m_slotLen*(1+numElapseSlot)-t;
}

Ptr<Packet>
AquaSimSFama::MakeRTS(AquaSimAddress recver, int slot_num)
{
	NS_LOG_FUNCTION(this << recver.GetAsInt() << slot_num);

	Ptr<Packet> rts_pkt = Create<Packet>();
  AquaSimHeader ash;
  SFamaHeader SFAMAh;
  MacHeader mach;
	AquaSimPtTag ptag;

	ash.SetSize(SFAMAh.GetSize(SFamaHeader::SFAMA_RTS));
	ash.SetTxTime(GetTxTime(ash.GetSize()) );
	ash.SetErrorFlag(false);
	ash.SetDirection(AquaSimHeader::DOWN);
	ptag.SetPacketType(AquaSimPtTag::PT_SFAMA);

	mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
	mach.SetDA(recver);

	SFAMAh.SetPType(SFamaHeader::SFAMA_RTS);
	//SFAMAh->SA = index_;
	//SFAMAh->DA = recver;
	SFAMAh.SetSlotNum(slot_num);

	//rts_pkt->next_ = NULL;

	rts_pkt->AddHeader(mach);
	rts_pkt->AddHeader(SFAMAh);
  rts_pkt->AddHeader(ash);
	rts_pkt->AddPacketTag(ptag);
	return rts_pkt;
}


Ptr<Packet>
AquaSimSFama::MakeCTS(AquaSimAddress rts_sender, int slot_num)
{
	NS_LOG_FUNCTION(this << rts_sender.GetAsInt() << slot_num);

	Ptr<Packet> cts_pkt = Create<Packet>();
  AquaSimHeader ash;
  SFamaHeader SFAMAh;
  MacHeader mach;
	AquaSimPtTag ptag;

	ash.SetSize(SFAMAh.GetSize(SFamaHeader::SFAMA_CTS));
	ash.SetTxTime(GetTxTime(ash.GetSize()) );
	ash.SetErrorFlag(false);
	ash.SetDirection(AquaSimHeader::DOWN);
	ptag.SetPacketType(AquaSimPtTag::PT_SFAMA);

	mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
	mach.SetDA(rts_sender);

	SFAMAh.SetPType(SFamaHeader::SFAMA_CTS);
	//SFAMAh->SA = index_;
	//SFAMAh->DA = rts_sender;
	SFAMAh.SetSlotNum(slot_num);

  //cts_pkt->next_ = NULL;

	cts_pkt->AddHeader(mach);
	cts_pkt->AddHeader(SFAMAh);
	cts_pkt->AddHeader(ash);
	cts_pkt->AddPacketTag(ptag);
	return cts_pkt;
}


Ptr<Packet>
AquaSimSFama::FillDATA(Ptr<Packet> data_pkt)
{
	NS_LOG_FUNCTION(this);
	if(AquaSimSFAMA_DEBUG) {
		data_pkt->Print(std::cout);
	}

  AquaSimHeader ash;
  SFamaHeader SFAMAh;
  MacHeader mach;
  data_pkt->RemoveHeader(ash);
  data_pkt->RemoveHeader(SFAMAh);
  data_pkt->RemoveHeader(mach);

  // TODO: FIX THIS! WHY DATA PACKET SIZE IS ALWAYS 47 (see SFAMAh.GetSize())??
//  ash.SetSize(SFAMAh.GetSize(SFamaHeader::SFAMA_DATA));
//  std::cout << "Packet Size:" << ash.GetSize() << "\n";

  ash.SetTxTime(GetTxTime(ash.GetSize()) );
  ash.SetErrorFlag(false);
  ash.SetDirection(AquaSimHeader::DOWN);

  mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
//  mach.SetDA(ash.GetNextHop());
  // Fix DA assignment. MAC-ROUTING TESTS.
//  std::cout << "SA FROM ASH:" << ash.GetSAddr() << "\n";
//  std::cout << "DA FROM ASH:" << ash.GetDAddr() << "\n";

  mach.SetDA(ash.GetDAddr());
  ///

  SFAMAh.SetPType(SFamaHeader::SFAMA_DATA);
  //SFAMAh->SA = index_;
  //SFAMAh->DA = ash.GetNextHop();

	data_pkt->AddHeader(mach);
	data_pkt->AddHeader(SFAMAh);
  data_pkt->AddHeader(ash);
	return data_pkt;
}


Ptr<Packet>
AquaSimSFama::MakeACK(AquaSimAddress data_sender)
{
	NS_LOG_FUNCTION(this << data_sender.GetAsInt());

  Ptr<Packet> ack_pkt = Create<Packet>();
  AquaSimHeader ash;
  SFamaHeader SFAMAh;
  MacHeader mach;
	AquaSimPtTag ptag;

  ash.SetSize(SFAMAh.GetSize(SFamaHeader::SFAMA_ACK));
  ash.SetTxTime(GetTxTime(ash.GetSize()) );
  ash.SetErrorFlag(false);
  ash.SetDirection(AquaSimHeader::DOWN);
	ptag.SetPacketType(AquaSimPtTag::PT_SFAMA);

  mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  mach.SetDA(data_sender);

  SFAMAh.SetPType(SFamaHeader::SFAMA_ACK);
  //SFAMAh->SA = index_;
  //SFAMAh->DA = data_sender;

	ack_pkt->AddHeader(mach);
  ack_pkt->AddHeader(SFAMAh);
	ack_pkt->AddHeader(ash);
	ack_pkt->AddPacketTag(ptag);
	return ack_pkt;
}


/*process all kinds of packets*/

void
AquaSimSFama::ProcessRTS(Ptr<Packet> rts_pkt)
{
	NS_LOG_FUNCTION(this);
	if(AquaSimSFAMA_DEBUG){
		rts_pkt->Print(std::cout);
	}

	AquaSimHeader ash;
  SFamaHeader SFAMAh;
  MacHeader mach;
	rts_pkt->RemoveHeader(ash);
  rts_pkt->RemoveHeader(SFAMAh);
  rts_pkt->PeekHeader(mach);
	rts_pkt->AddHeader(SFAMAh);
	rts_pkt->AddHeader(ash);

	double time2comingslot = GetTime2ComingSlot(Simulator::Now().ToDouble(Time::S));
//	std::cout << "Time to coming Slot: " << time2comingslot << "\n";

//	std::cout << "STATUS: " << GetStatus() << "\n";

	if( mach.GetDA() == AquaSimAddress::ConvertFrom(m_device->GetAddress()) ) {
		if ( (GetStatus() == IDLE_WAIT ||
			GetStatus() == WAIT_SEND_RTS ||
			GetStatus() == BACKOFF_FAIR )   		) {

				StopTimers();
				SetStatus(WAIT_SEND_CTS);
				//reply a cts
				m_waitSendTimer.m_pkt = MakeCTS(mach.GetSA(), SFAMAh.GetSlotNum());

        m_waitSendTimer.SetFunction(&AquaSimSFama_Wait_Send_Timer::expire,&m_waitSendTimer);
        m_waitSendTimer.Schedule(Seconds(time2comingslot));
		}
	}
	else {
		//do backoff
//		double backoff_time = time2comingslot + 1 /*for cts*/+
//			SFAMAh.GetSlotNum()*m_slotLen /*for data*/+ 1 /*for ack*/;

		double backoff_time = time2comingslot + (SFAMAh.GetSlotNum())*m_slotLen;
//		double backoff_time = time2comingslot + RandBackoffSlots()*m_slotLen;

//		std::cout << "SlotNum: " << SFAMAh.GetSlotNum() << "\n";
//		std::cout << "Current Slot: " << Simulator::Now().ToDouble(Time::S) / m_slotLen << "\n";

		StopTimers();
		SetStatus(BACKOFF);

    m_backoffTimer.SetFunction(&AquaSimSFama_Backoff_Timer::expire,&m_backoffTimer);
    m_backoffTimer.Schedule(Seconds(backoff_time));
	}
}

void
AquaSimSFama::ProcessCTS(Ptr<Packet> cts_pkt)
{
	NS_LOG_FUNCTION(this);
	if(AquaSimSFAMA_DEBUG){
		cts_pkt->Print(std::cout);
	}

	AquaSimHeader ash;
  SFamaHeader SFAMAh;
  MacHeader mach;
	cts_pkt->RemoveHeader(ash);
  cts_pkt->RemoveHeader(SFAMAh);
  cts_pkt->PeekHeader(mach);
	cts_pkt->AddHeader(SFAMAh);
	cts_pkt->AddHeader(ash);

	double time2comingslot = GetTime2ComingSlot(Simulator::Now().ToDouble(Time::S));

//	std::cout << "DESTINATION ADDRESS: " << mach.GetDA() << "\n";
//	std::cout << "STATUS:" << GetStatus() << "\n";


	if( mach.GetDA() == AquaSimAddress::ConvertFrom(m_device->GetAddress()) && GetStatus() == WAIT_RECV_CTS ) {

		//send DATA
		StopTimers();
		SetStatus(WAIT_SEND_DATA);
		//send the packet
		m_waitSendTimer.m_pkt = NULL;
    m_waitSendTimer.SetFunction(&AquaSimSFama_Wait_Send_Timer::expire,&m_waitSendTimer);
    m_waitSendTimer.Schedule(Seconds(time2comingslot));

		double wait_time = (1+SFAMAh.GetSlotNum())*m_slotLen+time2comingslot;
		if( time2comingslot < 0.1 ) {
			wait_time += m_slotLen;
		}

    m_waitReplyTimer.SetFunction(&AquaSimSFama_Wait_Reply_Timer::expire,&m_waitReplyTimer);
    m_waitReplyTimer.Schedule(Seconds(wait_time));
	}
	else {
		//do backoff
		double backoff_time = SFAMAh.GetSlotNum()*m_slotLen /*for data*/+
			1 /*for ack*/+time2comingslot;

		StopTimers();
		SetStatus(BACKOFF);

    m_backoffTimer.SetFunction(&AquaSimSFama_Backoff_Timer::expire,&m_backoffTimer);
    m_backoffTimer.Schedule(Seconds(backoff_time));
	}

}

void
AquaSimSFama::ProcessDATA(Ptr<Packet> data_pkt)
{
	NS_LOG_FUNCTION(this);

	AquaSimHeader ash;
  SFamaHeader SFAMAh;
  MacHeader mach;
	data_pkt->RemoveHeader(ash);
	data_pkt->RemoveHeader(SFAMAh);
  data_pkt->PeekHeader(mach);
	data_pkt->AddHeader(SFAMAh);
	data_pkt->AddHeader(ash);

	if( mach.GetDA() == AquaSimAddress::ConvertFrom(m_device->GetAddress()) && GetStatus() == WAIT_RECV_DATA ) {
	// if( mach.GetDA() == AquaSimAddress::ConvertFrom(m_device->GetAddress()) ) {

		//send ACK
		StopTimers();
		SetStatus(WAIT_SEND_ACK);
		// SetStatus(IDLE_WAIT);

		m_waitSendTimer.m_pkt = MakeACK(mach.GetSA());
    m_waitSendTimer.SetFunction(&AquaSimSFama_Wait_Send_Timer::expire,&m_waitSendTimer);
    m_waitSendTimer.Schedule(Seconds(GetTime2ComingSlot(Simulator::Now().ToDouble(Time::S))));

		/*send packet to upper layer*/
		data_pkt->RemoveHeader(ash);
		data_pkt->RemoveHeader(SFAMAh);
		ash.SetSize(SFAMAh.GetSize(SFamaHeader::SFAMA_DATA));
		data_pkt->AddHeader(SFAMAh);
		data_pkt->AddHeader(ash);

		SendUp(data_pkt->Copy()); /*the original one will be released*/
	}
	else {
		//do backoff
		double backoff_time = 1+GetTime2ComingSlot(Simulator::Now().ToDouble(Time::S)) /*for ack*/;

		StopTimers();
		SetStatus(BACKOFF);

    m_backoffTimer.SetFunction(&AquaSimSFama_Backoff_Timer::expire,&m_backoffTimer);
    m_backoffTimer.Schedule(Seconds(backoff_time));
	}
}


void
AquaSimSFama::ProcessACK(Ptr<Packet> ack_pkt)
{
	AquaSimHeader ash;
	SFamaHeader SFAMAh;
	MacHeader mach;
	ack_pkt->RemoveHeader(ash);
	ack_pkt->RemoveHeader(SFAMAh);
	ack_pkt->PeekHeader(mach);
	ack_pkt->AddHeader(SFAMAh);
	ack_pkt->AddHeader(ash);

  NS_LOG_DEBUG("ProcessACK(before)");
#ifdef AquaSimSFama_DEBUG
  NS_LOG_DEBUG(PrintAllQ());
#endif
  NS_LOG_DEBUG("ProcessACK: Status is " << GetStatus());

	if( mach.GetDA() == AquaSimAddress::ConvertFrom(m_device->GetAddress()) && GetStatus() == WAIT_RECV_ACK ) {
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
	AquaSimAddress recver_addr;
	int pkt_num = 1;

	if( m_sendingPktQ.empty() && m_CachedPktQ.empty() ) {
		return;
	}

	if( !m_sendingPktQ.empty() && GetStatus() == IDLE_WAIT  ) {
			AquaSimHeader ash;
			SFamaHeader SFAMAh;
			MacHeader mach;
			m_sendingPktQ.front()->RemoveHeader(ash);
			m_sendingPktQ.front()->RemoveHeader(SFAMAh);
			m_sendingPktQ.front()->PeekHeader(mach);
			m_sendingPktQ.front()->AddHeader(SFAMAh);
			m_sendingPktQ.front()->AddHeader(ash);

      recver_addr = mach.GetDA();
	} else if( !m_CachedPktQ.empty() && GetStatus() == IDLE_WAIT ) {
    NS_LOG_DEBUG("PrepareSendingDATA(before)");
#ifdef AquaSimSFama_DEBUG
    NS_LOG_DEBUG(PrintAllQ());
#endif
		AquaSimHeader ash_tmp;
		SFamaHeader SFAMAh_tmp;
		MacHeader mach_tmp;

		tmp_pkt = m_CachedPktQ.front();
		tmp_pkt->RemoveHeader(ash_tmp);
		tmp_pkt->RemoveHeader(SFAMAh_tmp);
		tmp_pkt->PeekHeader(mach_tmp);
		tmp_pkt->AddHeader(SFAMAh_tmp);
		tmp_pkt->AddHeader(ash_tmp);
    recver_addr = mach_tmp.GetDA();
		m_CachedPktQ.pop();
		m_sendingPktQ.push(tmp_pkt);
		pkt_num = 1;

		/*get at most m_maxBurst DATA packets with same receiver*/
		while( (pkt_num < m_maxBurst) && (!m_CachedPktQ.empty()) ) {
			tmp_pkt = m_CachedPktQ.front();
			m_CachedPktQ.pop();

			if( recver_addr == mach_tmp.GetDA() ) {
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


//	ScheduleRTS(recver_addr, (additional_txtime/m_slotLen)+1 /*for ceil*/
//	+1/*the basic slot*/ );

	ScheduleRTS(recver_addr, (int)(additional_txtime/m_slotLen) + 1);
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

//    std::cout << "TxTime: " << ash.GetTxTime().ToDouble(Time::S) << "\n";

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
AquaSimSFama::ScheduleRTS(AquaSimAddress recver, int slot_num)
{
	double backoff_time = RandBackoffSlots()*m_slotLen+GetTime2ComingSlot(Simulator::Now().ToDouble(Time::S));
	SetStatus(WAIT_SEND_RTS);
	m_waitSendTimer.m_pkt = MakeRTS(recver, slot_num);
  m_waitSendTimer.SetFunction(&AquaSimSFama_Wait_Send_Timer::expire,&m_waitSendTimer);
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
	NS_LOG_FUNCTION(this);
	if(AquaSimSFAMA_DEBUG){
		pkt->Print(std::cout);
	}

	AquaSimHeader ash;
  SFamaHeader SFAMAh;
  MacHeader mach;
  pkt->RemoveHeader(ash);
  pkt->RemoveHeader(SFAMAh);
  pkt->RemoveHeader(mach);

	ash.SetDirection(AquaSimHeader::DOWN);

	Time txtime=ash.GetTxTime();

	//status_handler.is_ack() = false;
//	std::cout << "PACKET TYPE: " << SFAMAh.GetPType() << "\n";

	if( SFAMAh.GetPType() == SFamaHeader::SFAMA_CTS ) {
    m_slotNumHandler = SFAMAh.GetSlotNum();
	}
	/*else if ( SFAMAh->packet_type == hdr_SFAMA::SFAMA_ACK ) {
		status_handler.is_ack() = true;
	}*/

	switch( m_device->GetTransmissionStatus() ) {
		case SLEEP:
			PowerOn();
		case RECV:
			NS_LOG_INFO("RECV-SEND Collision!!!!!");
			pkt=0;
			//do backoff??
			break;
		case NIDLE:
			//m_device->SetTransmissionStatus(SEND);
			ash.SetTimeStamp(Simulator::Now());
			// pkt->RemoveHeader(SFAMAh);
			// pkt->PeekHeader(mach);
			// pkt->AddHeader(SFAMAh);
			// pkt->AddHeader(ash);

			pkt->AddHeader(SFAMAh);
			pkt->AddHeader(mach);
			pkt->AddHeader(ash);

			NS_LOG_DEBUG(Simulator::Now().GetSeconds() << ": node " << mach.GetSA() <<
					       " send to node " << mach.GetDA() );
			// std::cout << Simulator::Now().GetSeconds() << ": node " << mach.GetSA() << " send to node " << mach.GetDA() << "\n";
			// std::cout << Simulator::Now().GetSeconds() << ": node " << ash.GetSAddr() << " send to node " << ash.GetDAddr() << "\n";
			SendDown(pkt->Copy());

			// StopTimers();
			// ReleaseSentPkts();
			// PrepareSendingDATA();

			// Simulator::Schedule(txtime - Seconds(0.1),&AquaSimSFama::StopTimers,this);
			// Simulator::Schedule(txtime - Seconds(0.1),&AquaSimSFama::ReleaseSentPkts,this);
			// Simulator::Schedule(txtime - Seconds(0.1),&AquaSimSFama::PrepareSendingDATA,this);

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
	//m_device->SetTransmissionStatus(NIDLE);

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
		// SetStatus(IDLE_WAIT);

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
  m_waitReplyTimer.SetFunction(&AquaSimSFama_Wait_Reply_Timer::expire,&m_waitReplyTimer);
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
    m_datasendTimer.SetFunction(&AquaSimSFama_DataSend_Timer::expire,&m_datasendTimer);
    m_datasendTimer.Schedule(Seconds(0.00001));
//    m_datasendTimer.Schedule(Seconds(0.00000001));
	}
	else {
		SendPkt(pkt->Copy());
	}
}

void
AquaSimSFama::WaitReplyTimerProcess(bool directcall)
{
	/*do backoff*/
	double backoff_time = RandBackoffSlots()*m_slotLen + GetTime2ComingSlot(Simulator::Now().ToDouble(Time::S));
	#ifdef AquaSimSFama_DEBUG
	if( !directcall )
    NS_LOG_WARN(Simulator::Now().GetSeconds() << " node " << m_device->GetNode() << "TIME OUT!!!!!");
	#endif  //AquaSimSFama_DEBUG
	if( directcall ) {
		SetStatus(BACKOFF_FAIR);
    m_backoffTimer.SetFunction(&AquaSimSFama_Backoff_Timer::expire,&m_backoffTimer);
    m_backoffTimer.Schedule(Seconds(GetTime2ComingSlot(Simulator::Now().ToDouble(Time::S))));
	} else {
		SetStatus(BACKOFF);
    m_backoffTimer.SetFunction(&AquaSimSFama_Backoff_Timer::expire,&m_backoffTimer);
//    std::cout << "TIMEOUT VALUE: " << Seconds(backoff_time) << "\n";

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

		// StopTimers();
		// SetStatus(WAIT_SEND_DATA);
		// ReleaseSentPkts();
		// PrepareSendingDATA();

    // Simulator::Schedule(Seconds(Seconds(m_dataSendingInterval)+2*txtime),&AquaSimSFama::StopTimers,this);
    // Simulator::Schedule(Seconds(Seconds(m_dataSendingInterval)+2*txtime),&AquaSimSFama::SetStatus,this, IDLE_WAIT);
    // Simulator::Schedule(Seconds(Seconds(m_dataSendingInterval)+2*txtime),&AquaSimSFama::ReleaseSentPkts,this);
    // Simulator::Schedule(Seconds(Seconds(m_dataSendingInterval)+2*txtime),&AquaSimSFama::PrepareSendingDATA,this);

	m_datasendTimer.SetFunction(&AquaSimSFama_DataSend_Timer::expire,&m_datasendTimer);
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
	NS_LOG_FUNCTION(this);
	if (AquaSimSFAMA_DEBUG) {
		pkt->Print(std::cout);
	}

  AquaSimHeader ash;
	SFamaHeader SFAMAh;
	MacHeader mach;
  pkt->RemoveHeader(ash);
  pkt->RemoveHeader(SFAMAh);
  pkt->RemoveHeader(mach);

	ash.SetDirection(AquaSimHeader::DOWN);

	switch( m_device->GetTransmissionStatus() ) {
		case SLEEP:
			PowerOn();
		case RECV:
      NS_LOG_INFO("RECV-SEND Collision!!!!!");
      pkt=0;
			//do backoff??
			break;
		case NIDLE:
			//m_device->SetTransmissionStatus(SEND);
			ash.SetTimeStamp(Simulator::Now());
	// 		pkt->RemoveHeader(SFAMAh);
    //   pkt->PeekHeader(mach);
			// pkt->AddHeader(SFAMAh);
			// pkt->AddHeader(ash);

			pkt->AddHeader(SFAMAh);
			pkt->AddHeader(mach);
			pkt->AddHeader(ash);

      NS_LOG_DEBUG(Simulator::Now().GetSeconds() << ": node " << mach.GetSA() <<
            " send to node " << mach.GetDA() );
			// std::cout << Simulator::Now().GetSeconds() << ": node " << mach.GetSA() << " send to node " << mach.GetDA() << "\n";
			SendDown(pkt->Copy());
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

void AquaSimSFama::DoDispose()
{
	while (!m_sendingPktQ.empty()) {
		m_sendingPktQ.front()=0;
		m_sendingPktQ.pop();
	}
	while (!m_CachedPktQ.empty()) {
		m_CachedPktQ.front()=0;
		m_CachedPktQ.pop();
	}
	while (!m_BackupSendingPktQ.empty()) {
		m_BackupSendingPktQ.front()=0;
		m_BackupSendingPktQ.pop();
	}
	m_rand=0;
	AquaSimMac::DoDispose();
}

#ifdef AquaSimSFama_DEBUG
void
AquaSimSFama::PrintAllQ()
{
  NS_LOG_INFO("Time " << Simulator::Now().GetSeconds() << " node " <<
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
