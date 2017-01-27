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

#include "aqua-sim-tmac.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-mac.h"
#include "aqua-sim-pt-tag.h"
//#include "vbf/vectorbasedforward.h"

#include "ns3/nstime.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimTMac");
NS_OBJECT_ENSURE_REGISTERED(AquaSimTMac);


void
AquaSimTMac::TBackoffHandler(Ptr<Packet> pkt)
{
  m_tBackoffCounter++;
  if(m_tBackoffCounter<MAXIMUMBACKOFF)
    TxND(pkt, m_tBackoffWindow);
  else
    {
      m_tBackoffCounter=0;  //clear
      NS_LOG_WARN("TBackoffHandler: too many backoffs.");
      pkt=0;
    }
}


void
AquaSimTMac::TStatusHandler()
{
  NS_LOG_FUNCTION(this);
  if(m_localStatus == SLEEP)
    PowerOff();
}

void
AquaSimTMac::TStatusHandler_SetStatus(TransStatus status)
{
  m_localStatus=status;
}


void
AquaSimTMac::TNDHandler()
{
  m_cycleStartTime=Simulator::Now().ToDouble(Time::S);
  SendND(m_shortPacketSize);
}


void
AquaSimTMac::RTSTimeoutHandler()
{
  NS_LOG_INFO("RTSTimeoutHandler: node " << m_device->GetNode() <<
    " timeout " << m_rtsTimeoutNum << " times");
  m_rtsTimeoutNum++;
  if(m_rtsTimeoutNum<2)
  SendRTS();
  else
    {
      m_rtsTimeoutNum=0;
      ProcessSleep();
    }
}


/*
RTSSilenceHandler::RTSSilenceHandler(AquaSimTMac* p):mac_(p){start_time=0;duration=0;}

void RTSSilenceHandler::handle(Event*e)
{
  mac_->ProcessRTSSilence();
}
*/


void
AquaSimTMac::CTSHandler(Ptr<Packet> pkt)
{
  m_ctsNum++;
  TxCTS(pkt);
}



/* ======================================================================
    AquaSimTMac for Aqua Sim.
    Implementation of AquaSimTMac in underwater scenarios

   ====================================================================== */


AquaSimTMac::AquaSimTMac()
{
  m_numSend=0;
  m_numData=0;
  m_largePacketSize=30;
  m_shortPacketSize=10;

  m_shortLatencyTableIndex=0;

  InitializeSilenceTable();

  m_rtsTimeoutNum=0;
  m_tBackoffWindow=0.0;
  m_tBackoffCounter=0;
  m_localStatus = SLEEP;
  m_ctsNum=0;

  m_periodTableIndex=0;

  m_nextPeriod=0;

  m_lastSilenceTime=0;
  m_lastRtsSilenceTime=0;


 for(int i=0;i<T_TABLE_SIZE;i++){


    m_shortLatencyTable[i].node_addr=AquaSimAddress();
    m_shortLatencyTable[i].num=0;
    m_shortLatencyTable[i].last_update_time=0.0;

    m_periodTable[i].node_addr=AquaSimAddress();
    m_periodTable[i].difference=0.0;
    m_periodTable[i].last_update_time=0.0;
  }

    m_arrivalTableIndex=0;
  for(int i=0;i<T_TABLE_SIZE;i++)
    m_arrivalTable[i].node_addr=AquaSimAddress();


   m_maxShortPacketTransmissionTime=((1.0*m_shortPacketSize)/m_bitRate)*(1+m_transmissionTimeError);
   m_maxLargePacketTransmissionTime=((1.0*m_largePacketSize)/m_bitRate)*(1+m_transmissionTimeError);

   m_minBackoffWindow=m_maxLargePacketTransmissionTime;



   m_maxPropagationTime=m_transmissionRange/1500+m_maxShortPacketTransmissionTime;
   m_taDuration=(m_contentionWindow+m_maxShortPacketTransmissionTime
	      +m_maxPropagationTime*2)*1.5;
  InitPhaseOne(m_ndWindow,m_ackNdWindow, m_phaseOneWindow);

}

AquaSimTMac::~AquaSimTMac()
{
}

TypeId
AquaSimTMac::GetTypeId()
{
  static TypeId tid = TypeId("ns3::AquaSimTMac")
    .SetParent<AquaSimMac>()
    .AddConstructor<AquaSimTMac>()
    .AddAttribute ("NDWindow", "Window to send ND",
      DoubleValue(1),
      MakeDoubleAccessor(&AquaSimTMac::m_ndWindow),
      MakeDoubleChecker<double>())
    .AddAttribute ("ACKNDWindow", "Window to send ACK_ND",
      DoubleValue(1),
      MakeDoubleAccessor(&AquaSimTMac::m_ackNdWindow),
      MakeDoubleChecker<double>())
    .AddAttribute ("PhaseOneWindow", "Time for latency detection",
      DoubleValue(3),
      MakeDoubleAccessor(&AquaSimTMac::m_phaseOneWindow),
      MakeDoubleChecker<double>())
    .AddAttribute ("PhaseTwoWindow", "Time for SYN announcement",
      DoubleValue(1),
      MakeDoubleAccessor(&AquaSimTMac::m_phaseTwoWindow),
      MakeDoubleChecker<double>())
    .AddAttribute ("PhaseTwoInterval", "Interval between windows of phase two",
      DoubleValue(0.5),
      MakeDoubleAccessor(&AquaSimTMac::m_phaseTwoInterval),
      MakeDoubleChecker<double>())
    .AddAttribute ("IntervalPhase2Phase3", "Interval between windows of phase 2 and 3",
      DoubleValue(1),
      MakeDoubleAccessor(&AquaSimTMac::m_intervalPhase2Phase3),
      MakeDoubleChecker<double>())
    .AddAttribute ("Duration", "Duration of duty cycle",
      DoubleValue(0.1),
      MakeDoubleAccessor(&AquaSimTMac::m_duration),
      MakeDoubleChecker<double>())
    .AddAttribute ("PhyOverhead", "The overhead caused by Phy layer.",
      IntegerValue(8),
      MakeIntegerAccessor(&AquaSimTMac::m_phyOverhead),
      MakeIntegerChecker<int>())
    .AddAttribute ("LargePacketSize", "Size of a large packet (bits)",
      IntegerValue(480),
      MakeIntegerAccessor(&AquaSimTMac::m_largePacketSize),
      MakeIntegerChecker<int>())
    .AddAttribute ("ShortPacketSize", "Size of a short packet (bits)",
      IntegerValue(40),
      MakeIntegerAccessor(&AquaSimTMac::m_shortPacketSize),
      MakeIntegerChecker<int>())
    .AddAttribute ("PhaseOneCycle", "Number of cycles in phase one",
      IntegerValue(4),
      MakeIntegerAccessor(&AquaSimTMac::m_phaseOneCycle),
      MakeIntegerChecker<int>())
    .AddAttribute ("PhaseTwoCycle", "Number of cycles in phase two",
      IntegerValue(2),
      MakeIntegerAccessor(&AquaSimTMac::m_phaseTwoCycle),
      MakeIntegerChecker<int>())
    .AddAttribute ("TransmissionTimeError", "Guardian Time",
      DoubleValue(0.0001),
      MakeDoubleAccessor(&AquaSimTMac::m_transmissionTimeError),
      MakeDoubleChecker<double>())
    .AddAttribute ("PeriodInterval", "Size of the period interval",
      DoubleValue(1),
      MakeDoubleAccessor(&AquaSimTMac::m_periodInterval),
      MakeDoubleChecker<double>())
    .AddAttribute ("SIF", "Interval between two successive data packets",
      DoubleValue(0.0001),
      MakeDoubleAccessor(&AquaSimTMac::m_sif),
      MakeDoubleChecker<double>())
    .AddAttribute ("ContentionWindow", "Size of the contention window",
      DoubleValue(0.1),
      MakeDoubleAccessor(&AquaSimTMac::m_contentionWindow),
      MakeDoubleChecker<double>())
    .AddAttribute ("TransmissionRange", "Transmission range of all nodes. Default is 3000",
      DoubleValue(3000.0),
      MakeDoubleAccessor(&AquaSimTMac::m_transmissionRange),
      MakeDoubleChecker<double>())
  ;
  return tid;
}

void
AquaSimTMac::InitPhaseOne(double t1,double t2, double t3)
{
  NS_LOG_INFO(this << m_phaseOneCycle);

  if(m_phaseOneCycle)
  {
    m_phaseStatus=PHASEONE;
    InitND(t1,t2,t3);
    m_phaseoneEvent = Simulator::Schedule(Seconds(t3),&AquaSimTMac::InitPhaseOne,
                          this,m_ndWindow,m_ackNdWindow,m_phaseOneWindow);
    m_phaseOneCycle--;
    return;
  }


  InitPhaseTwo();
  return;
}

/*
void
AquaSimTMac::InitPhaseTwo(){

   double delay=m_rand->GetValue()*m_phaseTwoWindow;
   m_phaseStatus=PHASETWO;

    m_cycleStartTime=Simulator::Now();
    //  m_nextPeriod=m_intervalPhase2Phase3+m_phaseTwoWindow+delay;
    m_nextPeriod=m_intervalPhase2Phase3+m_phaseTwoWindow;


    m_phasetwoEvent = Simulator::Schedule(Seconds(delay),&AquaSimTMac::SendSYN,this);
    m_phasethreeEvent = Simulator::Schedule(Seconds(m_nextPeriod),&AquaSimTMac::InitPhaseThree,this);
    printf("AquaSimTMac initphasetwo: the phasethree of node %d is scheduled at %f\n",index_,Simulator::Now()+m_nextPeriod);

    return;
}
*/


