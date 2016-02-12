/*
 * aqua-sim-mac-aloha.cc
 *
 *  Created on: Feb 11, 2016
 *      Author: robert
 */


#include "aqua-sim-mac-aloha.h"

#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/integer.h"
#include "ns3/double.h"


namespace ns3{

/*===========================AquaSimAlohaAckRetry Timer===========================*/
long AquaSimAlohaAckRetry::m_idGenerator = 0;


//construct function
AquaSimAloha::AquaSimAloha() :
	AquaSimMac(), m_boCounter(0), ALOHA_Status(PASSIVE), m_persistent(1.0),
	m_AckOn(1), m_minBackoff(0.0), m_maxBackoff(1.5), m_maxACKRetryInterval(0.05),
	m_blocked(false)
{
  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
}

AquaSimAloha::~AquaSimAloha()
{
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
	MakeDoubleChecker<AquaSimAloha>())
      .AddAttribute("AckOn", "If acknowledgement is on",
	IntegerValue(1),
	MakeIntegerAccessor (&AquaSimAloha::m_AckOn),
	MakeIntegerChecker<AquaSimAloha>())
      .AddAttribute("MinBackoff", "Minimum back off time",
	DoubleValue(0.0),
	MakeDoubleAccessor (&AquaSimAloha::m_minBackoff),
	MakeDoubleChecker<AquaSimAloha>())
      .AddAttribute("MaxBackoff", "Maximum back off time",
	DoubleValue(1.5),
	MakeDoubleAccessor (&AquaSimAloha::m_maxBackoff),
	MakeDoubleChecker<AquaSimAloha>())
      .AddAttribute("WaitAckTime", "Acknowledgement wait time (seconds)",
	DoubleValue(0.03),
	MakeDoubleAccessor (&AquaSimAloha::m_waitACKTime),
	MakeDoubleChecker<AquaSimAloha>())
    ;
  return tid;
}


void AquaSimAloha::DoBackoff()
{
  NS_LOG_FUNCTION(this);
  Time BackoffTime=m_rand(m_minBackoff,m_maxBackoff);
  m_boCounter++;
  if (m_boCounter < MAXIMUMCOUNTER)
    {
      ALOHA_Status = BACKOFF;
      Simulator::Schedule(BackoffTime, &AquaSimAloha::SendDataPkt, this);
    }
  else
    {
      m_boCounter=0;
      NS_LOG_INFO("Backoffhandler: too many backoffs");
      PktQ_.front()=0;
      PktQ_.pop();
      ProcessPassive();
  }
}

void AquaSimAloha::ProcessPassive()
{
  if (ALOHA_Status == PASSIVE && !m_blocked) {
    if (!PktQ_.empty() )
	SendDataPkt();
  }
}

void AquaSimAloha::StatusProcess(bool isAck)
{
  m_device->SetTransmissionStatus(NIDLE);

  if( m_blocked ) {
    m_blocked = false;
    ProcessPassive();
    return;
  }

  if( !m_AckOn ) {
    /*Must be DATA*/
    ALOHA_Status = PASSIVE;
    ProcessPassive();
  }
  else if (m_AckOn && !isAck ) {
    ALOHA_Status = WAIT_ACK;
  }
}


/*===========================Send and Receive===========================*/

void AquaSimAloha::TxProcess(Ptr<Packet> pkt)
{
  //callback to higher level, should be implemented differently
  //Scheduler::instance().schedule(&CallBack_handler, &m_callbackEvent, CALLBACK_DELAY);
  NS_LOG_FUNCTION(this << pkt);
  AquaSimHeader asHeader;
  AlohaHeader alohaH;
  pkt->RemoveHeader(asHeader);
  pkt->RemoveHeader(alohaH);

  //size() += alohaH.size();
  asHeader.SetTxTime(GetTxTime(asHeader.GetSerializedSize() + alohaH.GetSerializedSize()));
  asHeader.SetErrorFlag(false);
  asHeader.SetDirection(AquaSimHeader::DOWN);

  Time time;
  if( Simulator::Now().GetDouble() > 500 )	//why?
    time = Seconds(Simulator::Now());
  alohaH.packet_type = AlohaHeader::DATA;
  alohaH.SA = m_device->GetAddress();

  if(asHeader.GetNextHop() == (0xffffffff))  //IP_BROADCAST
    {
      alohaH.SetDA(Address(0xffffffff));	//MAC_BROADCAST
    }
  else {
      alohaH.SetDA(asHeader.GetNextHop());
    }

  pkt->AddHeader(asHeader);
  pkt->AddHeader(alohaH);

  PktQ_.push(pkt);//push packet to the queue

  //fill the next hop when sending out the packet;
  if(ALOHA_Status == PASSIVE && PktQ_.size() == 1 && !m_blocked )
  {
    SendDataPkt();
  }
}


