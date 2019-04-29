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


#include "aqua-sim-mac-aloha.h"
#include "aqua-sim-pt-tag.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-mac.h"

#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/integer.h"
#include "ns3/double.h"


namespace ns3{

NS_LOG_COMPONENT_DEFINE("AquaSimAloha");
NS_OBJECT_ENSURE_REGISTERED(AquaSimAloha);

/*===========================AquaSimAlohaAckRetry Timer===========================*/
long AquaSimAlohaAckRetry::m_idGenerator = 0;


//construct function
AquaSimAloha::AquaSimAloha() :
	AquaSimMac(), m_boCounter(0), ALOHA_Status(PASSIVE), m_persistent(1.0),
	m_AckOn(1), m_minBackoff(0.0), m_maxBackoff(1.5), m_maxACKRetryInterval(0.05),
	m_blocked(false)
{
	m_rand = CreateObject<UniformRandomVariable> ();
    Simulator::Schedule(Seconds(0), &AquaSimAloha::Init, this);
}

AquaSimAloha::~AquaSimAloha()
{
}

void AquaSimAloha::Init()
{
    m_maxPropDelay = m_maxTransmitDistance / Device()->GetPropSpeed();
}


TypeId
AquaSimAloha::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimAloha")
      .SetParent<AquaSimMac>()
      .AddConstructor<AquaSimAloha>()
      .AddAttribute("Persistent", "Persistence of sending data packets",
	DoubleValue(1.0),
	MakeDoubleAccessor (&AquaSimAloha::m_persistent),
	MakeDoubleChecker<double>())
      .AddAttribute("AckOn", "If acknowledgement is on",
	IntegerValue(1),
	MakeIntegerAccessor (&AquaSimAloha::m_AckOn),
	MakeIntegerChecker<int>())
      .AddAttribute("MinBackoff", "Minimum back off time",
	DoubleValue(0.0),
	MakeDoubleAccessor (&AquaSimAloha::m_minBackoff),
	MakeDoubleChecker<double>())
      .AddAttribute("MaxBackoff", "Maximum back off time",
	DoubleValue(1.5),
	MakeDoubleAccessor (&AquaSimAloha::m_maxBackoff),
	MakeDoubleChecker<double>())
      .AddAttribute("MaxTransmitDistance", "The maximum transmission distance in meters",
    DoubleValue(3000.0),
    MakeDoubleAccessor (&AquaSimAloha::m_maxTransmitDistance),
    MakeDoubleChecker<double>())
    ;
  return tid;
}

int64_t
AquaSimAloha::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_rand->SetStream(stream);
  return 1;
}

void AquaSimAloha::DoBackoff()
{
  //NS_LOG_FUNCTION(this);
  Time BackoffTime=Seconds(m_rand->GetValue(m_minBackoff,m_maxBackoff));
  m_boCounter++;
  if (m_boCounter < MAXIMUMCOUNTER)
    {
      ALOHA_Status = BACKOFF;
			NS_LOG_INFO("DoBackoff: " << BackoffTime.ToDouble(Time::S));
      Simulator::Schedule(BackoffTime, &AquaSimAloha::SendDataPkt, this);
    }
  else
    {
      m_boCounter=0;
      NS_LOG_INFO("Backoffhandler: too many backoffs");
			if (!PktQ_.empty()) {
      	PktQ_.front()=0;
      	PktQ_.pop();
        m_txPacketDrops += 1;
      ProcessPassive();
		}
  }
}

void AquaSimAloha::ProcessPassive()
{
  if (ALOHA_Status == PASSIVE && !m_blocked) {
    if (!PktQ_.empty() ) {
			SendDataPkt();
		}
	}
}