void
AquaSimTMac::InitPhaseTwo()
{

  //   double delay=m_rand->GetValue()*m_phaseTwoWindow;
  //  m_nextPeriod=m_intervalPhase2Phase3+m_phaseTwoCycle*m_phaseTwoWindow+delay;
  m_nextPeriod=m_intervalPhase2Phase3+m_phaseTwoCycle*m_phaseTwoWindow;

  m_phasethreeEvent = Simulator::Schedule(Seconds(m_nextPeriod),&AquaSimTMac::InitPhaseThree,this);

  StartPhaseTwo();
  return;
}

void
AquaSimTMac::StartPhaseTwo()
{
  NS_LOG_FUNCTION(this);
  //NS_LOG_DEBUG("StartPhaseTwo:" << m_device->GetAddress() << " phasetwoCycle: " << m_phaseTwoCycle);
  if(m_phaseTwoCycle)
  {
    m_phaseStatus=PHASETWO;
    m_cycleStartTime=Simulator::Now().ToDouble(Time::S);
    Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
    double  delay=m_rand->GetValue()*m_phaseTwoWindow;
    Ptr<Packet> pkt=GenerateSYN();
    Simulator::Schedule(Seconds(delay),&AquaSimTMac::TxND,this,pkt,m_phaseTwoWindow);
    m_phasetwoEvent = Simulator::Schedule(Seconds(m_phaseTwoWindow+m_phaseTwoInterval),
                                    &AquaSimTMac::SendSYN,this);
    m_nextPeriod-=m_phaseTwoWindow-m_phaseTwoInterval;
    m_phaseTwoCycle--;
  }
  return;
}



Ptr<Packet>
AquaSimTMac::GenerateSYN()
{
  Ptr<Packet> pkt =Create<Packet>();
  TMacHeader synh;
  AquaSimHeader ash;
  AquaSimPtTag ptag;

  ash.SetSize(m_shortPacketSize);
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetDirection(AquaSimHeader::DOWN);
  //ash.addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_TMAC);

  synh.SetPtype(PT_SYN);
  synh.SetPktNum(m_numSend);
  synh.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  synh.SetDuration(m_duration);
  m_numSend++;

  pkt->AddHeader(synh);
  pkt->AddHeader(ash);
  pkt->AddPacketTag(ptag);
  NS_LOG_INFO("GenerateSYN: node(" << synh.GetSenderAddr() <<
                ") generates SYN packet at " << Simulator::Now().GetSeconds());
	return pkt;
}


void
AquaSimTMac::SendSYN()
{
  Ptr<Packet> pkt =Create<Packet>();
  TMacHeader synh;
  AquaSimHeader ash;
  AquaSimPtTag ptag;

  ash.SetSize(m_shortPacketSize);
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetDirection(AquaSimHeader::DOWN);
  //ash.addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_TMAC);

  synh.SetPtype(PT_SYN);
  synh.SetPktNum(m_numSend);
  synh.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  synh.SetDuration(m_duration);
  m_numSend++;

  pkt->AddHeader(synh);
  pkt->AddHeader(ash);
  pkt->AddPacketTag(ptag);
  NS_LOG_INFO("SendSYN:node(" << m_device->GetNode() <<
                ") send SYN packet at " << Simulator::Now().GetSeconds());
  TxND(pkt, m_phaseTwoWindow);
}


void
AquaSimTMac::InitND(double t1,double t2, double t3)
{
  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
  double delay=m_rand->GetValue()*t1;
  double itval=(t3-t2-t1)/2.0;
  double delay3=t1+itval;

  m_shortNdEvent = Simulator::Schedule(Seconds(delay),&AquaSimTMac::TNDHandler,this);
  m_shortAckndEvent = Simulator::Schedule(Seconds(delay3),&AquaSimTMac::SendShortAckND,this);
  return;
}

void
AquaSimTMac::SendND(int pkt_size)
{
  Ptr<Packet> pkt =Create<Packet>();
  TMacHeader ndh;
  AquaSimHeader ash;
  AquaSimPtTag ptag;

  // additional 2*8 denotes the size of type,next-hop of the packet and
  // timestamp

  //  cmh->size()=sizeof(hdr_nd)+3*8;
  //  printf("old size is %d\n",cmh->size());
  ash.SetSize(m_shortPacketSize);

  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetDirection(AquaSimHeader::DOWN);
  //ash.addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_TMAC);

  ndh.SetPtype(PT_ND);
  ndh.SetPktNum(m_numSend);
  ndh.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  m_numSend++;

  pkt->AddHeader(ndh);
  pkt->AddHeader(ash);
  pkt->AddPacketTag(ptag);

  // iph->src_.addr_=node_->AquaSimAddress();
  // iph->dst_.addr_=node_->AquaSimAddress();
  //iph->dst_.port_=255;

  NS_LOG_INFO("SendND:node(" << m_device->GetNode() <<
                ") send ND type is " << ptag.GetPacketType() <<
                " at " << Simulator::Now().GetSeconds());
  TxND(pkt, m_ndWindow);
}


void
AquaSimTMac::SendShortAckND()
{
  NS_LOG_FUNCTION(this << m_device->GetNode());
  if (m_arrivalTableIndex==0) return;// not ND received

  while(m_arrivalTableIndex>0){
      Ptr<Packet> pkt = Create<Packet>();

      TMacHeader ackndh;
      AquaSimHeader ash;
      AquaSimPtTag ptag;

      ackndh.SetPtype(PT_SACKND);
      ackndh.SetPktNum(m_numSend);
      ackndh.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
      m_numSend++;

      int index1=-1;
      index1=rand()%m_arrivalTableIndex;
      double t2=-0.1;
      double t1=-0.1;

      AquaSimAddress receiver=m_arrivalTable[index1].node_addr;
      t2=m_arrivalTable[index1].arrival_time;
      t1=m_arrivalTable[index1].sending_time;

      for(int i=index1;i<m_arrivalTableIndex;i++){
          m_arrivalTable[i].node_addr=m_arrivalTable[i+1].node_addr;
          m_arrivalTable[i].sending_time=m_arrivalTable[i+1].sending_time;
          m_arrivalTable[i].arrival_time=m_arrivalTable[i+1].arrival_time;
      	}

      ackndh.SetArrivalTime(t2);
      ackndh.SetTS(t1);
  // additional 2*8 denotes the size of type,next-hop of the packet and
  // timestamp
  //  ash->size()=sizeof(hdr_ack_nd)+3*8;

      ash.SetSize(m_shortPacketSize);
      ash.SetNextHop(receiver);
      ash.SetDirection(AquaSimHeader::DOWN);
      //ash.addr_type()=NS_AF_ILINK;
      ptag.SetPacketType(AquaSimPtTag::PT_TMAC);

      pkt->AddHeader(ackndh);
      pkt->AddHeader(ash);
      pkt->AddPacketTag(ptag);
      Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
      double delay=m_rand->GetValue()*m_ackNdWindow;
      Simulator::Schedule(Seconds(delay),&AquaSimTMac::TxND,this,pkt,m_ackNdWindow);
    	m_arrivalTableIndex--;
  }


  m_arrivalTableIndex=0;
  for(int i=0;i<T_TABLE_SIZE;i++)
    m_arrivalTable[i].node_addr=AquaSimAddress();

  return;
}


void
AquaSimTMac::ProcessShortACKNDPacket(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this << m_device->GetNode());
  TMacHeader ackndh;
  AquaSimHeader ash;
  AquaSimPtTag ptag;

  pkt->RemoveHeader(ash);
  pkt->PeekHeader(ackndh);
  pkt->AddHeader(ash);
  pkt->RemovePacketTag(ptag);

  //ash.size()=m_shortPacketSize;
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetDirection(AquaSimHeader::DOWN);
  //ash.addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_TMAC);

  ackndh.SetPtype(PT_SYN);
  ackndh.SetPktNum(m_numSend);
  ackndh.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  ackndh.SetDuration(m_duration);
  m_numSend++;


  AquaSimAddress sender=ackndh.GetSenderAddr();
  double t4=Simulator::Now().ToDouble(Time::S);
  double t3=ash.GetTimeStamp().ToDouble(Time::S);
  AquaSimAddress myaddr=AquaSimAddress::ConvertFrom(m_device->GetAddress()) ;

  double t2=ackndh.GetArrivalTime();
  double t1=ackndh.GetTS();

  double latency=((t4-t1)-(t3-t2))/2.0;
  bool newone=true;

  pkt=0;

  for (int i=0;i<T_TABLE_SIZE;i++)
  if (m_shortLatencyTable[i].node_addr==sender)
    {
      m_shortLatencyTable[i].sumLatency+=latency;
      m_shortLatencyTable[i].num++;
      m_shortLatencyTable[i].last_update_time=Simulator::Now().ToDouble(Time::S);
      m_shortLatencyTable[i].latency =
                m_shortLatencyTable[i].sumLatency/m_shortLatencyTable[i].num;
      newone=false;
    }

  if(newone)
    {

      if(m_shortLatencyTableIndex>=T_TABLE_SIZE){
        NS_LOG_WARN("ProcessNDPacket: m_arrivalTable is full");
        return;
      }

    m_shortLatencyTable[m_shortLatencyTableIndex].node_addr=sender;
    m_shortLatencyTable[m_shortLatencyTableIndex].sumLatency+=latency;
    m_shortLatencyTable[m_shortLatencyTableIndex].num++;
    m_shortLatencyTable[m_shortLatencyTableIndex].last_update_time=
          Simulator::Now().ToDouble(Time::S);
    m_shortLatencyTable[m_shortLatencyTableIndex].latency =
          m_shortLatencyTable[m_shortLatencyTableIndex].sumLatency/m_shortLatencyTable[m_shortLatencyTableIndex].num;
    m_shortLatencyTableIndex++;
    }
  for(int i=0;i<m_shortLatencyTableIndex;i++)
    {
      NS_LOG_INFO("ProcessNDPacket:node(" << myaddr << ") to node (" <<
          m_shortLatencyTable[i].node_addr << ") short latency is " <<
          m_shortLatencyTable[i].latency << " and number is " <<
          m_shortLatencyTable[i].num);
    }