void AquaSimAloha::SendDataPkt()
{
  double P = m_rand(0,1);
  Ptr<Packet> tmp = PktQ_.front();
  AquaSimHeader asHeader;
  tmp->PeekHeader(asHeader);
  Address recver = asHeader.GetNextHop();

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
  AquaSimHeader asHeader;
  AlohaHeader alohaH;
  pkt->RemoveHeader(asHeader);
  pkt->PeekHeader(alohaH);

  asHeader.SetDirection(AquaSimHeader::DOWN);

  Time txtime = asHeader.GetTxTime();

  switch( m_device->TransmissionStatus() ) {
    case SLEEP:
      PowerOn();

    case NIDLE:
      m_device->SetTransmissionStatus(SEND);
      asHeader.SetTimeStamp(Seconds(Simulator::Now())); //why?
      asHeader.SetDirection(AquaSimHeader::DOWN);	//already set...

      //ACK doesn't affect the status, only process DATA here
      if (alohaH.packet_type == AlohaHeader::DATA) {
	//must be a DATA packet, so setup wait ack timer
	if ((alohaH.GetDA() != (0xffffffff)) && m_AckOn) {	//MAC_BROADCAST
	  ALOHA_Status = WAIT_ACK;
	  m_waitACKTimer = Simulator::Schedule((Seconds(m_waitACKTime)+txtime),&AquaSimAloha::DoBackoff, this);
	}
	else {
	  PktQ_.front()=0;
	  PktQ_.pop();
	  ALOHA_Status = PASSIVE;
	}
	m_isAck = false;
      }
      else{
	m_isAck = true;
      }
      pkt->AddHeader(asHeader);
      SendDown(pkt);

      m_blocked = true;
      Simulator::Schedule((txtime + Seconds(0.01)), &AquaSimAloha::StatusProcess, this, m_isAck);
      break;

    case RECV:
      NS_LOG_INFO("SendPkt: RECV-SEND collision!!!");
      if( alohaH.packet_type == AlohaHeader::ACK) {
	pkt->AddHeader(asHeader);
	RetryACK(pkt);
      }
      else
	pkt=0;

      ALOHA_Status = PASSIVE;
      break;

    default:
    //status is SEND
      NS_LOG_INFO("SendPkt: node " << m_device->GetNode() << " send data too fast");
      if( alohaH.packet_type == AlohaHeader::ACK ) {
	pkt->AddHeader(asHeader);
	RetryACK(pkt);
      }
      else
	pkt=0;
      ALOHA_Status = PASSIVE;
  }
}

void AquaSimAloha::RecvProcess(Ptr<Packet> pkt)
{
  AquaSimHeader asHeader;
  AlohaHeader alohaH;
  pkt->PeekHeader(asHeader);
  pkt->PeekHeader(alohaH);

  Address recver = alohaH.GetDA();

  if( asHeader.GetErrorFlag() )
  {
    if(/*drop_ && */recver==m_device->GetAddress()) {
	NS_LOG_INFO("Packet:" << pkt << " error/collision on node " << m_device->GetNode());
	//drop_->recv(pkt,"Error/Collision");
    }
    else
      pkt=0;

    //ProcessPassive();
    return;
  }

  if( alohaH.packet_type == AlohaHeader::ACK ) {
    //if get ACK after WaitACKTimer, ignore ACK
    if( recver == m_device->GetAddress() && ALOHA_Status == WAIT_ACK) {
	m_waitACKTimer.Cancel();
	m_boCounter=0;
	PktQ_.front()=0;
	PktQ_.pop();
	ALOHA_Status=PASSIVE;
	ProcessPassive();
    }
  }
  else if(alohaH.packet_type == AlohaHeader::DATA) {
    //process Data packet
    if( recver == m_device->GetAddress() || recver == Address(0xffffffff) ) {	//MAC_BROADCAST
	//size() -= alohaH.size();
	SendUp(pkt->Copy());
	if ( m_AckOn && (recver != Address(0xffffffff)))	//MAC_BROADCAST
	    ReplyACK(pkt->Copy());
	else
	    ProcessPassive();
    }

  }
  pkt=0;
}

void AquaSimAloha::ReplyACK(Ptr<Packet> pkt)//sendACK
{
  AlohaHeader alohaH;
  pkt->PeekHeader(alohaH);
  Address Data_Sender = alohaH.GetSA();

  SendPkt(MakeACK(Data_Sender));
  m_boCounter=0;
  pkt=0;
}

Ptr<Packet> AquaSimAloha::MakeACK(Address Data_Sender)
{
  Ptr<Packet> pkt = Ptr<Packet>();
  AquaSimHeader asHeader;
  AlohaHeader alohaH;

  //size() += alohaH.size();
  asHeader.SetTxTime(GetTxTime(asHeader.GetSerializedSize() + alohaH.GetSerializedSize()));
  asHeader.SetErrorFlag(false);
  asHeader.SetDirection(AquaSimHeader::DOWN);
  asHeader.SetNextHop(Data_Sender);
  //ptype = PT_ALOHA;

  alohaH.packet_type = AlohaHeader::ACK;
  alohaH.SetSA(m_device->GetAddress());
  alohaH.SetDA(Data_Sender);

  pkt->AddHeader(asHeader);
  pkt->AddHeader(alohaH);

  return pkt;
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


} // namespace ns3