void AquaSimAloha::StatusProcess(bool isAck)
{
  //m_device->SetTransmissionStatus(NIDLE);

  if( m_blocked ) {
    m_blocked = false;
    ProcessPassive();
    return;
  }

  if( !m_AckOn ) {
    /*Must be DATA*/
    ALOHA_Status = PASSIVE;
    NS_LOG_INFO("Status set to: PASSIVE");
    ProcessPassive();
  }
  else if (m_AckOn && !isAck ) {
    ALOHA_Status = WAIT_ACK;
    NS_LOG_INFO("Status set to: WAIT ACK");
  }
}


/*===========================Send and Receive===========================*/

bool AquaSimAloha::TxProcess(Ptr<Packet> pkt)
{
  //callback to higher level, should be implemented differently
  //Scheduler::instance().schedule(&CallBack_handler, &m_callbackEvent, CALLBACK_DELAY);
  NS_LOG_FUNCTION(m_device->GetAddress() << pkt << Simulator::Now().GetSeconds());
  AquaSimHeader asHeader;
  AlohaHeader alohaH;
  pkt->RemoveHeader(asHeader);
  //pkt->RemoveHeader(alohaH);	This may need to be fixed for multi hop.
        asHeader.SetSize(alohaH.GetSize()+asHeader.GetSize());
  asHeader.SetTxTime(GetTxTime(asHeader.GetSize()));
  asHeader.SetErrorFlag(false);
  asHeader.SetDirection(AquaSimHeader::DOWN);
  NS_LOG_INFO("Transmission time: "<< asHeader.GetTxTime ().GetSeconds () << " seconds");

  alohaH.SetPType(AlohaHeader::DATA);
	alohaH.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );

  if(asHeader.GetNextHop() == AquaSimAddress::GetBroadcast() )
    {
			alohaH.SetDA(asHeader.GetDAddr());
    }
  else {
      alohaH.SetDA(asHeader.GetNextHop());
    }

	pkt->AddHeader(alohaH);
	pkt->AddHeader(asHeader);

  PktQ_.push(pkt);//push packet to the queue

  //fill the next hop when sending out the packet;
  if(ALOHA_Status == PASSIVE && PktQ_.size() >= 1 && !m_blocked )
  {
    SendDataPkt();
  }
  return true;
}


void AquaSimAloha::SendDataPkt()
{
	NS_LOG_FUNCTION(this);
  double P = m_rand->GetValue(0,1);
  Ptr<Packet> tmp = PktQ_.front();
  AquaSimHeader asHeader;
	tmp->PeekHeader(asHeader);
  AquaSimAddress recver = asHeader.GetNextHop();

  ALOHA_Status = SEND_DATA;

  if( P<=m_persistent ) {
    if( asHeader.GetNextHop() == recver ) //why? {
	SendPkt(tmp->Copy());
  }
  else {
    //Binary Exponential Backoff
    m_boCounter--;
    DoBackoff();
  }

  return;
}