return;
}


void
AquaSimTMac::ProcessSYN(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this << m_device->GetNode());
  AquaSimHeader ash;
  TMacHeader synh;
  pkt->RemoveHeader(ash);
  pkt->PeekHeader(synh);
  pkt->AddHeader(ash);
  //hdr_cmn* cmh=HDR_CMN(pkt);

  AquaSimAddress sender=synh.GetSenderAddr();
  double interval=synh.GetInterval();
  double tduration=synh.GetDuration();
  pkt=0;

  double t1=-1.0;
  for (int i=0;i<T_TABLE_SIZE;i++)
  if (m_shortLatencyTable[i].node_addr==sender)
      t1=m_shortLatencyTable[i].latency;

  if(t1==-1.0) {
      NS_LOG_WARN("ProcessSYN: I receive a SYN from unknown neighbor");
      return;
    }

  interval-=t1;
  double t2=m_nextPeriod-(Simulator::Now().ToDouble(Time::S)-m_cycleStartTime);
  double d=interval-t2;

  if (d>=0.0) {
      while (d>=m_periodInterval) d-=m_periodInterval;
    }
  else
    {
      while (d+m_periodInterval<=0.0) d+=m_periodInterval;
    }

  bool newone=true;

  if(d<0) d=d+m_periodInterval;

  for (int i=0;i<T_TABLE_SIZE;i++)
  if (m_periodTable[i].node_addr==sender)
    {
      m_periodTable[i].difference=d;
      m_periodTable[i].last_update_time=Simulator::Now().ToDouble(Time::S);
      m_periodTable[i].duration =tduration;
      newone=false;
    }

  if(newone)
    {

      if(m_periodTableIndex>=T_TABLE_SIZE){
        NS_LOG_WARN("ProcessSYN: m_periodTable is full");
        return;
      }

      m_periodTable[m_periodTableIndex].node_addr=sender;
      m_periodTable[m_periodTableIndex].difference=d;
      m_periodTable[m_periodTableIndex].last_update_time=Simulator::Now().ToDouble(Time::S);
      m_periodTable[m_periodTableIndex].duration=tduration;
      m_periodTableIndex++;
    }

  for(int i=0;i<m_periodTableIndex;i++)
  {
    NS_LOG_INFO("ProcessSYN: node(" << m_device->GetAddress() <<
        ") to node (" << m_periodTable[i].node_addr <<
        ") period difference is " << m_periodTable[i].difference);
  }

 return;
}



/*
void
AquaSimTMac::ProcessSYN(Ptr<Packet> pkt)
{
  // printf("AquaSimTMac:ProcessSYN: node %d\n",index_);
    hdr_tmac* synh=HDR_TMAC(pkt);
    hdr_cmn* cmh=HDR_CMN(pkt);

    int  sender=synh->sender_addr;
    double interval=synh->interval;
    double tduration=synh->duration;
      Packet::free(pkt);


    double t1=-1.0;
 for (int i=0;i<T_TABLE_SIZE;i++)
 if (m_shortLatencyTable[i].node_addr==sender)
     t1=m_shortLatencyTable[i].latency;

 if(t1==-1.0) {
   printf("AquaSimTMac:ProcessSYN: I receive a SYN from unknown neighbor\n");
   return;
 }

 interval-=t1;
 double t2=m_nextPeriod-(Simulator::Now()-m_cycleStartTime);
 double d=interval-t2;

if (d>=0.0) {
   while (d>=m_periodInterval) d-=m_periodInterval;
 }
 else
   {
     while (d+m_periodInterval<=0.0) d+=m_periodInterval;
   }



 bool newone=true;

 for (int i=0;i<T_TABLE_SIZE;i++)
 if (m_periodTable[i].node_addr==sender)
      {
       m_periodTable[i].difference=d;
       m_periodTable[i].last_update_time=Simulator::Now();
       m_periodTable[i].duration =tduration;
       newone=false;
      }

 if(newone)
{

    if(m_periodTableIndex>=T_TABLE_SIZE){
      printf("AquaSimTMac:ProcessSYN:m_periodTable is full\n");
      return;
    }

    m_periodTable[m_periodTableIndex].node_addr=sender;
    m_periodTable[m_periodTableIndex].difference=d;
    m_periodTable[m_periodTableIndex].last_update_time=Simulator::Now();
    m_periodTable[m_periodTableIndex].duration=tduration;
    m_periodTableIndex++;
}

 for(int i=0;i<m_periodTableIndex;i++)
   printf("node (%d) to node (%d) period difference  is %f \n",index_,m_periodTable[i].node_addr, m_periodTable[i].difference);

 return;

}

*/


void
AquaSimTMac::ProcessSleep()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << Simulator::Now().GetSeconds());
  m_macStatus=TMAC_IDLE;
  PowerOff();
}


/*
void
AquaSimTMac::TxND(Ptr<Packet> pkt, double window)
{
  //  printf("AquaSimTMac TxND node %d\n",index_);
  hdr_cmn* cmh=HDR_CMN(pkt);
   hdr_tmac* synh = HDR_TMAC(pkt);

  UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;


  hdr_cmn::access(pkt)->txtime()=(cmh->size()*m_encodingEfficiency+
                                   m_phyOverhead)/m_bitRate;

  double txtime=hdr_cmn::access(pkt)->txtime();

  if(SLEEP==n->GetTransmissionStatus()) {
  PowerOn();
  n->SetTransmissionStatus(SEND);
  cmh->ts_=Simulator::Now();


  sendDown(pkt);
  m_tBackoffCounter=0;  //clear
  TStatusHandler_SetStatus(NIDLE);
  m_statusEvent = Simulator::Schedule(Seconds(txtime),&AquaSimTMac::TStatusHandler,this);
  return;
  }

  if(NIDLE==n->GetTransmissionStatus()){

  n->SetTransmissionStatus(SEND);

  //printf("TxND the data type is %d\n",MAC_BROADCAST);
  //printf("broadcast : I am going to send the packet down tx is %f\n",txtime);
     cmh->ts_=Simulator::Now();



  sendDown(pkt);
  m_tBackoffCounter=0;  //clear
ler.clear();
  //  printf("broadcast %d Tx Idle set timer at %f tx is %f\n",node_->nodeid(),Simulator::Now(),txtime);

   TStatusHandler_SetStatus(NIDLE);
   m_statusEvent = Simulator::Schedule(Seconds(txtime),&AquaSimTMac::TStatusHandler,this);
  return;
  }

  if(RECV==n->GetTransmissionStatus())
    {
      Scheduler& s=Scheduler::instance();
      double d1=window-(Simulator::Now()-m_cycleStartTime);

      if(d1>0){
      double backoff=m_rand->GetValue()*d1;
      m_tBackoffWindow=window;
   // printf("broadcast Tx set timer at %f backoff is %f\n",Simulator::Now(),backoff);
      Simulator::Schedule(Seconds(backoff),&AquaSimTMac::TBackoffHandler,this);
      return;
      }
      else {
          m_tBackoffCounter=0;  //clear

          printf("AquaSimTMac:backoff:no time left \n");
          Packet::free(pkt);
      }

    }

if (SEND==n->GetTransmissionStatus())
{
  // this case is supposed not to  happen
    printf("AquaSimTMac: queue send data too fas\n");
    Packet::free(pkt);
      return;
}

}

*/


void
AquaSimTMac::TxND(Ptr<Packet> pkt, double window)
{
	NS_LOG_FUNCTION(this << m_device->GetAddress());
	TMacHeader synh;
	AquaSimHeader ash;
  pkt->RemoveHeader(ash);
	pkt->RemoveHeader(synh);

  ash.SetTxTime(GetTxTime(ash.GetSerializedSize() + synh.GetSerializedSize()));
	//hdr_cmn::access(pkt)->txtime()=getTxTime(cmh->size());

	/*(cmh->size()*m_encodingEfficiency+m_phyOverhead)/m_bitRate;*/

	Time txtime=ash.GetTxTime();

	if(SLEEP==m_device->GetTransmissionStatus()) {
		PowerOn();
		//m_device->SetTransmissionStatus(SEND);
		ash.SetTimeStamp(Simulator::Now());

		if(m_phaseStatus==PHASETWO) {

			double t=Simulator::Now().ToDouble(Time::S)-m_cycleStartTime;

			synh.SetInterval(m_nextPeriod-t);
		}
    pkt->AddHeader(synh);
    pkt->AddHeader(ash);
		SendDown(pkt);
		m_tBackoffCounter=0; //clear

		TStatusHandler_SetStatus(NIDLE);
		m_statusEvent = Simulator::Schedule(txtime,&AquaSimTMac::TStatusHandler,this);
		return;
	}

	if(NIDLE==m_device->GetTransmissionStatus()) {

		//m_device->SetTransmissionStatus(SEND);

		//printf("TxND the data type is %d\n",MAC_BROADCAST);
		//printf("broadcast : I am going to send the packet down tx is %f\n",txtime);
		ash.SetTimeStamp(Simulator::Now());

		if(m_phaseStatus==PHASETWO) {

			double t=Simulator::Now().ToDouble(Time::S)-m_cycleStartTime;
			synh.SetInterval(m_nextPeriod-t);

		}
    pkt->AddHeader(synh);
    pkt->AddHeader(ash);
		SendDown(pkt);
		m_tBackoffCounter=0; //clear

		//  printf("broadcast %d Tx Idle set timer at %f tx is %f\n",node_->nodeid(),Simulator::Now(),txtime);

		TStatusHandler_SetStatus(NIDLE);
		m_statusEvent = Simulator::Schedule(txtime,&AquaSimTMac::TStatusHandler,this);
		return;
	}

	if(RECV==m_device->GetTransmissionStatus())
	{
		double d1=window-(Simulator::Now().ToDouble(Time::S)-m_cycleStartTime);

		if(d1>0) {
      Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
			double backoff=m_rand->GetValue()*d1;
			m_tBackoffWindow=window;
      pkt->AddHeader(synh);
      pkt->AddHeader(ash);
			// printf("broadcast Tx set timer at %f backoff is %f\n",Simulator::Now(),backoff);
			Simulator::Schedule(Seconds(backoff),&AquaSimTMac::TBackoffHandler,this,pkt);
			return;
		}
		else {
			m_tBackoffCounter=0; //clear

      NS_LOG_WARN("TMac:backoff: no time left");
      pkt=0;
		}

	}

	if (SEND==m_device->GetTransmissionStatus())
	{
		// this case is supposed not to  happen
    NS_LOG_WARN("TMac:queue send data too fast");
    pkt=0;
		return;
	}

}