void AquaSimAloha::SendPkt(Ptr<Packet> pkt)
{
	NS_LOG_FUNCTION(this);
  AquaSimHeader asHeader;
  AlohaHeader alohaH;
  pkt->RemoveHeader(asHeader);
  pkt->PeekHeader(alohaH);

  asHeader.SetDirection(AquaSimHeader::DOWN);
  asHeader.SetTimeStamp(Simulator::Now()); //why?

  //compute estimated RTT
  Time txtime = asHeader.GetTxTime();
  NS_LOG_DEBUG("pkt txTime: " << txtime.GetSeconds() << "("<<asHeader.GetSize()<<")");
  NS_LOG_DEBUG("AlohaHeader txTime: " << GetTxTime(alohaH.GetSize()) << " (" << alohaH.GetSize() << ")");
  NS_LOG_DEBUG("PropDelay: " << m_maxPropDelay);
  Time ertt = txtime + GetTxTime(alohaH.GetSize()) + GetTxTime(alohaH.GetSize()) + Seconds(m_maxPropDelay*2);

  switch( m_device->GetTransmissionStatus() ) {
    case SLEEP:
      PowerOn();

    case NIDLE: {
      //ACK doesn't affect the status, only process DATA here
      if (alohaH.GetPType() == AlohaHeader::DATA) {
				//must be a DATA packet, so setup wait ack timer
				if ((alohaH.GetDA() != AquaSimAddress::GetBroadcast()) && m_AckOn) {
				  NS_LOG_DEBUG("Set status to WAIT_ACK");
				  ALOHA_Status = WAIT_ACK;
				  m_waitACKTimer = Simulator::Schedule(ertt,&AquaSimAloha::DoBackoff, this);
				  NS_LOG_DEBUG("estimated RTT: " << ertt.GetSeconds () << "seconds");
				  NS_LOG_DEBUG("launch waitACKTimer");
				}
				else {
				if (!PktQ_.empty()) {
			  	PktQ_.front()=0;
			  	PktQ_.pop();
				}
			  ALOHA_Status = PASSIVE;
			}
			m_isAck = false;
      }
      else{
        NS_LOG_DEBUG("send ACK (" << asHeader.GetSize() << " bytes : " << asHeader.GetTxTime().GetSeconds() << " seconds)");
	m_isAck = true;
      }
			MacHeader mach;
			mach.SetDA(AquaSimAddress::GetBroadcast());
			mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
			pkt->AddHeader(mach);
      pkt->AddHeader(asHeader);
      SendDown(pkt);

      m_blocked = true;
      Simulator::Schedule(txtime + Seconds(0.01), &AquaSimAloha::StatusProcess, this, m_isAck);
      break;
		}
    case RECV:
      NS_LOG_INFO("SendPkt: RECV-SEND collision!!!");
      if( alohaH.GetPType() == AlohaHeader::ACK) {
        pkt->AddHeader(asHeader);
        RetryACK(pkt);
        ALOHA_Status = PASSIVE;
      }
      else
      {
          DoBackoff();
          pkt=0;
      }
      break;

    default:
    //status is SEND
      NS_LOG_INFO("SendPkt: node " << m_device->GetNode() << " send data too fast");
      if( alohaH.GetPType() == AlohaHeader::ACK ) {
	pkt->AddHeader(asHeader);
	RetryACK(pkt);
    ALOHA_Status = PASSIVE;
      }
      else
          {
              DoBackoff();
              pkt=0;
          }
  }
}

bool AquaSimAloha::RecvProcess(Ptr<Packet> pkt)
{
	NS_LOG_FUNCTION(m_device->GetAddress());
  AquaSimHeader asHeader;
  AlohaHeader alohaH;
	MacHeader mach;
  pkt->RemoveHeader(asHeader);
	pkt->RemoveHeader(mach);	//not used, leave off.
  pkt->PeekHeader(alohaH);
	pkt->AddHeader(asHeader);

  AquaSimAddress recver = alohaH.GetDA();
	AquaSimAddress myAddr = AquaSimAddress::ConvertFrom(m_device->GetAddress());

  if( asHeader.GetErrorFlag() )
  {
    if(/*drop_ && */recver==m_device->GetAddress()) {
	NS_LOG_INFO("Packet:" << pkt << " error/collision on node " << m_device->GetNode());
	//drop_->recv(pkt,"Error/Collision");
    }
    else
      pkt=0;

    //ProcessPassive();
    return false;
  }

  if( alohaH.GetPType() == AlohaHeader::ACK ) {
    //if get ACK after WaitACKTimer, ignore ACK
    NS_LOG_DEBUG("Received ACK");
    if( recver == myAddr && ALOHA_Status == WAIT_ACK) {
	m_waitACKTimer.Cancel();
	m_boCounter=0;
	if (!PktQ_.empty()) {
		PktQ_.front()=0;
		PktQ_.pop();
	}
	NS_LOG_DEBUG("Status set to PASSIVE after ACK reception");
	ALOHA_Status=PASSIVE;
	ProcessPassive();
    }
    else
      NS_LOG_INFO("ACK ignored: received after WaitACKTimer");
  }
  else if(alohaH.GetPType() == AlohaHeader::DATA) {
    //process Data packet
    if( recver == myAddr || recver == AquaSimAddress::GetBroadcast() ) {
                        pkt->RemoveHeader(asHeader);
                        auto cpkt = pkt->Copy();
                        pkt->AddHeader(asHeader);
                        cpkt->RemoveHeader(alohaH);
                        asHeader.SetSize(asHeader.GetSize() - alohaH.GetSize());
                        cpkt->AddHeader(asHeader);
                        SendUp(cpkt);

			if ( m_AckOn && (recver != AquaSimAddress::GetBroadcast()))
	    	ReplyACK(pkt->Copy());
				else
	    	ProcessPassive();
    	}
  }
  pkt=0;
  return true;
}