void
AquaSimTMac::InitPhaseThree()
{
  NS_LOG_FUNCTION(this);

  PrintTable();

  m_macStatus=TMAC_SLEEP;
  /* if(m_device->GetTransmissionStatus()==SLEEP)*/
  Wakeup();
  return;
}


void
AquaSimTMac::ResetMacStatus()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << Simulator::Now().GetSeconds());

 // the node is receiving some data
  if(m_device->GetTransmissionStatus()==RECV)
   {
     m_macStatus=TMAC_IDLE;
     m_poweroffEvent = Simulator::Schedule(Seconds(m_taDuration),&AquaSimTMac::ResetMacStatus,this);
   }
  else
   {
     m_macStatus=TMAC_SLEEP;
     PowerOff();
   }
}


void
AquaSimTMac::Wakeup()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << Simulator::Now().GetSeconds());

	//  m_poweroffEvent.Cancel();
	//  m_wakeupEvent.Cancel(); //? necessary?
	// m_poweroffEvent = Simulator::Schedule(Seconds(m_taDuration),&AquaSimTMac::ResetMacStatus,this);
	m_wakeupEvent = Simulator::Schedule(Seconds(m_periodInterval),&AquaSimTMac::Wakeup,this);

	if (m_macStatus==TMAC_SLEEP)
	{
		PowerOn();

		m_macStatus=TMAC_IDLE;
		m_poweroffEvent = Simulator::Schedule(Seconds(m_taDuration),&AquaSimTMac::ResetMacStatus,this);

		m_cycleStartTime=Simulator::Now().ToDouble(Time::S);

		if (NewData())
		{
      NS_LOG_INFO("WakeUp: There is new data in node " << m_device->GetAddress() <<
          " and the number of packet is " << m_txbuffer.num_of_packet);
			SendRTS();
		}
	}
	return;
}


void
AquaSimTMac::ReStart()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << m_taDuration << Simulator::Now().GetSeconds());

	m_poweroffEvent.Cancel();
	//m_wakeupEvent.Cancel();
	m_poweroffEvent = Simulator::Schedule(Seconds(m_taDuration),&AquaSimTMac::ResetMacStatus,this);

	if((m_macStatus==TMAC_SILENCE)||(m_macStatus==TMAC_IDLE))
	{
		m_macStatus=TMAC_IDLE;
		if (NewData())
		{
      NS_LOG_INFO("Restart: There is new data in node " << m_device->GetAddress() <<
          " and the number of packet is " << m_txbuffer.num_of_packet);
			SendRTS();
		}
	}
	return;
}


void
AquaSimTMac::PrintTable()
{
  NS_LOG_FUNCTION(this << "Short latency Table" << m_device->GetAddress());


	for (int i=0; i<T_TABLE_SIZE; i++)
	{
    NS_LOG_INFO("Node addr is " << m_shortLatencyTable[i].node_addr <<
        " and short latency is " << m_shortLatencyTable[i].latency);
	}

  NS_LOG_FUNCTION(this << "Period Table" << m_device->GetAddress());

	for (int i=0; i<T_TABLE_SIZE; i++)
	{
    NS_LOG_INFO("Node addr is " << m_periodTable[i].node_addr <<
        " and difference is " << m_periodTable[i].difference);
	}
}

bool
AquaSimTMac::NewData()
{
  return (!m_txbuffer.IsEmpty());//?think about it
}


void
AquaSimTMac::ProcessSilence()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << m_silenceTableIndex << Simulator::Now().GetSeconds());

  CleanSilenceTable();

	if(m_silenceTableIndex==0)
	{
		InitializeSilenceTable();
		ReStart();
		return;
	}

  NS_LOG_INFO("ProcessSilence: node " << m_device->GetAddress() <<
      ": there still exists silence record..");
	double silenceTime=0;
	silenceTime=m_silenceTable[0].start_time+m_silenceTable[0].duration;
	for (int i=0; i<m_silenceTableIndex; i++)
	{
		double t1=m_silenceTable[i].start_time;
		double t2=m_silenceTable[i].duration;
		if(silenceTime<t1+t2) silenceTime=t1+t2;
	}

	double t=silenceTime-Simulator::Simulator::Now().ToDouble(Time::S);
	m_silenceEvent.Cancel();
	m_silenceEvent = Simulator::Schedule(Seconds(t),&AquaSimTMac::ProcessSilence,this);
	m_lastSilenceTime=Simulator::Simulator::Now().ToDouble(Time::S)+t;
	return;
}

void
AquaSimTMac::CleanSilenceTable()
{
	if(m_silenceTableIndex==0) return;
	int i=0;

	while (i<m_silenceTableIndex)
	{
		double st=m_silenceTable[i].start_time;
		double du=m_silenceTable[i].duration;

		if ( (m_silenceTable[i].confirm_id==0) ||
		    ((st+du<=Simulator::Simulator::Now().ToDouble(Time::S)) &&
			(m_silenceTable[i].node_addr!=AquaSimAddress()) ))
		{
			NS_LOG_INFO("CleanSilence: node " << m_device->GetAddress() <<
			    " clears the silence record...");
			DeleteSilenceTable(i);
		}
		else i++;
	}

}


void
AquaSimTMac::DeleteSilenceTable(int index)
{
	for(int i=index; i<m_silenceTableIndex; i++)
	{
		m_silenceTable[i].node_addr=m_silenceTable[i+1].node_addr;
		m_silenceTable[i].start_time=m_silenceTable[i+1].start_time;
		m_silenceTable[i].duration=m_silenceTable[i+1].duration;
		m_silenceTable[i].confirm_id=m_silenceTable[i+1].confirm_id;
	}
	m_silenceTableIndex--;
	return;
}

void
AquaSimTMac::DeleteSilenceRecord(AquaSimAddress node_addr)
{
	int index=-1;
	for(int i=0; i<m_silenceTableIndex; i++)
		if (m_silenceTable[i].node_addr==node_addr) index=i;

	if(index!=-1) DeleteSilenceTable(index);
	return;
}



void
AquaSimTMac::InitializeSilenceTable()
{
  for(int i=0;i<T_TABLE_SIZE;i++)
    {
      m_silenceTable[i].node_addr=AquaSimAddress();
      m_silenceTable[i].start_time=0;
      m_silenceTable[i].duration=0;
      m_silenceTable[i].confirm_id=0;
    }
  m_silenceTableIndex=0;
  return;
}


void
AquaSimTMac::ConfirmSilenceTable(AquaSimAddress sender_addr, double duration)
{
	int index=-1;
	for(int i=0; i<m_silenceTableIndex; i++)
		if(m_silenceTable[i].node_addr==sender_addr) index=i;

	if(index!=-1) m_silenceTable[index].confirm_id=1;
	else
	{
		InsertSilenceTable(sender_addr,duration);
		ConfirmSilenceTable(sender_addr, duration);
	}
	return;
}

void
AquaSimTMac::DataUpdateSilenceTable(AquaSimAddress sender_addr)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());

	int index=-1;
	for(int i=0; i<m_silenceTableIndex; i++)
		if(m_silenceTable[i].node_addr==sender_addr) index=i;

//printf("AquaSimTMac:DataUpdateSilenceTable node %d index of this record is %d...\n",index_,index);
	if(index!=-1) m_silenceTable[index].confirm_id=1;
	else
	{
		// printf("AquaSimTMac:DataUpdateSilenceTable node %d this is new data record...\n",index_);
		double t=2*m_maxPropagationTime+m_maxLargePacketTransmissionTime;
		// InsertSilenceTable(sender_addr,t);
		ConfirmSilenceTable(sender_addr,t);
	}
}


void
AquaSimTMac::SendRTS()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << Simulator::Now().GetSeconds());

	m_rtsTimeoutNum=0;
  if (m_rtsTimeoutEvent.IsRunning())
    m_rtsTimeoutEvent.Cancel();

	if(m_macStatus==TMAC_SILENCE)
	{
    NS_LOG_INFO("SendRTS: node " << m_device->GetAddress() <<
        " is in TMAC_SILENCE state at " << Simulator::Now().GetSeconds() );
		return;
	}

	Ptr<Packet> p=m_txbuffer.head();
  TMacHeader rtsh;
  AquaSimHeader ash;
  p->RemoveHeader(ash);
  p->RemoveHeader(rtsh);
	AquaSimAddress receiver_addr=ash.GetNextHop();

	m_txbuffer.LockBuffer();
	int num=m_txbuffer.num_of_packet;

  NS_LOG_INFO("SendRTS: node " << m_device->GetAddress() << " local m_txbuffer");

	//AquaSimAddress sender_addr=AquaSimAddress::ConvertFrom(m_device->GetAddress()) ;
	double l=CheckLatency(m_shortLatencyTable,receiver_addr);
	double du=num*(((m_largePacketSize*m_encodingEfficiency+m_phyOverhead)/m_bitRate)+m_transmissionTimeError);
	double dt=3.1*l+m_maxPropagationTime+du*2+m_maxPropagationTime-m_maxShortPacketTransmissionTime;

	// Generate a RTS Packet

	Ptr<Packet> pkt =Create<Packet>();
  AquaSimHeader ashNew;
  AquaSimPtTag ptag;

  ashNew.SetNextHop(receiver_addr);
  ashNew.SetDirection(AquaSimHeader::DOWN);
  //ashNew.addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_TMAC);

  rtsh.SetPtype(PT_RTS);
  rtsh.SetPktNum(m_numSend);
  rtsh.SetDuration(dt);
  rtsh.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  rtsh.SetRecvAddr(receiver_addr);
  m_numSend++;

  ashNew.SetTxTime(GetTxTime(ash.GetSerializedSize() + rtsh.GetSerializedSize()));
	//hdr_cmn::access(pkt)->txtime()=getTxTime(cmh->size());
	/**m_encodingEfficiency+ m_phyOverhead)/m_bitRate;*/

	//Time txtime=ash.GetTxTime();

	m_macStatus=TMAC_RTS;

	m_poweroffEvent.Cancel();
  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
	double t2=m_rand->GetValue()*m_contentionWindow;

  pkt->AddHeader(rtsh);
  pkt->AddHeader(ashNew);
  pkt->AddPacketTag(ptag);
	m_rtsRecvAddr=receiver_addr;
  NS_LOG_INFO("SendRTS: node " << m_device->GetAddress() <<
        "is in TMAC_RTS at " << Simulator::Now().GetSeconds() <<
        " will tx RTS in " << t2);
	Simulator::Schedule(Seconds(t2),&AquaSimTMac::TxRTS,this,pkt,m_rtsRecvAddr);
	//   printf("TxRTS, node %d is in  TMAC_RTS at %f\n",index_,Simulator::Simulator::Now()());
}

void
AquaSimTMac::TxRTS(Ptr<Packet> pkt,AquaSimAddress receiver_addr)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << Simulator::Now().GetSeconds());

	m_rtsTimeoutEvent.Cancel();

	if(m_macStatus==TMAC_SILENCE)
	{
    NS_LOG_INFO("TxRTS:node(" << m_device->GetNode() <<
          ") is at silence state quit " << Simulator::Now().GetSeconds());
		m_rtsTimeoutNum=0;
    pkt=0;
		return;
	}

  AquaSimHeader ash;
  TMacHeader tmac;
  pkt->RemoveHeader(ash);

  ash.SetTxTime(GetTxTime(ash.GetSerializedSize() + tmac.GetSerializedSize()));
	//hdr_cmn::access(pkt)->txtime()=getTxTime(cmh->size());
	/**m_encodingEfficiency+m_phyOverhead)/m_bitRate;*/
	Time txtime=ash.GetTxTime();

	double l=CheckLatency(m_shortLatencyTable, receiver_addr);
	double t=2.2*l+m_minBackoffWindow*2;

	TransStatus status=m_device->GetTransmissionStatus();

	if(NIDLE==status) {
		//m_device->SetTransmissionStatus(SEND);

		ash.SetTimeStamp(Simulator::Now());
    pkt->AddHeader(ash);
		SendDown(pkt);
		// printf("broadcast %d Tx Idle set timer at %f tx is %f\n",node_->nodeid(),Simulator::Now(),txtime);

    NS_LOG_INFO("TxRTS, node " << m_device->GetAddress() <<
        " Tx RTS at " << Simulator::Now().GetSeconds() <<
        " and timeout is " << t << " number of try is " << m_rtsTimeoutNum);
		TStatusHandler_SetStatus(NIDLE);
		// m_rtsTimeoutEvent.Cancel();
		m_statusEvent = Simulator::Schedule(txtime,&AquaSimTMac::TStatusHandler,this);
		m_rtsTimeoutNum=0;
		m_rtsTimeoutEvent = Simulator::Schedule(Seconds(t),&AquaSimTMac::RTSTimeoutHandler,this);

		return;
	}


	if(RECV==status)
	{
    NS_LOG_INFO("TxRTS, node " << m_device->GetAddress() <<
		    " is in RECV state, backoff...");

    Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
		double t2=m_minBackoffWindow+m_rand->GetValue()*m_minBackoffWindow;
		m_rtsRecvAddr=receiver_addr;
    pkt->AddHeader(ash);
		Simulator::Schedule(Seconds(t2),&AquaSimTMac::TxRTS,this,pkt,m_rtsRecvAddr);

		return;
	}

	if (SEND==status)
	{
    NS_LOG_INFO("TxRTS: queue send data too fast\n");
		pkt=0;
		return;
	}

}


double
AquaSimTMac::CheckLatency(t_latency_record* table,AquaSimAddress addr)
{
	int i=0;
	double d=0.0;

	while((table[i].node_addr!=addr)&&(i<T_TABLE_SIZE))
	{
		//printf("node addr is%d and latency is%f\n",table[i].node_addr,table[i].latency);
		i++;
	}
	if (i==T_TABLE_SIZE) return d;
	else return table[i].latency;
}


double
AquaSimTMac:: CheckDifference(t_period_record* table,AquaSimAddress addr)
{
	int i=0;
	double d=-0.0;

	while((table[i].node_addr!=addr)&&(i<T_TABLE_SIZE)) i++;

	if (i==T_TABLE_SIZE) return d;
	else return table[i].difference;
}


void
AquaSimTMac::ProcessCTSPacket(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << Simulator::Now().GetSeconds());
  AquaSimHeader ash;
  TMacHeader ctsh;
  pkt->RemoveHeader(ash);
  pkt->PeekHeader(ctsh);
  pkt->AddHeader(ash);

	//hdr_cmn* cmh=HDR_CMN(pkt);

	AquaSimAddress sender_addr=ctsh.GetSenderAddr();
	AquaSimAddress receiver_addr=ctsh.GetRecvAddr();
	double dt=ctsh.GetDuration();
	double l=CheckLatency(m_shortLatencyTable,sender_addr);
	double t=dt-2*l;

  pkt=0;

	if(receiver_addr!=m_device->GetAddress())
	{
		if(t>0)
		{
			ConfirmSilenceTable(receiver_addr,t);
			if(m_macStatus==TMAC_SILENCE)
			{
        NS_LOG_INFO("ProcessCTS: node(" << m_device->GetNode() <<
              ") I am already in silence state");
				if(m_lastSilenceTime<t+Simulator::Now().ToDouble(Time::S))
				{
          NS_LOG_INFO("ProcessCTS: node(" << m_device->GetNode() <<
                ") the silence is longer than existing one...");
					m_silenceEvent.Cancel();
					m_silenceEvent = Simulator::Schedule(Seconds(t),&AquaSimTMac::ProcessSilence,this);
					m_lastSilenceTime=Simulator::Now().ToDouble(Time::S)+t;
				}
			}// end of silence state
			else
			{
        NS_LOG_INFO("ProcessCTS: node(" << m_device->GetNode() <<
              ") I am going to be in silence state");

				if(m_macStatus==TMAC_IDLE) m_poweroffEvent.Cancel();
				m_timeoutEvent.Cancel();

				m_silenceEvent = Simulator::Schedule(Seconds(t),&AquaSimTMac::ProcessSilence,this);
				m_macStatus=TMAC_SILENCE;
				m_lastSilenceTime=Simulator::Now().ToDouble(Time::S)+t;
			}
		}
	}// end of no-intended receiver
	else {
		if(m_macStatus!=TMAC_RTS) {
      NS_LOG_INFO("ProcessCTS:status change, I quit this chance");
			return;
		}

		NS_LOG_INFO("ProcessCTS: node " << m_device->GetNode() << " this CTS is for me");

		double t=m_maxPropagationTime-l;
		m_rtsTimeoutEvent.Cancel();// cancel the timer of RTS
		m_rtsTimeoutNum=0;
		m_transmissionAddr=sender_addr;
		Simulator::Schedule(Seconds(t),&AquaSimTMac::TxData,this,m_transmissionAddr);
	}

	return;
}


void
AquaSimTMac::ClearTxBuffer()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() );

  Ptr<Packet>  p1[MAXIMUM_BUFFER];

  //for (int i=0;i<MAXIMUM_BUFFER;i++)p1[i]=NULL;
  Ptr<buffer_cell> bp=m_txbuffer.head_;
  int i=0;
  while(bp){
    p1[i]=bp->packet;
    bp=bp->next;
    i++;
  }


  for (int i=0;i<MAXIMUM_BUFFER;i++){
    //   printf("ClearTxBuffer the poniter is%d\n",p1[i]);
    if (m_bitMap[i]==1) m_txbuffer.DeletePacket(p1[i]);

  }

  NS_LOG_INFO("ClearTxBuffer:m_txbuffer is cleared, there are " <<
      m_txbuffer.num_of_packet << "packets left");

  return;
}