void AquaSimAloha::ReplyACK(Ptr<Packet> pkt) //sendACK
{
	NS_LOG_FUNCTION(this);
	//wouldn't it make more sense to just include aloha header SA instead of pkt for parameters?
  AlohaHeader alohaH;
	AquaSimHeader asHeader;
	pkt->RemoveHeader(asHeader);
	pkt->PeekHeader(alohaH);
	pkt->AddHeader(asHeader);
  AquaSimAddress Data_Sender = alohaH.GetSA();

  SendPkt(MakeACK(Data_Sender));
  m_boCounter=0;
  pkt=0;
}

Ptr<Packet> AquaSimAloha::MakeACK(AquaSimAddress Data_Sender)
{
	NS_LOG_FUNCTION(this);
  Ptr<Packet> pkt = Create<Packet>();
  AquaSimHeader asHeader;
  AlohaHeader alohaH;
	AquaSimPtTag ptag;

        asHeader.SetSize(alohaH.GetSize());
  asHeader.SetTxTime(GetTxTime(alohaH.GetSize()));
  asHeader.SetErrorFlag(false);
  asHeader.SetDirection(AquaSimHeader::DOWN);
  asHeader.SetNextHop(Data_Sender);
	ptag.SetPacketType(AquaSimPtTag::PT_UWALOHA);

  alohaH.SetPType(AlohaHeader::ACK);
  alohaH.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  alohaH.SetDA(Data_Sender);

	pkt->AddHeader(alohaH);
  pkt->AddHeader(asHeader);
	pkt->AddPacketTag(ptag);
  return pkt;
}

AquaSimAlohaAckRetry::~AquaSimAlohaAckRetry()
{
	m_mac=0;
	m_pkt=0;
}

void AquaSimAloha::ProcessRetryTimer(AquaSimAlohaAckRetry* timer)
{
  Ptr<Packet> pkt = timer->Pkt();
  if( RetryTimerMap_.count(timer->Id()) != 0 ) {
      RetryTimerMap_.erase(timer->Id());
  } else {
      NS_LOG_INFO("ProcessRetryTimer: error: cannot find the ack_retry timer");
  }
	delete timer;
  SendPkt(pkt);
}

void AquaSimAloha::RetryACK(Ptr<Packet> ack)
{
  NS_LOG_FUNCTION(this);

  AquaSimAlohaAckRetry* timer = new AquaSimAlohaAckRetry(this, ack);
  Simulator::Schedule(Seconds(m_maxACKRetryInterval*m_rand->GetValue()), &AquaSimAloha::ProcessRetryTimer, this, timer);
  RetryTimerMap_[timer->Id()] = timer;
}

void AquaSimAloha::DoDispose()
{
	NS_LOG_FUNCTION(this);
	while(!PktQ_.empty()) {
		PktQ_.front()=0;
		PktQ_.pop();
	}
  for (std::map<long,AquaSimAlohaAckRetry*>::iterator it=RetryTimerMap_.begin(); it!=RetryTimerMap_.end(); ++it) {
		delete it->second;
		it->second=0;
	}
	RetryTimerMap_.clear();
	m_rand=0;
	AquaSimMac::DoDispose();
}

} // namespace ns3