void
AquaSimTMac::ProcessACKDataPacket(Ptr<Packet> pkt)
{
	NS_LOG_FUNCTION(this << m_device->GetAddress() <<
	                Simulator::Now().GetSeconds() << m_duration);

  AquaSimHeader ash;
  pkt->PeekHeader(ash);
	//hdr_tmac* ackh=HDR_TMAC(pkt);

	AquaSimAddress dst=ash.GetNextHop();

	if(dst!=m_device->GetAddress()) {
		pkt=0;
		return;
	}

	if(m_macStatus!=TMAC_TRANSMISSION)
	{
    NS_LOG_INFO("ProcessACKDATAPacket:node(" << m_device->GetNode() <<
		      " not in transmission state, just quit");
    pkt=0;
		return;
	}

  NS_LOG_INFO("ProcessACKDATAPacket:node(" << m_device->GetNode() <<
        " cancel timeout duration=" << m_duration);
	m_timeoutEvent.Cancel();// cancel the timer of data


	for (int i=0; i<MAXIMUM_BUFFER; i++) m_bitMap[i]=0;

	//workout in place of pkt->accessdata()
	uint8_t *data = new uint8_t[sizeof(m_bitMap)];
	pkt->CopyData(data,sizeof(m_bitMap));
	memcpy(m_bitMap, data,sizeof(m_bitMap));
  NS_LOG_INFO("ProcessACKDATAPacket:node(" << m_device->GetNode() <<
	     "received the bitmap is:");

	for (int i=0; i<MAXIMUM_BUFFER; i++) NS_LOG_INFO("bmap[" << i << "]=" << m_bitMap[i]);

  NS_LOG_INFO("ProcessACKDATAPacket: m_txbuffer will be cleared, there are " <<
        m_txbuffer.num_of_packet << " packets in queue and duration=" << m_duration);

	pkt=0;


	/*
	   !!!!
	   This part should consider the retransmission state, in this implementation,
	   we don't consider the packets loss, therefore, we just ignore it, it should be added later.

	 */

	ClearTxBuffer();

	m_txbuffer.UnlockBuffer();

  NS_LOG_INFO("ProcessACKDATAPacket: node " << m_device->GetNode() <<
        " unlock m_txbuffer m_duration=" << m_duration);
	ResumeTxProcess();

	m_macStatus=TMAC_IDLE;

	ReStart();
	return;
}


void
AquaSimTMac::ProcessRTSPacket(Ptr<Packet> pkt)
{
	NS_LOG_FUNCTION(this << m_device->GetAddress());

  TMacHeader rtsh;
  AquaSimHeader ash;
  pkt->RemoveHeader(ash);
  pkt->PeekHeader(rtsh);
  pkt->AddHeader(ash);

	AquaSimAddress sender_addr=rtsh.GetSenderAddr();
	AquaSimAddress receiver_addr=ash.GetNextHop();

	double l=CheckLatency(m_shortLatencyTable, sender_addr);
	double duration=rtsh.GetDuration();
	double silenceTime=duration-2*l;
	double t=2*m_maxPropagationTime
	          +2*m_maxShortPacketTransmissionTime+2*m_minBackoffWindow;
  pkt=0;

	if((receiver_addr!=m_device->GetAddress())&&(silenceTime>0))
	{

		if(m_macStatus==TMAC_IDLE)
		{
      NS_LOG_INFO("ProcessRTS:node(" << m_device->GetNode() <<
			       ") I am not the intended receiver and will be in silence");
			InsertSilenceTable(sender_addr,silenceTime);

			m_macStatus=TMAC_SILENCE;

			m_poweroffEvent.Cancel();
			m_silenceEvent = Simulator::Schedule(Seconds(t),&AquaSimTMac::ProcessSilence,this);
			m_lastSilenceTime=t+Simulator::Now().ToDouble(Time::S);
			return;
		}
		if(m_macStatus==TMAC_SILENCE)
		{
			InsertSilenceTable(sender_addr,silenceTime);
			if(m_lastSilenceTime<t+Simulator::Now().ToDouble(Time::S))
			{
        NS_LOG_INFO("ProcessRTS:node(" << m_device->GetNode() <<
				      ") I am not the intended receiver, gets a longer silence...");

				m_silenceEvent.Cancel();
				m_silenceEvent = Simulator::Schedule(Seconds(t),&AquaSimTMac::ProcessSilence,this);
				m_lastSilenceTime=Simulator::Now().ToDouble(Time::S)+t;
			}
			return;
		}

		if(m_macStatus==TMAC_RTS)
		{

			if (sender_addr != m_device->GetAddress())
			{
        NS_LOG_INFO("ProcessRTS:node(" << m_device->GetNode() <<
              ") I am not the intended receiver and quits the  RTS state");
				InsertSilenceTable(sender_addr,silenceTime);
				m_macStatus=TMAC_SILENCE;
				m_poweroffEvent.Cancel();
				m_rtsTimeoutEvent.Cancel();
				m_rtsTimeoutNum=0;
				m_silenceEvent = Simulator::Schedule(Seconds(t),&AquaSimTMac::ProcessSilence,this);
				m_lastSilenceTime=t+Simulator::Now().ToDouble(Time::S);
			}
			return;
		}
    NS_LOG_INFO("ProcessRTS:node(" << m_device->GetNode() <<
          ") this RTS is not for me and I am in unknown state");
		return;
	}

	if(receiver_addr!=m_device->GetAddress()) return;
    //switch may be better here.
	if(m_macStatus==TMAC_IDLE)
	{
    NS_LOG_INFO("ProcessRTS:node(" << m_device->GetNode() <<
          ") is in idle state and ready to process the RTS");
		double dt=0;

		// dt=l+2.5*duration+m_maxPropagationTime;
		dt=duration-l;
		double dt1=dt-l-m_maxShortPacketTransmissionTime;

		m_poweroffEvent.Cancel();

		Ptr<Packet> p=GenerateCTS(sender_addr,dt);
		TxCTS(p);

		m_ackEvent.Cancel();
		m_ackEvent = Simulator::Schedule(Seconds(dt1),&AquaSimTMac::SendACKPacket,this);

		return;
	}

	if(m_macStatus==TMAC_CTS)
	{
    NS_LOG_INFO("ProcessRTS:node(" << m_device->GetNode() << ") is in CTS state");
		return;
	}

	if(m_macStatus==TMAC_SILENCE)
	{
    NS_LOG_INFO("ProcessRTS:node(" << m_device->GetNode() << ") is in SILENCE state");
		return;
	}

  NS_LOG_INFO("ProcessRTS:node(" << m_device->GetNode() << ") is in Unknown state");
}

void
AquaSimTMac:: InsertSilenceTable(AquaSimAddress sender_addr,double duration)
{
	int index=-1;
	for(int i=0; i<m_silenceTableIndex; i++)
		if(m_silenceTable[i].node_addr==sender_addr) index=i;


	if(index==-1) // this is a new silence record
	{
    NS_LOG_INFO("InsertSilenceTable:node(" << m_device->GetNode() <<
        ") this silence from node " << sender_addr << " is new one, duration=" <<
        duration << " at time " << Simulator::Now().GetSeconds());
		m_silenceTable[m_silenceTableIndex].node_addr=sender_addr;
		m_silenceTable[m_silenceTableIndex].start_time=Simulator::Now().ToDouble(Time::S);
		m_silenceTable[m_silenceTableIndex].duration=duration;
		m_silenceTable[m_silenceTableIndex].confirm_id=0;
		m_silenceTableIndex++;
	}
	else
	{
    NS_LOG_INFO("InsertSilenceTable:node(" << m_device->GetNode() <<
        ") this silence from node " << sender_addr << " is old one, duration=" <<
        duration << " at time " << Simulator::Now().GetSeconds());
		m_silenceTable[index].start_time=Simulator::Now().ToDouble(Time::S);
		m_silenceTable[index].duration=duration;
		m_silenceTable[index].confirm_id=0;
	}

	return;
}


Ptr<Packet>
AquaSimTMac::GenerateCTS(AquaSimAddress receiver_addr, double duration)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());

	Ptr<Packet> pkt =Create<Packet>();
  TMacHeader ctsh;
  AquaSimHeader ash;
  AquaSimPtTag ptag;
  ash.SetSize(m_shortPacketSize);
  ash.SetNextHop(receiver_addr);
  ash.SetDirection(AquaSimHeader::DOWN);
  //ash.addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_TMAC);

  ctsh.SetPtype(PT_CTS);
  ctsh.SetPktNum(m_numSend);
  ctsh.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  ctsh.SetRecvAddr(receiver_addr);
  ctsh.SetDuration(m_duration);
  m_numSend++;

  pkt->AddHeader(ctsh);
  pkt->AddHeader(ash);
  pkt->AddPacketTag(ptag);
	return pkt;
}


void
AquaSimTMac::TxCTS(Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << Simulator::Now().GetSeconds());
	if(m_macStatus==TMAC_SILENCE)
	{
    NS_LOG_INFO("TxCTS:node " << m_device->GetNode() <<
          ", I am in silence state, I have to quit..");
		m_ackEvent.Cancel();
		m_timeoutEvent.Cancel();
		m_ctsNum=0;
    p=0;
    return;
	}

	if(m_ctsNum==2)
	{
    NS_LOG_INFO("TxCTS:node " << m_device->GetNode() <<
          ", I have to try to send CTS twice, I have to quit..");
		m_ctsNum=0;
		m_ackEvent.Cancel();
    p=0;
  	m_macStatus=TMAC_IDLE;
		ReStart();
		return;
	}

  TMacHeader ctsh;
  AquaSimHeader ash;
  p->RemoveHeader(ash);
  p->PeekHeader(ctsh);

	m_macStatus=TMAC_CTS;
  ash.SetTxTime(GetTxTime(ash.GetSerializedSize() + ctsh.GetSerializedSize()));
	//hdr_cmn::access(p)->txtime()=getTxTime(cmh->size());
	/**m_encodingEfficiency+m_phyOverhead)/m_bitRate;*/

	Time txtime=ash.GetTxTime();
	AquaSimAddress receiver_addr=ash.GetNextHop();


	double l=CheckLatency(m_shortLatencyTable, receiver_addr);
	double t=2.2*l+m_minBackoffWindow*2+m_maxShortPacketTransmissionTime
	          +m_maxLargePacketTransmissionTime;
  //         unused
	//double t1=m_transmissionTimeError+ctsh.GetDuration();

	for (int i=0; i<MAXIMUM_BUFFER; i++) m_bitMap[i]=0;

	TransStatus status=m_device->GetTransmissionStatus();

	if(NIDLE==status) {
		//m_device->SetTransmissionStatus(SEND);

		ash.SetTimeStamp(Simulator::Now());
    p->AddHeader(ash);
		SendDown(p);

    NS_LOG_INFO("TxCTS:node" << m_device->GetNode() <<
		      " tx CTS " << Simulator::Now().GetSeconds() <<
          " and timeout is set at " << t);

		TStatusHandler_SetStatus(NIDLE);
		m_timeoutEvent.Cancel();
		m_ctsNum=0;
		m_statusEvent = Simulator::Schedule(txtime,&AquaSimTMac::TStatusHandler,this);
		m_timeoutEvent = Simulator::Schedule(Seconds(t),&AquaSimTMac::SetIdle,this);
		return;
	}

	if(RECV==status)
	{
    NS_LOG_INFO("TxCTS: node " << m_device->GetNode() << " has to back off");
    Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
		double t2=m_minBackoffWindow*(1+m_rand->GetValue());
    p->AddHeader(ash);
		Simulator::Schedule(Seconds(t2),&AquaSimTMac::CTSHandler,this,p);
		return;
	}

	if (SEND==status)
	{
		// this case is supposed not to  happen
    NS_LOG_WARN("SendCTS is in wrong status");
		p=0;
		return;
	}
}


void
AquaSimTMac::SetIdle()
{

	// sometimes, need to cancel the timer
	m_ackEvent.Cancel();

  NS_LOG_FUNCTION(this << m_device->GetAddress() << Simulator::Now().GetSeconds());
	if(m_macStatus==TMAC_SILENCE) return;
	else
	{
		m_macStatus=TMAC_IDLE;
		ReStart();
		return;
	}
}


void
AquaSimTMac::ProcessNDPacket(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());
  TMacHeader ndh;
  AquaSimHeader ash;
  pkt->RemoveHeader(ash);
  pkt->PeekHeader(ndh);
  pkt->AddHeader(ash);

	AquaSimAddress sender=ndh.GetSenderAddr();
	if(m_arrivalTableIndex>=T_TABLE_SIZE) {
    NS_LOG_INFO("ProcessNDPacket:m_arrivalTable is full\n");
		pkt=0;
		return;
	}
	m_arrivalTable[m_arrivalTableIndex].node_addr=sender;
	m_arrivalTable[m_arrivalTableIndex].arrival_time=Simulator::Now().ToDouble(Time::S);
	m_arrivalTable[m_arrivalTableIndex].sending_time=ash.GetTimeStamp().ToDouble(Time::S);
	m_arrivalTableIndex++;
  pkt=0;
	return;
}


void
AquaSimTMac::ProcessDataPacket(Ptr<Packet> pkt)
{
	NS_LOG_FUNCTION(this << m_device->GetAddress() << m_macStatus);

	//hdr_uwvb* vbh=HDR_UWVB(pkt);
	TMacHeader tmach;
	AquaSimHeader ash;
  pkt->RemoveHeader(ash);
	pkt->PeekHeader(tmach);
	pkt->AddHeader(ash);

	AquaSimAddress dst=ash.GetNextHop();
	m_dataSender=tmach.GetSenderAddr();
	int num=tmach.GetDataNum();

	if(dst!=m_device->GetAddress())
	{
		if(m_macStatus==TMAC_SILENCE) DataUpdateSilenceTable(m_dataSender);
		else
		{
			NS_LOG_INFO("ProcessDataPacket: node " << m_device->GetNode() <<
			            ", I am not in silence state, my state is " << m_macStatus);

			double t=2*m_maxPropagationTime+m_maxLargePacketTransmissionTime;

			if(m_macStatus==TMAC_IDLE) m_poweroffEvent.Cancel();
			m_timeoutEvent.Cancel();
			m_macStatus=TMAC_SILENCE;
			m_silenceEvent.Cancel();
			m_silenceEvent = Simulator::Schedule(Seconds(t),&AquaSimTMac::ProcessSilence,this);

		}

		return;
	}

	if(m_macStatus==TMAC_CTS) m_macStatus=TMAC_RECV;
	m_timeoutEvent.Cancel();

	MarkBitMap(num);
	NS_LOG_INFO("ProcessDataPacket: node " << m_device->GetNode() <<
	             " send up the packet");
  SendUp(pkt);
	// SendACKPacket();
	return;
}

void
AquaSimTMac::MarkBitMap(int num){
  if(num<MAXIMUM_BUFFER) m_bitMap[num]=1;
}


void
AquaSimTMac::SendACKPacket()
{
	NS_LOG_FUNCTION(this << m_device->GetAddress() << Simulator::Now().GetSeconds());

	if(m_dataSender.GetAsInt() < 0) {
		NS_LOG_INFO("ScheduleACKData: invalid sender address");
		return;
	}

	if(m_macStatus!=TMAC_RECV) {
		NS_LOG_INFO("ScheduleACKData: invalid state\n");
		return;
	}

	Ptr<Packet> pkt=Create<Packet>(sizeof(m_bitMap));
  TMacHeader revh;
  AquaSimHeader ash;
  AquaSimPtTag ptag;

  //work around for pkt->accessdata()
  uint8_t *data = new uint8_t[sizeof(m_bitMap)];
  memcpy(data,m_bitMap,sizeof(m_bitMap));
  Ptr<Packet> tempPacket = Create<Packet>(data,sizeof(m_bitMap));
  pkt->AddAtEnd(tempPacket);

  NS_LOG_INFO("ScheduleACKData: Schdeule ACKDATA: node " << m_device->GetNode() <<
              " return bitmap is");
	for(int i=0; i<MAXIMUM_BUFFER; i++) NS_LOG_INFO("bmap[" << i << "]=" << m_bitMap[i]);

  ash.SetSize(m_shortPacketSize);
  ash.SetNextHop(m_dataSender);
  ash.SetDirection(AquaSimHeader::DOWN);
  //ash.addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_TMAC);

  revh.SetPtype(PT_ACKDATA);
  revh.SetPktNum(m_numSend);
  revh.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  m_numSend++;

  pkt->AddHeader(revh);
  pkt->AddHeader(ash);
  pkt->AddPacketTag(ptag);
	TxACKData(pkt);
}


void
AquaSimTMac::TxACKData(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << Simulator::Now().GetSeconds());
  AquaSimHeader ash;
  TMacHeader tmac;
  pkt->RemoveHeader(ash);

  ash.SetTxTime(GetTxTime(ash.GetSerializedSize() + tmac.GetSerializedSize()));
	//hdr_cmn::access(pkt)->txtime()=getTxTime(cmh->size());
	/**m_encodingEfficiency+m_phyOverhead)/m_bitRate;*/
	Time txtime=ash.GetTxTime();

	m_macStatus=TMAC_IDLE;

	m_poweroffEvent.Cancel();
	m_poweroffEvent = Simulator::Schedule(Seconds(m_taDuration),&AquaSimTMac::ResetMacStatus,this);

	//  printf("TxACKData, node %d is in  TMAC_IDLE at %f\n",index_,Simulator::Now());
	TransStatus status=m_device->GetTransmissionStatus();

	if(SLEEP==status) {
		PowerOn();
		//m_device->SetTransmissionStatus(SEND);
		ash.SetTimeStamp(Simulator::Now());
    pkt->AddHeader(ash);
		SendDown(pkt,SLEEP);

		// printf("AquaSimTMac TxACKData node %d at %f\n",index_,Simulator::Now());
		TStatusHandler_SetStatus(SLEEP);
		m_statusEvent = Simulator::Schedule(txtime,&AquaSimTMac::TStatusHandler,this);
		return;
	}

	if(NIDLE==status) {
    NS_LOG_INFO("TxACKData node " << m_device->GetNode() <<
              " is idle state at " << Simulator::Now().GetSeconds());
		//m_device->SetTransmissionStatus(SEND);

    ash.SetTimeStamp(Simulator::Now());
    pkt->AddHeader(ash);
		SendDown(pkt);

		TStatusHandler_SetStatus(NIDLE);
		m_statusEvent = Simulator::Schedule(txtime,&AquaSimTMac::TStatusHandler,this);
		return;
	}

	if(RECV==status)
	{
    NS_LOG_INFO("TxACKData node " << m_device->GetNode() <<
              " is in recv state at " << Simulator::Now().GetSeconds() <<
              " will be interrupted");

		InterruptRecv(txtime.GetDouble());
    ash.SetTimeStamp(Simulator::Now());
    pkt->AddHeader(ash);
		SendDown(pkt);

		TStatusHandler_SetStatus(NIDLE);
		m_statusEvent = Simulator::Schedule(txtime,&AquaSimTMac::TStatusHandler,this);

		return;
	}
	if (SEND==status)
	{
    NS_LOG_INFO("TxACKData node " << m_device->GetNode() << " send data too fast");
		pkt=0;
		return;
	}
}


/*
 this program is used to handle the received packet,
it should be virtual function, different class may have
different versions.
*/

void
AquaSimTMac::TxData(AquaSimAddress receiver)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << Simulator::Now().GetSeconds());

	if (m_txbuffer.IsEmpty())
	{
    NS_LOG_INFO("TxData: what the hell! I don't have data to send");
		return;
	}

	if((m_macStatus!=TMAC_RTS)&&(m_macStatus!=TMAC_TRANSMISSION)) {
    NS_LOG_INFO("TxData:node " << m_device->GetNode() << " is not in transmission state");
		return;
	}

	if(m_device->GetTransmissionStatus()==SLEEP) PowerOn();

	m_macStatus=TMAC_TRANSMISSION;

	Ptr<Packet> pkt=m_txbuffer.next();
  TMacHeader datah;
  AquaSimHeader ash;
  AquaSimPtTag ptag;
  pkt->RemoveHeader(ash);
  pkt->RemoveHeader(datah);
  pkt->RemovePacketTag(ptag);
	//hdr_uwvb* hdr2=hdr_uwvb::access(pkt);

	/* printf("AquaSimTMac:node %d TxData at time %f data type
	     is %d offset is%d and size is %d and offset is %d
	     and size is%d uwvb offset is %d and size is %d\n",
	     index_,Simulator::Now(),hdr2->mess_type,cmh,sizeof(hdr_cmn),
	     datah,sizeof(hdr_tmac),hdr2,sizeof(hdr_uwvb));
	 */
	datah.SetPtype(PT_DATA);
  datah.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  datah.SetPktNum(m_numSend);
  datah.SetDataNum(m_numData);
	m_numSend++;
	m_numData++;

  ash.SetSize(m_largePacketSize);
  ash.SetNextHop(receiver);
  ash.SetDirection(AquaSimHeader::DOWN);
  //ash.addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_TMAC);
  pkt->AddPacketTag(ptag);
  ash.SetTxTime(GetTxTime(ash.GetSerializedSize() + datah.GetSerializedSize()));
	//hdr_cmn::access(pkt)->txtime()=getTxTime(cmh->size());
	/**m_encodingEfficiency+m_phyOverhead)/m_bitRate;*/
	Time txtime=ash.GetTxTime();

  NS_LOG_INFO("TxData:node " << m_device->GetNode() <<
  	           " TxData at time " << Simulator::Now().GetSeconds() <<
               /*" data type is " << hdr2->mess_type << */
               " packet data_num=" << datah.GetDataNum() <<
               " class data_num=" << m_numData);
	TransStatus status=m_device->GetTransmissionStatus();


	if(NIDLE==status)
	{
		//m_device->SetTransmissionStatus(SEND);
    pkt->AddHeader(datah);
    pkt->AddHeader(ash);
		SendDown(pkt);
		// printf("AquaSimTMac:node %d TxData at %f\n ",index_,Simulator::Now());
		TStatusHandler_SetStatus(NIDLE);
		m_statusEvent = Simulator::Schedule(txtime,&AquaSimTMac::TStatusHandler,this);
	}

	if(RECV==status)
	{
    NS_LOG_INFO("TxData:node " << m_device->GetNode() <<
                  " TxData interrupt the receiving status at " <<
                  Simulator::Now().GetSeconds());

		InterruptRecv(txtime.GetDouble());
    pkt->AddHeader(datah);
    pkt->AddHeader(ash);
		SendDown(pkt);

		TStatusHandler_SetStatus(NIDLE);
		m_statusEvent = Simulator::Schedule(txtime,&AquaSimTMac::TStatusHandler,this);
	}

	if (SEND==status)
	{
    NS_LOG_INFO("Txdata: queue send data too fast");
		pkt=0;
	}

	if (m_txbuffer.IsEnd())
  {
    NS_LOG_INFO("Txdata:node " << m_device->GetNode() <<
		            " is in state MAC_TRANSMISSION");

		// double l=CheckLatency(m_shortLatencyTable, receiver);
    //    unused
		//double dt=((m_largePacketSize*m_encodingEfficiency+m_phyOverhead)/m_bitRate)+m_transmissionTimeError;
		// double t=2.1*m_maxPropagationTime+m_transmissionTimeError+dt;
		double t=2.0*m_maxPropagationTime+2.0*m_minBackoffWindow+(2+m_numData)*m_maxLargePacketTransmissionTime;

    NS_LOG_INFO("TxData:node " << m_device->GetNode() <<
                  " TxData at " << Simulator::Now().GetSeconds() <<
                  " and timeout is set " << t);
		m_timeoutEvent.Cancel();
		m_timeoutEvent = Simulator::Schedule(Seconds(t),&AquaSimTMac::SetIdle,this);
		m_numData=0;
	}
	else {
		double it=m_sif+txtime.ToDouble(Time::S);
    NS_LOG_INFO("TxData:node " << m_device->GetNode() <<
                  " schedule next data packet, interval=" << it <<
                  " at time " << Simulator::Now().GetSeconds());
		Simulator::Schedule(Seconds(it),&AquaSimTMac::TxData,this,m_transmissionAddr);
	}
}


void
AquaSimTMac::ResumeTxProcess()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << Simulator::Now().GetSeconds());

	if(!m_txbuffer.IsFull())
    NS_LOG_WARN("ResumeTxProcess: txbuffer is full.");
		//if(callback_) callback_->handle(&m_statusEvent);
		return;
}



/*
 this program is used to handle the transmitted packet,
it should be virtual function, different class may have
different versions.
*/

bool
AquaSimTMac::TxProcess(Ptr<Packet> pkt)
{
	//hdr_uwvb* hdr=HDR_UWVB(pkt);
  NS_LOG_FUNCTION(this << m_device->GetAddress());
	if (m_device->GetHopStatus() != 0) {
    AquaSimHeader ash;
    pkt->RemoveHeader(ash);
		ash.SetNextHop(AquaSimAddress(m_device->GetNextHop()) );
		ash.SetErrorFlag(false);// set off the error status;
    pkt->AddHeader(ash);
		// printf("AquaSimTMac:TxProcess: node %d set next hop to %d\n",index_,cmh->next_hop());
	}

	m_txbuffer.AddNewPacket(pkt);
  NS_LOG_INFO("TxProcess:node " << m_device->GetNode() <<
	           " put new data packets in m_txbuffer");
	if(!m_txbuffer.IsFull())
		//if(callback_) callback_->handle(&m_statusEvent);
		return false;
	return true;
}


bool
AquaSimTMac::RecvProcess(Ptr<Packet> pkt)
{
  TMacHeader tmac;
  AquaSimHeader ash;
  pkt->RemoveHeader(ash);
  pkt->PeekHeader(tmac);
  if (tmac.GetPtype() == TMacHeader::PT_OTHER)
  {
    tmac.SetPktNum(pkt->GetUid());
    tmac.SetPtype(TMacHeader::PT_DATA);
    tmac.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
    pkt->AddHeader(tmac);
  }
  AquaSimAddress receiver_addr=tmac.GetSenderAddr();
  pkt->AddHeader(ash);
  AquaSimAddress dst=ash.GetNextHop();
  int ptype=tmac.GetPtype();

	if(ash.GetErrorFlag())
	{
    NS_LOG_INFO("RecvProcess:node " << m_device->GetNode() <<
              " gets a corrupted packet from node " << receiver_addr <<
              " at " << Simulator::Now().GetSeconds());
		return false;
	}
  NS_LOG_INFO("RecvProcess:node " << m_device->GetNode() <<
            " gets a packet from node " << receiver_addr <<
            " at " << Simulator::Now().GetSeconds());

	if(dst==AquaSimAddress::GetBroadcast()) {
		if (ptype==PT_ND) ProcessNDPacket(pkt); //this is ND packet
		if (ptype==PT_SYN) ProcessSYN(pkt);
		return true;
	}

  //this should be switch

	if ((ptype==PT_SACKND)&&(dst==m_device->GetAddress()))
	{
		ProcessShortACKNDPacket(pkt);
		return true;
	}
	if (ptype==PT_DATA) {
		ProcessDataPacket(pkt);
		return true;
	}
	if (ptype==PT_ACKDATA) {
		ProcessACKDataPacket(pkt);
		return true;
		// printf("underwaterbroadcastmac:this is my packet \n");
	}

	if(ptype==PT_RTS)
	{
		ProcessRTSPacket(pkt);
		return true;
	}

	if(ptype==PT_CTS)
	{
		ProcessCTSPacket(pkt);
		return true;
	}
  NS_LOG_INFO("RecvProcess:node " << m_device->GetNode() <<
            " this is neither broadcast nor my packet " << dst <<
            " , just drop it at " << Simulator::Now().GetSeconds());
  pkt=0;
	return false;
}


void
AquaSimTMac::StatusProcess(TransStatus state)
{
  NS_LOG_FUNCTION(this << m_device->GetNode());

	if(SLEEP==m_device->GetTransmissionStatus()) return;

	m_device->SetTransmissionStatus(state);

	return;
}

void AquaSimTMac::DoDispose()
{
  m_rand=0;
  AquaSimMac::DoDispose();
}

}  //namespace ns3
