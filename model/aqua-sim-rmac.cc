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

/*
In this version of RMAC, the major modifications for the 10-5-2006
  version are :
1. Adding the number of control for phase 2
2. Adding the full collecting reservations, i.e., each node has
   to collect reservations in two consecutive periods.
3. when the revack windows collide, the receiver will schedule the ackrev
in the next period interval
4. no carrier sensing necessary
*/

#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/random-variable-stream.h"
#include "ns3/double.h"
#include "ns3/integer.h"

#include "aqua-sim-rmac.h"
#include "aqua-sim-rmac-buffer.h"
#include "aqua-sim-phy.h"
#include "aqua-sim-header.h"
#include "aqua-sim-pt-tag.h"

#include <stdlib.h>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimRMac");
NS_OBJECT_ENSURE_REGISTERED(AquaSimRMac);

/*	Out of date code.

void
AquaSimRMac::InitPhaseThree(){


  printf("RMac: this is InitPhaseThree\n");

   SortPeriodTable();
   PrintTable();

  UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
  if(n->TransmissionStatus()==SLEEP) Poweron();

  mac_status=RMAC_IDLE;
   m_sleepEvent = Simulator::Schedule(Seconds(m_duration), &AquaSimRMac::ProcessSleep, this);

    return;
}
*/


/* ======================================================================
    RMAC for Aqua-Sim Next Generation
   ====================================================================== */

AquaSimRMac::AquaSimRMac()
{
  m_numSend=0;
  m_numData=0;
  m_numBlock=0;

  m_largePacketSize=30;
  m_shortPacketSize=10;
  m_timer=5;

  m_shortLatencyTableIndex=0;
  m_reservedTimeTableIndex=0;
  m_reservationTableIndex=0;
  m_ackDataTableIndex=0;
  m_periodTableIndex=0;
  m_nextPeriod=0;
  ack_rev_pt=NULL;

  m_recvBusy=false;
  m_carrierSense=false;
  m_collectRev=false;

  m_recvDataSender=AquaSimAddress();
  m_recvDuration=0;
  m_recvStatus=0;
  m_NDBackoffWindow=0;
  m_NDBackoffCounter=0;

  for(int i=0;i<R_TABLE_SIZE;i++){

    next_available_table[i].node_addr=AquaSimAddress(-1);
    next_available_table[i].required_time=0;

    short_latency_table[i].node_addr=AquaSimAddress(-1);
    short_latency_table[i].num=0;
    short_latency_table[i].last_update_time=0.0;

    period_table[i].node_addr=AquaSimAddress(-1);
    period_table[i].difference=0.0;
    period_table[i].last_update_time=0.0;
  }

  m_arrivalTableIndex=0;
  for(int i=0;i<R_TABLE_SIZE;i++)
    arrival_table[i].node_addr=AquaSimAddress(-1);

  m_theta=m_transmissionTimeError/10.0;
  m_maxShortPacketTransmissiontime=((1.0*m_shortPacketSize*m_encodingEfficiency
                      +m_phyOverhead)/m_bitRate)*(1+m_transmissionTimeError);
  m_maxLargePacketTransmissiontime=((1.0*m_largePacketSize*m_encodingEfficiency
                      +m_phyOverhead)/m_bitRate)*(1+m_transmissionTimeError);

  InitPhaseOne(m_NDwindow,m_ackNDwindow, m_phaseOneWindow);
}

AquaSimRMac::~AquaSimRMac()
{
}

TypeId
AquaSimRMac::GetTypeId()
{
  static TypeId tid = TypeId("ns3::AquaSimRMac")
    .SetParent<AquaSimMac>()
    .AddConstructor<AquaSimRMac>()
    .AddAttribute ("BitRate", "Bit rate of MAC layer.",
      DoubleValue(1.0e4),
      MakeDoubleAccessor(&AquaSimRMac::m_bitRate),
      MakeDoubleChecker<double>())
    .AddAttribute ("EncodingEfficiency", "Ratio of encoding",
      DoubleValue(1),
      MakeDoubleAccessor(&AquaSimRMac::m_encodingEfficiency),
      MakeDoubleChecker<double>())
    .AddAttribute ("NDWindow", "Window to send ND",
      DoubleValue(1),
      MakeDoubleAccessor(&AquaSimRMac::m_NDwindow),
      MakeDoubleChecker<double>())
    .AddAttribute ("ACKNDWindow", "Window to send ACK_ND",
      DoubleValue(1),
      MakeDoubleAccessor(&AquaSimRMac::m_ackNDwindow),
      MakeDoubleChecker<double>())
    .AddAttribute ("PhaseOneWindow", "Time for latency detection",
      DoubleValue(3),
      MakeDoubleAccessor(&AquaSimRMac::m_phaseOneWindow),
      MakeDoubleChecker<double>())
    .AddAttribute ("PhaseTwoWindow", "Time for SYN announcement",
      DoubleValue(1),
      MakeDoubleAccessor(&AquaSimRMac::m_phaseTwoWindow),
      MakeDoubleChecker<double>())
    .AddAttribute ("PhaseTwoInterval", "Interval between windows of phase two",
      DoubleValue(0.5),
      MakeDoubleAccessor(&AquaSimRMac::m_phaseTwoInterval),
      MakeDoubleChecker<double>())
    .AddAttribute ("IntervalPhase2Phase3", "Interval between windows of phase 2 and 3",
      DoubleValue(1),
      MakeDoubleAccessor(&AquaSimRMac::m_intervalPhase2Phase3),
      MakeDoubleChecker<double>())
    .AddAttribute ("Duration", "Duration of duty cycle",
      DoubleValue(0.1),
      MakeDoubleAccessor(&AquaSimRMac::m_duration),
      MakeDoubleChecker<double>())
    .AddAttribute ("PhyOverhead", "The overhead caused by Phy layer.",
      IntegerValue(8),
      MakeIntegerAccessor(&AquaSimRMac::m_phyOverhead),
      MakeIntegerChecker<int>())
    .AddAttribute ("LargePacketSize", "Size of a large packet (bits)",
      IntegerValue(480),
      MakeIntegerAccessor(&AquaSimRMac::m_largePacketSize),
      MakeIntegerChecker<int>())
    .AddAttribute ("ShortPacketSize", "Size of a short packet (bits)",
      IntegerValue(40),
      MakeIntegerAccessor(&AquaSimRMac::m_shortPacketSize),
      MakeIntegerChecker<int>())
    .AddAttribute ("PhaseOneCycle", "Number of cycles in phase one",
      IntegerValue(4),
      MakeIntegerAccessor(&AquaSimRMac::m_phaseOneCycle),
      MakeIntegerChecker<int>())
    .AddAttribute ("PhaseTwoCycle", "Number of cycles in phase two",
      IntegerValue(2),
      MakeIntegerAccessor(&AquaSimRMac::m_phaseTwoCycle),
      MakeIntegerChecker<int>())
    .AddAttribute ("TransmissionTimeError", "Guardian Time",
      DoubleValue(0.0001),
      MakeDoubleAccessor(&AquaSimRMac::m_transmissionTimeError),
      MakeDoubleChecker<double>())
  ;
  return tid;
}

void
AquaSimRMac::InitPhaseOne(double t1,double t2, double t3)
{
  NS_LOG_FUNCTION("PhaseOne cycle:" << m_phaseOneCycle);

  if(m_phaseOneCycle)
    {
      m_phaseStatus=PHASEONE;

      InitND(t1,t2,t3);
      m_phaseOneEvent = Simulator::Schedule(Seconds(t3),&AquaSimRMac::InitPhaseOne, this, t1, t2, t3);
      m_phaseOneCycle--;
      return;
    }

   // PrintTable();
   InitPhaseTwo();
   return;
}




void
AquaSimRMac::TxACKRev(Ptr<Packet> pkt){

  NS_LOG_FUNCTION(this << pkt);
  NS_LOG_INFO("AquaSimRMac Txackrev: node:" << m_device->GetAddress() <<
	      " at time:" << Seconds(Simulator::Now()));
  DeleteBufferCell(pkt);

  AquaSimHeader asHeader;
  RMacHeader rHeader;
  pkt->RemoveHeader(asHeader);
  pkt->PeekHeader(rHeader);

  //assert(initialized());
  //UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;

  Time totalTxTime = Seconds( ((asHeader.GetSerializedSize() + rHeader.GetSerializedSize()) * m_encodingEfficiency
			+ m_phyOverhead) / m_bitRate );
  asHeader.SetTxTime(totalTxTime);

  NS_LOG_INFO("AquaSimRMac Txackrev: node:" << m_device->GetAddress() <<
	      " is transmitting a packet st=" << rHeader.GetSt() <<
	      " txtime=" << totalTxTime << " at time " << Seconds(Simulator::Now()));

  if(m_device->TransmissionStatus() == SLEEP)
    {
      PowerOn();
      m_device->SetTransmissionStatus(SEND);
      asHeader.SetTimeStamp(Seconds(Simulator::Now()));
      pkt->AddHeader(asHeader);
      SendDown(pkt);
  // check if the sending this ACKRev collides with my own ackrev windows
  if(IsACKREVWindowCovered(Simulator::Now().GetDouble()))
    {
      NS_LOG_DEBUG("AquaSimRMac Txackrev: node:" << m_device->GetAddress() <<
		   " converged with ACKwindow");
      InsertReservedTimeTable(rHeader.GetSenderAddr(),m_periodInterval,(4*m_periodInterval));
    }
  m_device->SetTransmissionStatus(SLEEP);
  Simulator::Schedule(totalTxTime, &AquaSimRMac::StatusProcess, this, m_device->TransmissionStatus());
  return;
  }

  if(NIDLE==m_device->TransmissionStatus())
    {
      m_device->SetTransmissionStatus(SEND);
      asHeader.SetTimeStamp(Seconds(Simulator::Now()));
      pkt->AddHeader(asHeader);
      SendDown(pkt);
  //  printf("broadcast %d Tx Idle set timer at %f tx is %f\n",node_->nodeid(),NOW,txtime);

  // check if the sending this ACKRev collides with my own ackrev windows
  if(IsACKREVWindowCovered(Simulator::Now().GetDouble()))
    {
      NS_LOG_DEBUG("AquaSimRMac Txackrev: node:" << m_device->GetAddress() <<
		   " converged with ACKwindow");
      InsertReservedTimeTable(rHeader.GetSenderAddr(),m_periodInterval,(4*m_periodInterval));
    }
  m_device->SetTransmissionStatus(NIDLE);
  Simulator::Schedule(totalTxTime, &AquaSimRMac::StatusProcess, this, m_device->TransmissionStatus());
  return;
  }

  if(RECV==m_device->TransmissionStatus())
    {
      InterruptRecv(totalTxTime.GetDouble());
      asHeader.SetTimeStamp(Seconds(Simulator::Now()));
      pkt->AddHeader(asHeader);
      SendDown(pkt);

      if(IsACKREVWindowCovered(Simulator::Now().GetDouble()))
	{
	  NS_LOG_DEBUG("AquaSimRMac Txackrev: node:" << m_device->GetAddress() <<
	  		   " converged with ACKwindow");
	  InsertReservedTimeTable(rHeader.GetSenderAddr(),m_periodInterval,(4*m_periodInterval));
      }
      m_device->SetTransmissionStatus(NIDLE);
      Simulator::Schedule(totalTxTime, &AquaSimRMac::StatusProcess, this, m_device->TransmissionStatus());
     return;
    }

if (SEND==m_device->TransmissionStatus())
  {
    NS_LOG_INFO("AquaSimRMac: queue send data too fast");
    //Packet::free(pkt);	//smart pointer can handle this
    pkt = 0;
    return;
  }
}

bool
AquaSimRMac::IsACKREVWindowCovered(double current_time)
{
  if((current_time-m_cycleStartTime==m_periodInterval)
     ||(current_time-m_cycleStartTime==0)) return true;
  else return false;
}

void
AquaSimRMac:: InsertReservedTimeTable(AquaSimAddress sender_addr, double start_time, double dt)
{
  NS_LOG_FUNCTION(m_device->GetAddress() << Seconds(Simulator::Now()));

  if(m_reservedTimeTableIndex>=R_TABLE_SIZE)
    {
      NS_LOG_DEBUG("AquaSimRMac:InsertReservedTimeTable: the reservedTimeTable is full");
      return;
    }
  int index=-1;
  for(int i=0;i<m_reservedTimeTableIndex;i++)
    {
      if(reserved_time_table[i].node_addr==sender_addr) index=i;
    }
  if(index==-1)
    {
      reserved_time_table[m_reservedTimeTableIndex].node_addr=sender_addr;
      reserved_time_table[m_reservedTimeTableIndex].start_time=start_time;
      reserved_time_table[m_reservedTimeTableIndex].duration=dt;
      m_reservedTimeTableIndex++;
    }
  else
    {
      reserved_time_table[index].node_addr=sender_addr;
      reserved_time_table[index].start_time=start_time;
      reserved_time_table[index].duration=dt;
    }
}


void
AquaSimRMac::DeleteBufferCell(Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this);
  buffer_cell* t1;
  buffer_cell* t2;
  t1=ack_rev_pt;

  if(!t1)
    {
      NS_LOG_INFO("AquaSimRMac: there is no ackRev link");
      return;
    }

  if(t1->next) t2=t1->next;


  if(t1->packet==p)
    {
      ack_rev_pt=ack_rev_pt->next;
      delete t1;
      return;
    }

  /*
  if(t2){

    Packet* t=t2->packet;
    hdr_ack_rev*  th=HDR_ACK_REV(t);
   printf("Rmac:node %d  !!!!sender_addr=%d\n", index_, th->sender_addr);
  }
  */

  while(t2)
    {
      if(p==t2->packet)
	{
	  t1->next=t2->next;
	  delete t2;
	  return;
	}
      t1=t2;
      t2=t2->next;
    }

  return;
}


/*

void
AquaSimRMac::InitPhaseThree(){


  printf("RMac: this is InitPhaseThree\n");

   SortPeriodTable();
   PrintTable();

  UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
  if(n->TransmissionStatus()==SLEEP) Poweron();

  mac_status=RMAC_IDLE;
   m_sleepEvent = Simulator::Schedule(Seconds(m_duration), &AquaSimRMac::ProcessSleep, this);

    return;
}
*/


void
AquaSimRMac::InitPhaseThree()
{
  NS_LOG_FUNCTION(this);

  SortPeriodTable(period_table);
  PrintTable();

  m_macStatus=RMAC_IDLE;
  Wakeup();
  return;
}


void
AquaSimRMac::PrintTable()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());

  for (int i=0;i<R_TABLE_SIZE;i++)
    {
      NS_LOG_DEBUG("PrintTable(ShortLatency) Node Addr:" << short_latency_table[i].node_addr <<
		   " and short latency:" << short_latency_table[i].latency);
    }

  for (int i=0;i<R_TABLE_SIZE;i++)
    {
      NS_LOG_DEBUG("PrintTable(PeriodTable) Node Addr:" << period_table[i].node_addr <<
		   " and difference:" << period_table[i].difference);
    }
}


/*
void
AquaSimRMac::SortPeriodTable()
{
  printf("RMac:SortPeriodTable;node %d sort period table \n",index_);
  bool unswapped=false;
  int i=0;
  int j=0;

  while ((!unswapped)&&(i<period_table_index-1))
    {
      j=0;
      unswapped=true;
      while (j<period_table_index-1-i)
	{
       if(period_table[j].difference>period_table[j+1].difference)
	{
	  // printf("sortperiodtable;node %d swictch two values %f and %f \n",
	  // index_,period_table[j].difference, period_table[j+1].difference);
	  double dt=period_table[j].difference;
          AquaSimAddress addr=period_table[i].node_addr;
          double du=period_table[i].duration;
          double lut=period_table[i].last_update_time;

          period_table[j].difference=period_table[j+1].difference;
          period_table[j].node_addr=period_table[j+1].node_addr;
          period_table[j].duration=period_table[j+1].duration;
          period_table[j].last_update_time=period_table[j+1].last_update_time;

	  period_table[j+1].difference=dt;
          period_table[j+1].node_addr=addr;
          period_table[j+1].duration=du;
          period_table[j+1].last_update_time=lut;
          unswapped=false;
	}
       j++;
	}
      i++;
    }
}

*/






// use bubble algorithm to sort the period table

void
AquaSimRMac::SortPeriodTable(struct period_record * table)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());
  bool unswapped=false;
  int i=0;
  int j=0;

  while ((!unswapped)&&(i<m_periodTableIndex-1))
    {
      j=0;
      unswapped=true;
      while (j<m_periodTableIndex-1-i)
	{
       if(table[j].difference>table[j+1].difference)
	{
	  // printf("sortperiodtable;node %d swictch two values %f and %f \n",
	  // index_,period_table[j].difference, period_table[j+1].difference);
	  double dt=table[j].difference;
          AquaSimAddress addr=table[j].node_addr;
          double du=table[j].duration;
          double lut=table[j].last_update_time;

          table[j].difference=table[j+1].difference;
          table[j].node_addr=table[j+1].node_addr;
          table[j].duration=table[j+1].duration;
          table[j].last_update_time=table[j+1].last_update_time;

	  table[j+1].difference=dt;
          table[j+1].node_addr=addr;
          table[j+1].duration=du;
          table[j+1].last_update_time=lut;
          unswapped=false;
	}
       j++;
	}
      i++;
    }
}


void
AquaSimRMac::ProcessSleep(){
  NS_LOG_INFO("AquaSimRMac::ProcessSleep: Node:" << m_device->GetAddress() <<
	      " is ProcessSleep at time:" << Seconds(Simulator::Now()) <<
	      " and wake up after " << m_periodInterval << " - " << m_duration);

  if(m_macStatus==RMAC_RECV) return;

  PowerOff(); //? Is it safe to poweroff

  if((m_macStatus==RMAC_IDLE)&&(m_reservationTableIndex!=0))
   {
     if(!m_collectRev) m_collectRev=true;
     else
       {
	 NS_LOG_INFO("AquaSimRMac: Node:" << m_device->GetAddress() <<
		     " ProcessSleep reservation table is not empty(" <<
		     m_reservationTableIndex << ")");
	 // m_macStatus=RMAC_ACKREV;
	 ArrangeReservation();
       }
   }
   return;
}


void
AquaSimRMac::InsertBackoff(AquaSimAddress sender_addr)
{
  // int indx=-1;
  double elapsed_time=Simulator::Now().GetDouble()-m_cycleStartTime;
  double start_time=elapsed_time+m_periodInterval;
  // adding m_periodInterval just to make sure that subsequent process correct
  double dt=4*m_periodInterval;
  InsertReservedTimeTable(sender_addr,start_time,dt);
}

void
AquaSimRMac::CancelREVtimeout()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());
  Simulator::Cancel(m_timeoutEvent);
}


void
AquaSimRMac::ClearChannel()
{
  NS_LOG_INFO("AquaSimRMac:ClearChannel Node:" << m_device->GetAddress() <<
	      " at time:" << Seconds(Simulator::Now()));
  if(NewData())
    {
      if(m_macStatus==RMAC_FORBIDDED)
	{// avoid overlap
	  MakeReservation();
	  m_macStatus=RMAC_REV;
	}
    }
  else m_macStatus=RMAC_IDLE;
}


void
AquaSimRMac::CancelReservation()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());

  for (int i=0;i<R_TABLE_SIZE;i++)
    {
      reservation_table[i].node_addr=AquaSimAddress(-1);
    }
}


void
AquaSimRMac::StartRECV(double dt, int id, AquaSimAddress data_sender)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << Seconds(Simulator::Now()));
  if(id==0)
    {
      //   data_sender=-12;

      /*
      for (int i=0;i<MAXIMUM_BUFFER;i++) bit_map[i]=0;
      printf("rmac:StartRECV: node %d at %f to power on\n",index_,NOW);
      */

      PowerOn();
      m_recvBusy=false;

      m_macStatus=RMAC_RECV;
      m_recvStatus=1;

      double t=2*m_maxLargePacketTransmissiontime;

      m_timeoutEvent = Simulator::Schedule(Seconds(t), &AquaSimRMac::ResetMacStatus, this);
      m_macRecvEvent = Simulator::Schedule(Seconds(dt), &AquaSimRMac::StartRECV, this, m_recvDuration, m_recvStatus, m_recvDataSender);
    }
  else
    {
      // modification for the version02-10-2006 here, the RECV status ends when
      // the receiver receives the data packets.
      // m_macStatus=RMAC_IDLE;
      NS_LOG_INFO("AquaSimRMac:StartRECV: Node:" << m_device->GetAddress() <<
		  " at time:" << Seconds(Simulator::Now()) << " to power off");
      ScheduleACKData(data_sender);
      PowerOff();
    }
}

void
AquaSimRMac::ArrangeReservation()
{
  NS_LOG_INFO("AquaSimRMac:ArrangeReservation: Node:" << m_device->GetAddress() <<
	      " at time:" << Seconds(Simulator::Now()));
  int sender_index=-1;
  if(ProcessRetransmission())
    {
      NS_LOG_INFO("AquaSimRMac:ArrangeReservation: Node:" << m_device->GetAddress() <<
    	      " handle retransmission this time!!");
      return;
    }

  sender_index=SelectReservation();
  if(sender_index==-1)
    {
      NS_LOG_INFO("AquaSimRMac:ArrangeReservation: Node:" << m_device->GetAddress() <<
    	      " no reservation selected!!");
      return;
    }
  else
    {
      m_macStatus=RMAC_ACKREV;

      AquaSimAddress sender=reservation_table[sender_index].node_addr;
      double dt=reservation_table[sender_index].required_time;
      double offset=reservation_table[sender_index].interval;

      NS_LOG_INFO("AquaSimRMac:ArrangeReservation: Sender:" << sender <<
    	      " and duration:" << dt << " is scheduled");
      ScheduleACKREV(sender,dt,offset);
  }
}


//the receiver is the address of the data sender
void
AquaSimRMac::ScheduleACKREV(AquaSimAddress receiver, double duration, double offset)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());
  int i=0;
  //  double Number_Period=0;
  double last_time=0.0;
  double upper_bound=0;
  double elapsed_time=Simulator::Now().GetDouble()-m_cycleStartTime;

  AquaSimAddress receiver_addr=receiver;
  double dt=CheckDifference(period_table,receiver);
  double latency=CheckLatency(short_latency_table,receiver_addr) - m_maxShortPacketTransmissiontime;


  NS_LOG_INFO("AquaSimRMac:ScheduleACKRev: Node:" << m_device->GetAddress() <<
	      " is scheduling ackrev, duration:" << duration <<
	      ", interval:" << offset);
  while ((period_table[i].node_addr!=-1)&&(i<R_TABLE_SIZE))
    {
      if (period_table[i].node_addr!=receiver)
	{
	  AquaSimAddress nid=period_table[i].node_addr;
	  double d1=CheckDifference(period_table,nid);
	  double l1=CheckLatency(short_latency_table,nid)-m_maxShortPacketTransmissiontime;
	  double t1=CalculateACKRevTime(d1,l1,elapsed_time);

	  Ptr<Packet> ackrev=GenerateACKRev(nid,receiver,duration);

	  InsertACKRevLink(ackrev,&t1);
	  Simulator::Schedule(Seconds(t1), &AquaSimRMac::TxACKRev, this, ackrev);
	  NS_LOG_INFO("AquaSimRMac:ScheduleACKRev: Node:" << m_device->GetAddress() << " and node:" <<
		      nid << " t1:" << t1 << " at current time:" << Seconds(Simulator::Now()));

	  //   if (Number_Period<1) Number_Period=1;
	  if(t1+2*l1>last_time) last_time=t1+2*l1;
	  if(t1+l1>upper_bound) upper_bound=t1+l1;
	}
	i++;
    } // end of all the neighbors
    //      double l=offset;
    // int receiver_addr=receiver;
    // double dt=CheckDifference(period_table,receiver);

    //  double latency=CheckLatency(short_latency_table,receiver_addr)
    //          -max_short_packet_transmissiontime;
    double t1=CalculateACKRevTime(dt,latency,elapsed_time);

    while(t1<upper_bound) t1+=m_periodInterval;
    double t3=t1+2*latency+m_maxShortPacketTransmissiontime;

    Ptr<Packet> ackrevPacket=GenerateACKRev(receiver,receiver,duration);
    InsertACKRevLink(ackrevPacket,&t1);

    Simulator::Schedule(Seconds(t1), &AquaSimRMac::TxACKRev, this, ackrevPacket);
    NS_LOG_INFO("AquaSimRMac:ScheduleACKRev: Node:" << m_device->GetAddress() << " and node:" <<
		      receiver << " t1:" << t1 << " t3:" << t3 << " latency:" << latency <<
		      " at current time:" << Seconds(Simulator::Now()));
    // decide the start time of reservation duration

    if(t3<last_time)t3=last_time;

    double st=dt+offset-elapsed_time+latency;
    while(t3>st) st+=m_periodInterval;  // start time of reserved duration

    double end_time=st+duration;  // end time of reserved duration

    /* this is modified from the version 02-10-2006 */
    double available_period=m_periodInterval+dt-elapsed_time;
    double end_period=m_periodInterval-elapsed_time;

    // the earliest time period that is available for new request
    while(available_period<(end_time+latency))available_period+=m_periodInterval;

    available_period=available_period + m_duration - latency-m_maxShortPacketTransmissiontime;

    while (end_period<available_period) end_period+=m_periodInterval;

    // SetStartTime(ack_rev_pt,st,available_period);
    SetStartTime(ack_rev_pt,st,end_period);
    // SetStartTime(ack_rev_pt,st,end_time);
    NS_LOG_INFO("AquaSimRMac:ScheduleACKRev: Node:" << m_device->GetAddress() <<
		" offset time:" << st << " and set end time:" << end_period <<
		" at current time:" << Seconds(Simulator::Now()));
    m_recvStatus=0;

    // 2 times m_maxLargePacketTransmissiontime is enough for ackdata
    // m_recvDuration=(duration+2*m_maxLargePacketTransmissiontime);

    // this is time for ackdata
    m_recvDuration=duration;
    m_recvDataSender=receiver;
    m_macRecvEvent = Simulator::Schedule(Seconds(st), &AquaSimRMac::StartRECV, this, m_recvDuration, m_recvStatus, m_recvDataSender);
}



// this function first check if the arranged slot collides with
// the slot for the intended receiver if so, adjust its sending
// time to avoid interference the intended receiver, however,
// interfere with the receiver.

double
AquaSimRMac::CalculateACKRevTime(double diff1, double l1,double diff2,double l2)
{
  bool collision_status=false;
  double elapsed_time=Simulator::Now().GetDouble()-m_cycleStartTime;
  double s1=diff1-l1;
  while (s1<0) s1+=m_periodInterval;
  double s2=diff2-l2;
  while (s2<0) s2+=m_periodInterval;

  double delta=s1-s2;
  if(((s1<=s2)&&(s2<=s1+m_maxShortPacketTransmissiontime))
     || ((s2<=s1)&&(s1<=s2+m_maxShortPacketTransmissiontime)))
    collision_status=true;

  if(collision_status)
    {
      NS_LOG_DEBUG("AquaSimRMac:CalculateACKRev: collision! delta:" << delta);
      delta=s1-s2;
    }
  else delta=0;

  double offset_time=diff2+delta;
  while (elapsed_time+l2>offset_time) offset_time+=m_periodInterval;

  return offset_time-l2-elapsed_time;
}


double
AquaSimRMac::CalculateACKRevTime(double diff,double latency, double elapsed_time)
{
  double offset_time=diff;
  while (elapsed_time+latency>offset_time) offset_time+=m_periodInterval;

  return offset_time-latency-elapsed_time;
}


Ptr<Packet>
AquaSimRMac::GenerateACKRev(AquaSimAddress receiver, AquaSimAddress intended_receiver, double duration)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());

  Ptr<Packet> pkt = Create<Packet>();
  AquaSimHeader asHeader;
  RMacHeader rHeader;
  AquaSimPtTag ptag;

  asHeader.SetNextHop(receiver);
  asHeader.SetDirection(AquaSimHeader::DOWN);
  // addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_RMAC);
  // size()=short_packet_size_;

  rHeader.SetPtype(P_ACKREV);
  rHeader.SetPktNum(m_numSend);
  rHeader.SetRecvAddr(intended_receiver);
  rHeader.SetDuration(duration);
  rHeader.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );

  pkt->AddHeader(asHeader);
  pkt->AddHeader(rHeader);
  pkt->AddPacketTag(ptag);
  m_numSend++;
  return pkt;
}


// In old version of this program, the silence duration is not  changed
// however, in new version of this program, the silence duration is also changed

void
AquaSimRMac::SetStartTime(buffer_cell* ack_rev_pt, double st, double next_period)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());
  buffer_cell* t1;
  t1=ack_rev_pt;
  while(t1)
    {
      RMacHeader rHeader;
      (t1->packet)->RemoveHeader(rHeader);
      double d=t1->delay;
      rHeader.SetSt(st-d);
      rHeader.SetInterval(next_period-d);
      rHeader.SetDuration(next_period-d);
      NS_LOG_INFO("AquaSimRMac Setstarttime: Node:" << m_device->GetAddress() <<
		  " offset time is:" << rHeader.GetSt() << " and next period is:" <<
		  rHeader.GetInterval());
      t1=t1->next;
    }
}


/* // old version of SetStartTime
void
RMac::SetStartTime(buffer_cell* ack_rev_pt, double st, double next_period){
 printf("rmac setstarttime: node %d \n",index_);
  buffer_cell* t1;
  t1=ack_rev_pt;
  while(t1){
    hdr_rmac*  ackrevh=HDR_RMAC(t1->packet);
    double d=t1->delay;
    ackrevh->st=st-d;
    ackrevh->interval=next_period-d;
 printf("rmac setstarttime: node %d interval to recv =%f and next period is %f\n",index_,ackrevh->st,ackrevh->interval);
    t1=t1->next;
  }
}
*/

void
AquaSimRMac::InsertACKRevLink(Ptr<Packet> p, double d)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());
  buffer_cell* t1=new buffer_cell;
  t1->packet=p;
  t1->delay=d;
  t1->next=NULL;

  if(ack_rev_pt==NULL)
    {
      ack_rev_pt=t1;
      NS_LOG_INFO("Node:" << m_device->GetAddress() << " ackrev link is empty");
      return;
    }
  else
    {
      buffer_cell* t2=ack_rev_pt;
      ack_rev_pt=t1;
      t1->next=t2;
      NS_LOG_INFO("Node:" << m_device->GetAddress() << " ackrev link is empty");
      return;
    }
}

void
AquaSimRMac::InsertACKRevLink(Ptr<Packet> p, double* d)
{
  double s1=*d;
  double win=m_maxShortPacketTransmissiontime;

  NS_LOG_FUNCTION(this << m_device->GetAddress());
  buffer_cell* t1=new buffer_cell;
  t1->packet=p;
  t1->delay=s1;
  t1->next=NULL;

  if(ack_rev_pt==NULL)
    {
      ack_rev_pt=t1;
      NS_LOG_INFO("Node:" << m_device->GetAddress() << " ackrev link is empty");
      return;
    }
  else
    {
      buffer_cell* t2=ack_rev_pt;
      buffer_cell* tmp;
      NS_LOG_INFO("Node:" << m_device->GetAddress() << " ackrev link is empty");

      while(t2)
	{
	  tmp=t2;
	  double s2=t2->delay;
	  if(((s1<=s2)&&(s2<=s1+win))|| ((s2<=s1)&&(s1<=s2+win)))
	    {
	      NS_LOG_INFO("InsertACKrev: Node:" << m_device->GetAddress() << " finds collisions!");
	      s1+=m_periodInterval;
	    }
	  t2=t2->next;
	}

      t1->delay=s1;
      tmp->next=t1;
      *d=s1;
      return;
    }
}

void
AquaSimRMac::ResetReservationTable()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());
  for(int i=0;i<R_TABLE_SIZE;i++)
    {
      reservation_table[i].node_addr=AquaSimAddress(-1);
      reservation_table[i].required_time=0.0;
    }
}

// returned true if there exist retransmission request, false otherwise
bool
AquaSimRMac::ProcessRetransmission()
{
  bool status=false;
  int i=0;
  while (i<m_reservationTableIndex)
    {
      if(IsRetransmission(i))
	{
	  status=true;
	  ScheduleACKData(reservation_table[i].node_addr);
	  ClearReservationTable(i); // delete the record from table
	  i--;
	}
      i++;
    }
  return status;
}

void
AquaSimRMac::ClearReservationTable(int index)
{
  for(int i=index;i<m_reservationTableIndex-1;i++)
    {
      reservation_table[i].node_addr=reservation_table[i+1].node_addr;
      reservation_table[i].block_id=reservation_table[i+1].block_id;
      reservation_table[i].required_time=reservation_table[i+1].required_time;
      reservation_table[i].interval=reservation_table[i+1].interval;
    }
  m_reservationTableIndex--;
}

bool
AquaSimRMac::IsRetransmission(int reservation_index)
{
  int block=reservation_table[reservation_index].block_id;
  AquaSimAddress node_addr=reservation_table[reservation_index].node_addr;

  for(int i=0;i<m_ackDataTableIndex;i++)
    if((ackdata_table[i].node_addr==node_addr)&&(ackdata_table[i].block_num==block))
      {
	NS_LOG_INFO("AquaSimRMac:IsRetransmission: Node:" << m_device->GetAddress() <<
		    " received a retx from node:" << node_addr);
	return true;
      }
  return false;
}

int
AquaSimRMac::SelectReservation()
{

  /* this one favor the long queue
  printf("rmac:selectReservation: node %d\n",index_);
  int index=-1;
  double dt=-1.0;
  int i=0;

  while(!(reservation_table[i].node_addr==-1))
   {
     printf("rmac:select reservation: node %d, request id is%d \n",index_,reservation_table[i].node_addr);
    if (reservation_table[i].required_time>dt) index=i;
    i++;
   }

  return index;
  */

  if(0==m_reservationTableIndex) return -1; // no new reservation request
  // if(skip){
  // if(rand()%2==0) return -1;
    // }
  //  if(rand()%2==0) return -1;
  int i=0;
  while(!(reservation_table[i].node_addr.GetAsInt()==-1))
    {
      NS_LOG_INFO("AquaSimRMac:SelectReservation: Node:" << m_device->GetAddress() <<
		  " request id is " << reservation_table[i].node_addr << " i:" << i);
      i++;
    }
  //  printf("rmac:select reservation  node %d i=%d\n",index_,i);
  return rand()%i;
}

void
AquaSimRMac::ResetMacStatus(){
  NS_LOG_FUNCTION(this << m_device->GetAddress() << Seconds(Simulator::Now()));

  if((m_macStatus==RMAC_WAIT_ACKREV)||(m_macStatus==RMAC_FORBIDDED))
    {
      m_txBuffer.UnlockBuffer();
      //ResumeTxProcess();
      NS_LOG_INFO("AquaSimRMac:ResetMacStatus: Node:" << m_device->GetAddress() << " unlock txbuffer");
    }

  if(m_macStatus==RMAC_RECV)
    {
      NS_LOG_INFO("AquaSimRMac:ResetMacStatus: Node:" << m_device->GetAddress() <<
		  " don't receive the data packet at time:" << Seconds(Simulator::Now()));
      Simulator::Cancel(m_macRecvEvent);
      PowerOff();
    }
  m_macStatus=RMAC_IDLE;
}

void
AquaSimRMac::Wakeup()
{
  NS_LOG_FUNCTION(this);
  NS_LOG_DEBUG("AquaSimRMac:WakeUp: Node:" << m_device->GetAddress() << " wake up at time:" <<
	       Seconds(Simulator::Now()) << " and the packet number is " << m_txBuffer.num_of_packet);

  // reset the carrier sense
  m_device->ResetCarrierSense();
  m_carrierSense=false;

  /*
  if(n->CarrierSense()){
    printf("Rmac: node %d sense the busy channel at %f\n",index_,NOW);
   carrier_sense=true;
  }
  else {
     carrier_sense=false;
     n->ResetCarrierId();
  }
  */
  //skip=!skip;

  PowerOn();
  m_cycleStartTime=Simulator::Now().GetDouble();

  /*
  for(int i=0;i<R_TABLE_SIZE;i++){
    reservation_table[i].node_addr=AquaSimAddress(-1);
  }
  reservation_table_index=0;
  */

   // one ack windows:rev
   double ACKwindow=m_maxShortPacketTransmissiontime;

   NS_LOG_DEBUG("AquaSimRMac:WakeUp: Node:" << m_device->GetAddress() << " schedule sleep after:" <<
   	       m_duration << " at " << Seconds(Simulator::Now()));

   m_sleepEvent = Simulator::Schedule(Seconds(m_duration), &AquaSimRMac::ProcessSleep, this);
   m_wakeupEvent = Simulator::Schedule(Seconds(m_periodInterval), &AquaSimRMac::Wakeup, this);
   //m_carrierSenseEvent = Simulator::Schedule(Seconds(ACKwindow), &AquaSimRMac::ProcessCarrier, this);
   m_ackWindowEvent = Simulator::Schedule(Seconds(1.5*ACKwindow), &AquaSimRMac::ProcessListen, this);
   return;
}

void
AquaSimRMac::ResetReservation()
{
  for(int i=0;i<R_TABLE_SIZE;i++)
    {
      reservation_table[i].node_addr=AquaSimAddress(-1);
    }

  m_reservationTableIndex=0;
}

void
AquaSimRMac::ProcessCarrier()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << Seconds(Simulator::Now()));

  if(m_device->CarrierId())
    {
      NS_LOG_INFO("AquaSimRMac: Node:" << m_device->GetAddress() << " senses carrier!!");
      m_carrierSense=true;
    }
  else m_carrierSense=false;
  m_device->ResetCarrierId();
}


void
AquaSimRMac::ProcessListen()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << Seconds(Simulator::Now()));
  if(m_carrierSense)
    {
      NS_LOG_DEBUG("AquaSimRMac: Node:" << m_device->GetAddress() <<
		   " senses cariers at time:" << Seconds(Simulator::Now()));
      // InsertBackoff();
      m_carrierSense=false;
    }
  ProcessReservedTimeTable();

  switch (m_macStatus)
  {
    case RMAC_IDLE:
      if (NewData()&&(!m_collectRev))
	{
	  NS_LOG_INFO("WakeUp: There is new data in node " << m_device->GetAddress() <<
		      " and the packet number is " << m_txBuffer.num_of_packet);
	  m_macStatus=RMAC_REV;
	  MakeReservation();
	}
      break;
    case RMAC_FORBIDDED:
      NS_LOG_INFO("WakeUp NODE " << m_device->GetAddress() << " is in state RMAC_FORBIDDED");
      CancelReservation();
      CancelREVtimeout();
      ClearACKRevLink();
      m_collectRev=false;
      break;
    case RMAC_WAIT_ACKREV:
      m_collectRev=false;
      NS_LOG_INFO("WakeUp NODE " << m_device->GetAddress() << " is in state RMAC_WAIT_ACKREV");
      break;
    case RMAC_RECV:
      m_collectRev=false;
      NS_LOG_INFO("WakeUp NODE " << m_device->GetAddress() << " is in state RMAC_RECV");
      break;
    case RMAC_TRANSMISSION:
      m_collectRev=false;
      NS_LOG_INFO("WakeUp NODE " << m_device->GetAddress() << " is in state RMAC_TRANSMISSION");
      break;
    case RMAC_REV:
      m_collectRev=false;
      NS_LOG_INFO("WakeUp NODE " << m_device->GetAddress() << " is in state RMAC_REV");

      break;
    case RMAC_ACKREV:
      m_collectRev=false;
      NS_LOG_INFO("WakeUp NODE " << m_device->GetAddress() << " is in state RMAC_ACKREV");
      break;
    case RMAC_WAIT_ACKDATA:
      m_collectRev=false;
      NS_LOG_INFO("WakeUp NODE " << m_device->GetAddress() << " is in state RMAC_WAIT_ACKDATA");
      break;
    default:
      m_collectRev=false;
      NS_LOG_INFO("WakeUp node " << m_device->GetAddress() << " don't expect to be in this state");
      break;
  }

  if(!m_collectRev) ResetReservation();
  return;
}

/*
void
AquaSimRMac::ProcessCarrier()
{
  printf("RMac:node %d processes carrier sense at %f...\n",index_,NOW);
  UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
  if(n->CarrierSense())
     {
      InsertBackoff();
      n->ResetCarrierSense();
     }
}
*/

void
AquaSimRMac::ClearACKRevLink()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());
  if(!ack_rev_pt) return;
  buffer_cell* t1;
  buffer_cell* t2;

  // t1=ack_rev_pt->next;
  t1=ack_rev_pt;
  while (t1){
    t2=t1->next;
    t1->packet = 0;
    delete t1;
    t1=t2;
    ack_rev_pt=t1;
  }
}


void
AquaSimRMac::ProcessReservedTimeTable()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << m_reservedTimeTableIndex);
  int i=0;
  //   double largest_duration=0;
  double elapsed_time=Simulator::Now().GetDouble()-m_cycleStartTime;

  while(i<m_reservedTimeTableIndex)
    {
      // printf("rmac:ProcessReservedtimetable: node %d index=%d\n",index_, reserved_time_table_index);
      double nst=reserved_time_table[i].start_time-m_periodInterval-elapsed_time;
      double lt=reserved_time_table[i].duration;
      AquaSimAddress addr=reserved_time_table[i].node_addr;
      double  l=CheckLatency(short_latency_table,addr);
      double t1=l-m_maxShortPacketTransmissiontime;
      nst=nst-t1;

      if (nst<0) {
	  if((lt+nst)<=0) {
	      DeleteRecord(i);
	      i--;
	  }
      else
	{ // nst>=
	  m_macStatus=RMAC_FORBIDDED;
	  NS_LOG_INFO("AquaSimRMac:ProcessReservedTimeTable: node:" << m_device->GetAddress() <<
		      " sets reserved time interval 0.0 and duration:" << (lt+nst));
	 reserved_time_table[i].start_time=elapsed_time;
	 reserved_time_table[i].duration=lt+nst;
	}
      }// end of nst<0
    else {
      // nst>0
      // if (nst<=PeriodInterval_) mac_status=RMAC_FORBIDDED;
	m_macStatus=RMAC_FORBIDDED;
	  NS_LOG_INFO("AquaSimRMac:ProcessReservedTimeTable: node:" << m_device->GetAddress() <<
		      " sets reserved time interval " << nst << " and duration:" << lt);
	reserved_time_table[i].start_time=nst;
	reserved_time_table[i].duration=lt;
      }
      i++;
    }

  if(m_macStatus==RMAC_FORBIDDED)
    {
      //what's this used for??
      //m_clearChannelEvent = Simulator::Schedule(Seconds(largest_duration), &AquaSimRMac::ClearChannel, this);
    }

  if((m_reservedTimeTableIndex==0)&&(m_macStatus==RMAC_FORBIDDED))
    m_macStatus=RMAC_IDLE;
}


void
AquaSimRMac::DeleteRecord(int index)
{
  for(int i=index;i<m_reservedTimeTableIndex;i++)
    {
      reserved_time_table[i].node_addr= reserved_time_table[i+1].node_addr;
      reserved_time_table[i].start_time= reserved_time_table[i+1].start_time;
      reserved_time_table[i].duration= reserved_time_table[i+1].duration;
      m_reservedTimeTableIndex--;
    }
  NS_LOG_FUNCTION(this << m_device->GetAddress() << m_reservedTimeTableIndex);
}

bool
AquaSimRMac::NewData()
{
  return (!m_txBuffer.IsEmpty());//!! this is correct??think about it
}

void
AquaSimRMac::MakeReservation()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << Seconds(Simulator::Now()));

  Ptr<Packet> p = m_txBuffer.head();
  AquaSimHeader asHeader;
  p->PeekHeader(asHeader);

  AquaSimAddress receiver_addr=asHeader.GetNextHop();

  m_txBuffer.LockBuffer();
  int num=m_txBuffer.num_of_packet;
  NS_LOG_INFO("AquaSimRMac:MakeReservation: Node " << m_device->GetAddress() << " lock txbuffer");

  //   AquaSimAddress sender_addr=index_;
  // double lt=-1.0;

  double dt=num*m_maxLargePacketTransmissiontime+(num-1)*m_SIF;
  double it=CalculateOffset(dt);
  double t2=DetermineSendingTime(receiver_addr);

  Ptr<Packet> pkt = Create<Packet>();
  RMacHeader rHeader;
  AquaSimPtTag ptag;

  asHeader.SetNextHop(receiver_addr);
  asHeader.SetDirection(AquaSimHeader::DOWN);
  // addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_RMAC);

  rHeader.SetPtype(P_REV);
  rHeader.SetBlockNum(m_numBlock);
  rHeader.SetPktNum(m_numSend);
  rHeader.SetDuration(dt);
  rHeader.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  rHeader.SetInterval(it);
  m_numSend++;

  pkt->AddHeader(asHeader);
  pkt->AddHeader(rHeader);
  pkt->AddPacketTag(ptag);

  NS_LOG_INFO("AquaSimRMac:MakeReservation: Node " << m_device->GetAddress() <<
	      " send a reservation to node " << receiver_addr << ", duration is " <<
	      rHeader.GetDuration() << " and offset is " << it << " after " << t2 <<
	      " at " << Seconds(Simulator::Now()));
  Simulator::Schedule(Seconds(t2), &AquaSimRMac::TxRev, this, pkt);
}


/*
In this function, we assume that the sleep time is long enough such that there
exist interval between two reserved slot (ACK slot) that is long for the max
data transmission, this function will return the offset of the transmission
duration to the beginning if this period.
*/
double
AquaSimRMac::CalculateOffset(double dt)
{

  int index=-1;

  double offset=0.0;
  double ack_window=m_maxShortPacketTransmissiontime;
  double elapsed_time=Simulator::Now().GetDouble()-m_cycleStartTime;
  struct period_record table[R_TABLE_SIZE];

  for(int i=0;i<R_TABLE_SIZE;i++)
    {
      table[i].node_addr=period_table[i].node_addr;
      double l=CheckLatency(short_latency_table, table[i].node_addr)
		  -m_maxShortPacketTransmissiontime;
      double d=period_table[i].difference-l;
      if (d<0) d+=m_periodInterval;
      table[i].difference=d;
    }
  SortPeriodTable(table);

  for (int i=0;i<R_TABLE_SIZE;i++)
    {
      NS_LOG_DEBUG("Node Addr:" << table[i].node_addr <<
		   " and difference:" << table[i].difference);
    }

  // find the first index that can be reached by sending data after elapsed_time
  int k=0;
  while((-1==index)&&(k<m_periodTableIndex))
    {
      if(table[k].difference+ack_window>elapsed_time) index=k;
      k++;
    }

  if(-1==index)
    {
      offset=elapsed_time;
      return offset;
    }

  //  double it=0;
  int start_index=-1;
  double t0=elapsed_time;

  for(int i=index;i<m_periodTableIndex-1;i++)
    {
      // double t=period_table[i+1].difference-period_table[i].difference;
      double t=table[i].difference-t0;

      //t-=ack_window;// avoid the reserved time slot for ackrev
      if((t>=dt)&&(-1==start_index)) start_index=i;
      t0=table[i].difference+ack_window;
    }
  //  printf("Calculate offset start_index=%d and index=%d and elapsedtime=%f t0=%f\n",
  //            start_index,index, elapsed_time,t0);

  // we assumw that the listen window is large enough, there must
  // exist slot larger enough for data transmission

  if(-1==start_index) start_index=m_periodTableIndex-1;
  if(start_index==index) return elapsed_time;

  offset=table[start_index-1].difference+ack_window;
  return offset;
}

// determine the sending time, we assume that listen duration is
// long enough such that we have enough time slots to select.
// this function randomly selects one of the available slots and
// converts into time scale
double
AquaSimRMac::DetermineSendingTime(AquaSimAddress receiver_addr)
{
  struct period_record table[R_TABLE_SIZE];

  for(int i=0;i<R_TABLE_SIZE;i++)
    {
      table[i].node_addr=period_table[i].node_addr;
      double l=CheckLatency(short_latency_table, table[i].node_addr)
		 -m_maxShortPacketTransmissiontime;
      double d=period_table[i].difference-l;
      if (d<0) d+=m_periodInterval;
      table[i].difference=d;
    }
    SortPeriodTable(table);

  //  double delay=CheckLatency(short_latency_table,receiver_addr)
  //  -max_short_packet_transmissiontime;

  /*
  double dt1=CheckDifference(period_table,receiver_addr);
  double elapsed_time=NOW-cycle_start_time;
  double offset_time=0;
  */
  double elapsed_time=Simulator::Now().GetDouble()-m_cycleStartTime;
  double time_slot=m_maxShortPacketTransmissiontime;
  double dt1=CheckDifference(table,receiver_addr);
  double offset_time=dt1+time_slot-elapsed_time;
  while (offset_time<0) offset_time+=m_periodInterval;

  double dt2=dt1+m_duration-time_slot; // end of the time slot
  double t0=dt1;   //start time of the time slot
  int num_slot=0;
  int n=0;
  int i=0;

  //  while ((period_table[i].difference>dt1)&&(period_table[i].difference<dt2))
  while (table[i].difference<dt2)
    {
      if(table[i].difference>dt1)
	{
	  double t=table[i].difference;
	  double l=t-t0-time_slot;
	  n=(int) floor(l/time_slot);
	num_slot+=n;
	t0=t;
        }
      i++;
    }

  double l=dt2-t0-time_slot;
  n=(int) floor(l/time_slot);
  num_slot+=n;

  int randIndex=rand()% num_slot;

  i=0;
  int sum=0;
  double rand_time=0.0;
  //  int sum1=0;
  t0=dt1;
  bool allocated=false;
  // while ((period_table[i].difference>=dt1)&&(period_table[i].difference<dt2))

  while (table[i].difference<dt2)
    {
      if(table[i].difference>dt1)
	{
	  double t=table[i].difference;
	  rand_time=t0-dt1;

	  double l=t-t0-time_slot;
	  n=(int) floor(l/time_slot);

	  t0=t;
	  // i++;
	  if((sum+n)>randIndex)
	    {
	      allocated=true;
	      while(sum<=randIndex)
		{
		  rand_time+=time_slot;
		  sum++;
		}
	    }
	  else sum+=n;
	}
      i++;
    }

  if(!allocated)
    {
      rand_time=t0-dt1;
      double l=dt2-t0-time_slot;
      n=(int) floor(l/time_slot);

      if((sum+n)>randIndex)
	{
	  allocated=true;
	  while(sum<=randIndex)
	    {
	      rand_time+=time_slot;
	      sum++;
	    }
	}
    }

  if(!allocated) NS_LOG_DEBUG("AquaSimRMac:DetermineSendingTime node:" << m_device->GetAddress() <<
			      " has some problem to allocate sending time");

   return offset_time+rand_time;
}


double
AquaSimRMac::CheckLatency(latency_record* table,AquaSimAddress addr)
{
  int i=0;
  double d=0.0;

  while((table[i].node_addr!=addr)&&(i<R_TABLE_SIZE))
    {
      //printf("node addr is%d and latency is%f\n",table[i].node_addr,table[i].latency);
      i++;
    }
  if (i==R_TABLE_SIZE) return d;
  else return table[i].latency;
}




double
AquaSimRMac::CheckDifference(period_record* table,AquaSimAddress addr)
{
  int i=0;
  double d=-0.0;

  while((table[i].node_addr!=addr)&&(i<R_TABLE_SIZE))i++;

  if (i==R_TABLE_SIZE) return d;
  else return table[i].difference;
}


void
AquaSimRMac::TxRev(Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this << Seconds(Simulator::Now()));

  AquaSimHeader asHeader;
  RMacHeader rHeader;
  p->RemoveHeader(asHeader);
  p->PeekHeader(rHeader);

  if(m_macStatus==RMAC_FORBIDDED)
    {
      NS_LOG_INFO("TxRev: node " << m_device->GetAddress() <<
		  " is in RMAC_FORBIDDED, cancel sending REV at " << Seconds(Simulator::Now()));
      p=0;
      return;
    }

  // Is it possible??
  if(m_macStatus==RMAC_WAIT_ACKREV)
    {
      NS_LOG_INFO("TxRev: node " << m_device->GetAddress() <<
		  " is in RMAC_ACKREV, cancel sending REV at " << Seconds(Simulator::Now()));
      p=0;
      return;
    }

  int totalHeaderSize = rHeader.GetSerializedSize() + asHeader.GetSize();
  asHeader.SetTxTime(Seconds((totalHeaderSize*m_encodingEfficiency + m_phyOverhead) / m_bitRate));

  Time txtime=asHeader.GetTxTime();

  m_macStatus=RMAC_WAIT_ACKREV;
  NS_LOG_INFO("TxRev: node " << m_device->GetAddress() <<
		  " is in RMAC_WAIT_ACKREV at " << Seconds(Simulator::Now()));
  // double t=Timer*PeriodInterval_;

  double t=5*m_periodInterval;
  m_timeoutEvent = Simulator::Schedule(Seconds(t), &AquaSimRMac::ResetMacStatus, this);

  if(SLEEP==m_device->TransmissionStatus())
    {
      PowerOn();
      m_device->SetTransmissionStatus(SEND);
      asHeader.SetTimeStamp(Seconds(Simulator::Now()));
      p->AddHeader(asHeader);

      SendDown(p);
      // printf("TxREV, node %d is in sleep at %f\n",index_,NOW);
      m_device->SetTransmissionStatus(SLEEP);
      Simulator::Schedule(txtime, &AquaSimRMac::StatusProcess, this, m_device->TransmissionStatus());
      return;
    }

  if(NIDLE==m_device->TransmissionStatus())
    {
      m_device->SetTransmissionStatus(SEND);
      asHeader.SetTimeStamp(Seconds(Simulator::Now()));
      p->AddHeader(asHeader);

      SendDown(p);
      // printf("broadcast %d Tx Idle set timer at %f tx is %f\n",node_->nodeid(),NOW,txtime);

      NS_LOG_INFO("TxRev: node " << m_device->GetAddress() << " is in idle at " << Seconds(Simulator::Now()));
      m_device->SetTransmissionStatus(NIDLE);
      Simulator::Schedule(txtime, &AquaSimRMac::StatusProcess, this, m_device->TransmissionStatus());
      return;
    }

  if(RECV==m_device->TransmissionStatus())
    {
      InterruptRecv(txtime.GetDouble());
      asHeader.SetTimeStamp(Seconds(Simulator::Now()));
      p->AddHeader(asHeader);
      SendDown(p);
      NS_LOG_INFO("TxRev: node " << m_device->GetAddress() << " is in recv at " << Seconds(Simulator::Now()));
      m_device->SetTransmissionStatus(NIDLE);
      Simulator::Schedule(txtime, &AquaSimRMac::StatusProcess, this, m_device->TransmissionStatus());
      return;
    }

  if (SEND==m_device->TransmissionStatus())
    {
      NS_LOG_INFO("TxRev: queue send data too fast");
      p=0;
      return;
    }
}



void
AquaSimRMac::InitPhaseTwo()
{
  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();

  double delay=m_rand->GetValue()*m_phaseTwoWindow;
  m_nextPeriod=m_intervalPhase2Phase3+m_phaseTwoCycle*m_phaseTwoWindow+delay;

  m_phaseThreeEvent = Simulator::Schedule(Seconds(m_nextPeriod), &AquaSimRMac::InitPhaseThree, this);

  StartPhaseTwo();
}

void
AquaSimRMac::StartPhaseTwo()
{
  if(m_phaseTwoCycle)
    {
      NS_LOG_INFO("Phase Two: node " << m_device->GetAddress() << " and cycle:" << m_phaseTwoCycle);
      m_phaseStatus=PHASETWO;
      m_cycleStartTime=Simulator::Now().GetDouble();

      Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
      double  delay=m_rand->GetValue()*m_phaseTwoWindow;
      Ptr<Packet> pkt=GenerateSYN();

      Simulator::Schedule(Seconds(delay), &AquaSimRMac::TxND, this, pkt, m_phaseTwoWindow);
      m_phaseTwoEvent = Simulator::Schedule(Seconds(m_phaseTwoWindow+m_phaseTwoInterval), &AquaSimRMac::StartPhaseTwo, this);

      m_nextPeriod-=m_phaseTwoWindow-m_phaseTwoInterval;
      m_phaseTwoCycle--;
    }
  return;
}

/*
void
AquaSimRMac::InitPhaseTwo(){

   double delay=Random::uniform()*PhaseTwo_window_;
   PhaseStatus=PHASETWO;

    cycle_start_time=NOW;
    next_period=IntervalPhase2Phase3_+PhaseTwo_window_+delay;

    printf("rmac Initphasetwo: the phasethree of node %d is scheduled at %f\n",index_,NOW+next_period);
    m_phaseTwoEvent = Simulator::Schedule(Seconds(delay), &AquaSimRMac::StartPhaseTwo, this);
    return;
}
*/

Ptr<Packet>
AquaSimRMac::GenerateSYN()
{
  Ptr<Packet> pkt = Create<Packet>();
  AquaSimHeader asHeader;
  RMacHeader rHeader;
  AquaSimPtTag ptag;

  // size()=short_packet_size_;
  //asHeader.SetNextHop(MAC_BROADCAST);
  asHeader.SetDirection(AquaSimHeader::DOWN);
  // addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_RMAC);

  rHeader.SetPtype(P_SYN);
  rHeader.SetPktNum(m_numSend);
  rHeader.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  rHeader.SetDuration(m_duration);
  m_numSend++;

  pkt->AddHeader(asHeader);
  pkt->AddHeader(rHeader);
  pkt->AddPacketTag(ptag);

  NS_LOG_INFO("AquaSimRMac:GenerateSYN: node " << rHeader.GetSenderAddr() <<
	      " generates SYN packet at " << Seconds(Simulator::Now()));
  return pkt;
}

void
AquaSimRMac::SendSYN()
{
  Ptr<Packet> pkt = Create<Packet>();
  AquaSimHeader asHeader;
  RMacHeader rHeader;
  AquaSimPtTag ptag;

  // size()=short_packet_size_;
  //asHeader.SetNextHop(MAC_BROADCAST);
  asHeader.SetDirection(AquaSimHeader::DOWN);
  // addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_RMAC);

  rHeader.SetPtype(P_SYN);
  rHeader.SetPktNum(m_numSend);
  rHeader.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  rHeader.SetDuration(m_duration);
  m_numSend++;

  pkt->AddHeader(asHeader);
  pkt->AddHeader(rHeader);
  pkt->AddPacketTag(ptag);

  NS_LOG_INFO("AquaSimRMac:SendSYN: node " << rHeader.GetSenderAddr() <<
	      " send SYN packet at " << Seconds(Simulator::Now()));
  TxND(pkt, m_phaseTwoWindow);
}


void
AquaSimRMac::InitND(double t1,double t2, double t3)
{
  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();

  double delay=m_rand->GetValue()*t1;
  double itval=(t3-t2-t1)/2.0;
  double delay3=t1+itval;

  m_shortNdEvent = Simulator::Schedule(Seconds(delay), &AquaSimRMac::ShortNDHandler, this);
  Simulator::Schedule(Seconds(delay3), &AquaSimRMac::SendShortAckND, this);
}

void
AquaSimRMac::ShortNDHandler()
{
  m_cycleStartTime=Simulator::Now().GetDouble();
  SendND(m_shortPacketSize);
}

void
AquaSimRMac::SendND(int pkt_size)
{
  NS_LOG_FUNCTION(this << Seconds(Simulator::Now()));
  Ptr<Packet> pkt = Create<Packet>();
  AquaSimHeader asHeader;
  RMacHeader rHeader;
  AquaSimPtTag ptag;

  // additional 2*8 denotes the size of type,next-hop of the packet and
  // timestamp
      //Not really used in ns3
  // size()=short_packet_size_;

  //asHeader.SetNextHop(MAC_BROADCAST);
  asHeader.SetDirection(AquaSimHeader::DOWN);
  // addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_RMAC);

  rHeader.SetPtype(P_ND);
  rHeader.SetPktNum(m_numSend);
  rHeader.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  m_numSend++;

  pkt->AddHeader(asHeader);
  pkt->AddHeader(rHeader);
  pkt->AddPacketTag(ptag);

  //printf("rmac SendND:node(%d) send ND type is %d at %f\n", ndh->sender_addr,cmh->ptype_, NOW);
  TxND(pkt, m_NDwindow);
}

void
AquaSimRMac::SendShortAckND()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());
  if (m_arrivalTableIndex==0) return;// not ND received

  while(m_arrivalTableIndex>0)
    {
      Ptr<Packet> pkt = Create<Packet>();
      AquaSimHeader asHeader;
      RMacHeader rHeader;
      AquaSimPtTag ptag;

      rHeader.SetPtype(P_SACKND);
      rHeader.SetPktNum(m_numSend);
      rHeader.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
      m_numSend++;

      ptag.SetPacketType(AquaSimPtTag::PT_RMAC);

      int index1=-1;
      index1=rand()%m_arrivalTableIndex;
      double t2=-0.1;
      double t1=-0.1;

      AquaSimAddress receiver=arrival_table[index1].node_addr;
      t2=arrival_table[index1].arrival_time;
      t1=arrival_table[index1].sending_time;

      for(int i=index1;i<m_arrivalTableIndex;i++)
	{
	  arrival_table[i].node_addr=arrival_table[i+1].node_addr;
          arrival_table[i].sending_time=arrival_table[i+1].sending_time;
          arrival_table[i].arrival_time=arrival_table[i+1].arrival_time;
	}

      rHeader.SetArrivalTime(Seconds(t2));
      rHeader.SetTs(t1);
  // additional 2*8 denotes the size of type,next-hop of the packet and
  // timestamp
      //not used.
      //  cmh->size()=sizeof(hdr_ack_nd)+3*8;
      //cmh->size()=short_packet_size_;

      asHeader.SetNextHop(receiver);
      asHeader.SetDirection(AquaSimHeader::DOWN);
      // addr_type()=NS_AF_ILINK;

      pkt->AddHeader(asHeader);
      pkt->AddHeader(rHeader);
      pkt->AddPacketTag(ptag);

      Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
      double delay=m_rand->GetValue()*m_ackNDwindow;
      Simulator::Schedule(Seconds(delay), &AquaSimRMac::TxND, this, pkt, m_ackNDwindow);

      m_arrivalTableIndex--;
    }

  m_arrivalTableIndex=0;
  for(int i=0;i<R_TABLE_SIZE;i++)
    arrival_table[i].node_addr=AquaSimAddress(-1);
}

/*
void
AquaSimRMac::TxND(Packet* pkt, double window)
{
  //  printf("RMac TxND node %d\n",index_);
  hdr_cmn* cmh=HDR_CMN(pkt);
   hdr_rmac* synh = HDR_RMAC(pkt);

  assert(initialized());
  UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;


  hdr_cmn::access(pkt)->txtime()=(cmh->size()*encoding_efficiency_+
                                   PhyOverhead_)/bit_rate_;

  double txtime=hdr_cmn::access(pkt)->txtime();

  if(SLEEP==n->TransmissionStatus()) {
  Poweron();
  n->SetTransmissionStatus(SEND);
  cmh->ts_=NOW;

  if(PhaseStatus==PHASETWO){

    double t=NOW-cycle_start_time;

    synh->interval=next_period-t;

  m_phaseThreeEvent = Simulator::Schedule(Seconds(synh->interval), &AquaSimRMac::InitPhaseThree, this);
  }

  sendDown(pkt);
      m_NDBackoffCounter=0;	//reset

  m_device->SetTransmissionStatus(NIDLE);
  Simulator::Schedule(totalTxTime, &AquaSimRMac::StatusProcess, this, m_device->TransmissionStatus());
  return;
  }

  if(NIDLE==n->TransmissionStatus()){

  n->SetTransmissionStatus(SEND);

  //printf("TxND the data type is %d\n",MAC_BROADCAST);
  //printf("broadcast : I am going to send the packet down tx is %f\n",txtime);
     cmh->ts_=NOW;

  if(PhaseStatus==PHASETWO){

   double t=NOW-cycle_start_time;
   synh->interval=next_period-t;

    m_phaseThreeEvent = Simulator::Schedule(Seconds(synh->interval), &AquaSimRMac::InitPhaseThree, this);
  }

  sendDown(pkt);
      m_NDBackoffCounter=0;	//reset
  //  printf("broadcast %d Tx Idle set timer at %f tx is %f\n",node_->nodeid(),NOW,txtime);

  m_device->SetTransmissionStatus(NIDLE);
  Simulator::Schedule(totalTxTime, &AquaSimRMac::StatusProcess, this, m_device->TransmissionStatus());
  return;
  }

  if(RECV==n->TransmissionStatus())
    {
      Scheduler& s=Scheduler::instance();
      double d1=window-(NOW-cycle_start_time);

      if(d1>0){
      double backoff=Random::uniform()*d1;
      m_NDBackoffWindow_=window;
   // printf("broadcast Tx set timer at %f backoff is %f\n",NOW,backoff);
      s.schedule(&backoff_handler,(Event*) pkt,backoff);
      return;
      }
      else {
      m_NDBackoffCounter=0;	//reset
          printf("Rmac:NODE %d backoff:no time left \n",index_);
          Packet::free(pkt);
      }

    }

if (SEND==n->TransmissionStatus())
{
  // this case is supposed not to  happen
    printf("rmac: queue send data too fas\n");
    Packet::free(pkt);
      return;
}

}

*/



void
AquaSimRMac::TxND(Ptr<Packet> pkt, double window)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());

  AquaSimHeader asHeader;
  RMacHeader rHeader;
  pkt->RemoveHeader(asHeader);
  pkt->RemoveHeader(rHeader);

  Time totalTxTime = Seconds( ((asHeader.GetSerializedSize() + rHeader.GetSerializedSize()) * m_encodingEfficiency
			+ m_phyOverhead) / m_bitRate );
  asHeader.SetTxTime(totalTxTime);

  if(SLEEP==m_device->TransmissionStatus())
    {
      PowerOn();
      m_device->SetTransmissionStatus(SEND);
      asHeader.SetTimeStamp(Simulator::Now());

      if(m_phaseStatus==PHASETWO)
	{
	  double t=Simulator::Now().GetDouble()-m_cycleStartTime;
	  rHeader.SetInterval(m_nextPeriod-t);
	}

      pkt->AddHeader(asHeader);
      pkt->AddHeader(rHeader);
      SendDown(pkt);
      m_NDBackoffCounter=0;	//reset

      m_device->SetTransmissionStatus(NIDLE);
      Simulator::Schedule(totalTxTime, &AquaSimRMac::StatusProcess, this, m_device->TransmissionStatus());
      return;
    }

  if(NIDLE==m_device->TransmissionStatus())
    {
      m_device->SetTransmissionStatus(SEND);
      //printf("TxND the data type is %d\n",MAC_BROADCAST);
      //printf("broadcast : I am going to send the packet down tx is %f\n",txtime);
      asHeader.SetTimeStamp(Simulator::Now());

      if(m_phaseStatus==PHASETWO)
	{
	  double t=Simulator::Now().GetDouble()-m_cycleStartTime;
	  rHeader.SetInterval(m_nextPeriod-t);
	}
      pkt->AddHeader(asHeader);
      pkt->AddHeader(rHeader);
      SendDown(pkt);
      m_NDBackoffCounter=0;	//reset
      //  printf("broadcast %d Tx Idle set timer at %f tx is %f\n",node_->nodeid(),NOW,txtime);

      m_device->SetTransmissionStatus(NIDLE);
      Simulator::Schedule(totalTxTime, &AquaSimRMac::StatusProcess, this, m_device->TransmissionStatus());
      return;
    }

  if(RECV==m_device->TransmissionStatus())
    {
      double d1=window-(Simulator::Now().GetDouble()-m_cycleStartTime);

      if(d1>0)
	{
	  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
	  double backoff=m_rand->GetValue()*d1;
	  m_NDBackoffWindow=window;
	  // printf("broadcast Tx set timer at %f backoff is %f\n",NOW,backoff);

	  pkt->AddHeader(asHeader);
	  pkt->AddHeader(rHeader);
	  Simulator::Schedule(Seconds(backoff), &AquaSimRMac::NDBackoffHandler, this, pkt);
	  return;
	}
      else
	{
	  m_NDBackoffCounter=0;	//reset
	  NS_LOG_WARN("AquaSimRMac:Backoff: node " << m_device->GetAddress() << " no time left, just drop the packet");
	  pkt=0;
	}
    }

  if (SEND==m_device->TransmissionStatus())
    {
      // this case is supposed to happen
      NS_LOG_INFO("AquaSimRMac: queue send data too fast");
      pkt=0;
      return;
    }
}

void
AquaSimRMac::NDBackoffHandler(Ptr<Packet> p)
{
  m_NDBackoffCounter++;
  if(m_NDBackoffCounter<MAXIMUMBACKOFF)
    TxND(p,m_NDBackoffWindow);
  else
    {
      m_NDBackoffCounter=0;  //clear
      NS_LOG_WARN("AquaSimRMac:Backoff: too many backoffs.");
      p=0;
    }
}

void
AquaSimRMac::ProcessACKRevPacket(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this << Seconds(Simulator::Now()));

  AquaSimHeader asHeader;
  RMacHeader rHeader;
  pkt->PeekHeader(asHeader);
  pkt->PeekHeader(rHeader);

  AquaSimAddress dst=asHeader.GetNextHop();
  int ptype=rHeader.GetPtype();

  // since the node clearly receives the ACKrev, does not need to backoff,
  //  therefore, resets the carrier sense;

  if(ptype!=P_ACKREV)
    {
      NS_LOG_INFO("AquaSimRMac:ProcessACKRevPacket: node " << m_device->GetAddress() <<
		  " receives no ACK_REV packet at " << Seconds(Simulator::Now()));
      return;
    }
  if(asHeader.GetErrorFlag())
    {
      NS_LOG_INFO("AquaSimRMac:ProcessACKRevPacket: node " << m_device->GetAddress() <<
		  " senses carrier in ackwindow at " << Seconds(Simulator::Now()));
      m_carrierSense=true;
      return;
    }
  m_device->ResetCarrierId();
  m_carrierSense=false;

  AquaSimAddress receiver_addr=rHeader.GetRecvAddr();
  AquaSimAddress sender_addr=rHeader.GetSenderAddr();
  double st=rHeader.GetSt();
  double dt=rHeader.GetDuration() - st;

  double  l=CheckLatency(short_latency_table,sender_addr);
  double  it=st-l;

  double elapsedtime=Simulator::Now().GetDouble()-m_cycleStartTime;

  if(elapsedtime>1.1*m_maxShortPacketTransmissiontime)
    {
      NS_LOG_INFO("AquaSimRMac:ProcessACKRevPacket: node " << m_device->GetAddress() <<
		  " this is out of my ackrev window...");
      pkt=0;
      return;
    }


  if((dst!=m_device->GetAddress())&&(m_device->GetAddress()==receiver_addr))
    {
      NS_LOG_INFO("AquaSimRMac:ProcessACKRevPacket: node " << m_device->GetAddress() <<
		  " receives a ackrev target at " << dst << " and receiver is " << receiver_addr);
      pkt=0;
      return;
    }
  NS_LOG_INFO("AquaSimRMac:ProcessACKRevPacket: node " << m_device->GetAddress() <<
		  " I get the ACK REV packet offset is " << it << " and duration=" <<
		  dt << "at " << Seconds(Simulator::Now()));
  //printf("rmac:ProcessAckRevPacket: node %d I get the ACK REV packet interval is %f \n",index_, it);

  if(it<0)
    NS_LOG_INFO("AquaSimRMac:ProcessACKRevPacket: the notification is too short");

  pkt=0;

  if(receiver_addr!=m_device->GetAddress())
    {
      // This ackrev is not for me
      NS_LOG_INFO("AquaSimRMac:ProcessACKRevPacket: node " << m_device->GetAddress() <<
		  " this ACKREV is not for me");
      double poffset=m_periodInterval+elapsedtime+it-l+m_maxShortPacketTransmissiontime;
      InsertReservedTimeTable(receiver_addr,poffset,dt);
    }
    else
      {
	// This ackrev is for me
	if(m_macStatus!=RMAC_WAIT_ACKREV) {
	    NS_LOG_INFO("AquaSimRMac:ProcessACKRevPacket: status change, I quit this chance ");
	    return;
	}

	NS_LOG_INFO("AquaSimRMac:ProcessACKRevPacket: node " << m_device->GetAddress() <<
		    " this ACKREV is for me.");
        m_numData=0;

	double  it1=it-l+m_maxShortPacketTransmissiontime;
	m_macStatus=RMAC_TRANSMISSION;

	NS_LOG_INFO("AquaSimRMac:ProcessACKRevPacket: node " << m_device->GetAddress() <<
			" schedule Txdata after " << it1 << " at time " << Seconds(Simulator::Now()) <<
			" latency is " << l);
	Simulator::Cancel(m_timeoutEvent); // cancel the timer of rev
        m_transReceiver=sender_addr;
        m_transmissionEvent = Simulator::Schedule(Seconds(it1), &AquaSimRMac::TxData, this, m_transReceiver);
    }
    return;
}

void
AquaSimRMac::ClearTxBuffer()
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());

  Ptr<Packet> p1[MAXIMUM_BUFFER];

  for (int i=0;i<MAXIMUM_BUFFER;i++)p1[i]=NULL;
  buffer_cell* bp=m_txBuffer.head_;
  int i=0;
  while(bp)
    {
      p1[i]=bp->packet;
      bp=bp->next;
      i++;
    }

  for (int i=0;i<MAXIMUM_BUFFER;i++)
    {
      //   printf("ClearTxBuffer the poniter is%d\n",p1[i]);
      if (m_bitMap[i]==1) m_txBuffer.DeletePacket(p1[i]);
    }

  /*
  printf("ClearTxBuffer: show the queue****************after txbufferdelete\n");
      t->showqueue();
  */
  NS_LOG_INFO("AquaSimRMac:TxBuffer is cleared, there are " << m_txBuffer.num_of_packet << " packets left");
  return;
}

void
AquaSimRMac::ProcessACKDataPacket(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << Seconds(Simulator::Now()) << m_duration);

  NS_LOG_INFO("AquaSimRMac:ProcessACKDataPacket canceling timeout event");
  Simulator::Cancel(m_timeoutEvent); // cancel the timer of data

  for (int i=0;i<MAXIMUM_BUFFER;i++)m_bitMap[i]=0;

  //memcpy(m_bitMap, pkt->accessdata(),sizeof(m_bitMap));
  //copy data...

  NS_LOG_INFO("AquaSimRMac:ProcessACKDataPacket node " << m_device->GetAddress() << " received the bitmap is");
  for (int i=0;i<MAXIMUM_BUFFER;i++) NS_LOG_INFO("bitmap[" << i << "]=" << m_bitMap[i]);

  NS_LOG_INFO("AquaSimRMac:TxBuffer will be cleared, there are " << m_txBuffer.num_of_packet <<
	      " packets in queue and duration=" << m_duration);
  pkt=0;

 /*
!!!
This part should consider the retransmission state, in this implementation, we don't consider the packets loss
caused by channel errors, therefore, we just ignore it, it should be added later.
*/
  ClearTxBuffer();
  m_numBlock++;
  m_txBuffer.UnlockBuffer();
  m_macStatus=RMAC_IDLE;
  NS_LOG_INFO("AquaSimRMac:TxBuffer node " << m_device->GetAddress() <<
		  " unlock txbuffer duration=" << m_duration);
  ResumeTxProcess();
}

void
AquaSimRMac::ProcessRevPacket(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());
  RMacHeader rHeader;
  pkt->PeekHeader(rHeader);

  AquaSimAddress sender_addr=rHeader.GetSenderAddr();
  double dt=rHeader.GetDuration();
  double interval=rHeader.GetInterval();
  int block=rHeader.GetBlockNum();

  pkt=0;

  if (m_macStatus==RMAC_IDLE)
    {
      if(m_reservationTableIndex <R_TABLE_SIZE)
	{
	  reservation_table[m_reservationTableIndex].node_addr=sender_addr;
	  reservation_table[m_reservationTableIndex].required_time=dt;
	  reservation_table[m_reservationTableIndex].interval=interval;
	  reservation_table[m_reservationTableIndex].block_id=block;
	  m_reservationTableIndex++;
	}
      else
	{
	  NS_LOG_INFO("AquaSimRMac:ProcessRevPacket: too many reservation, drop the packet");
	}
    }
  else
    {
      NS_LOG_INFO("AquaSimRMac:ProcessRevPacket: I am not in idle state, drop this packet");
    }
}

void
AquaSimRMac::ProcessNDPacket(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this);
  AquaSimHeader asHeader;
  RMacHeader rHeader;
  pkt->PeekHeader(asHeader);
  pkt->PeekHeader(rHeader);

  AquaSimAddress sender=rHeader.GetSenderAddr();
  double time=Simulator::Now().ToDouble(Time::S);
  if(m_arrivalTableIndex>=R_TABLE_SIZE)
    {
      NS_LOG_INFO("AquaSimRMac:ProcessNDPacket arrival table is full");
      pkt=0;
      return;
    }
  arrival_table[m_arrivalTableIndex].node_addr=sender;
  arrival_table[m_arrivalTableIndex].arrival_time=time;
  arrival_table[m_arrivalTableIndex].sending_time=asHeader.GetTimeStamp().GetDouble();
  m_arrivalTableIndex++;
  pkt=0;
  return;
}


void
AquaSimRMac::ProcessDataPacket(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());
  RMacHeader rHeader;
  pkt->PeekHeader(rHeader);

  AquaSimAddress data_sender=rHeader.GetSenderAddr();
  int bnum=rHeader.GetBlockNum();
  int num=rHeader.GetDataNum();

  m_recvBusy=true;
  Simulator::Cancel(m_timeoutEvent);

  //     MarkBitMap(num);
  UpdateACKDataTable(data_sender,bnum,num);

  SendUp(pkt);
  return;
}

/*
void
AquaSimRMac::MarkBitMap(int num){
  if(num<MAXIMUM_BUFFER) bit_map[num]=1;
}
*/

void
AquaSimRMac::UpdateACKDataTable(AquaSimAddress data_sender,int bnum,int num)
{
  int index=-1;
  for (int i=0;i<m_ackDataTableIndex;i++)
    if(ackdata_table[i].node_addr==data_sender) index=i;

  if(index==-1){
    ackdata_table[m_ackDataTableIndex].node_addr=data_sender;
    ackdata_table[m_ackDataTableIndex].block_num=bnum;
    ackdata_table[m_ackDataTableIndex].bitmap[num]=1;
    m_ackDataTableIndex++;
  }
  else
    {
    ackdata_table[index].node_addr=data_sender;
    ackdata_table[index].block_num=bnum;
    ackdata_table[index].bitmap[num]=1;
    }
}

// this program need to be modified to handle the
// retransmission
void
AquaSimRMac::ScheduleACKData(AquaSimAddress data_sender)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << Seconds(Simulator::Now()));

  if(data_sender.GetAsInt()<0)
    {
      NS_LOG_INFO("AquaSimRMac:ScheduleACKData invalid sender address");
      return;
    }

  Ptr<Packet> pkt = Create<Packet>(sizeof(m_bitMap));
  AquaSimHeader asHeader;
  RMacHeader rHeader;
  AquaSimPtTag ptag;

  CopyBitmap(pkt, data_sender);

  asHeader.SetNextHop(data_sender);
  asHeader.SetDirection(AquaSimHeader::DOWN);
  // addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_RMAC);
  // size() = short_packet_size;

  rHeader.SetPtype(P_ACKDATA);
  rHeader.SetPktNum(m_numSend);
  rHeader.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  m_numSend++;

  pkt->AddHeader(asHeader);
  pkt->AddHeader(rHeader);
  pkt->AddPacketTag(ptag);

  double t1=DetermineSendingTime(data_sender);

  NS_LOG_INFO("AquaSimRMac:ScheduleACKData: node " << m_device->GetAddress() <<
		" schedule ackdata after " << t1 << " at " << Seconds(Simulator::Now()));
  Simulator::Schedule(Seconds(t1), &AquaSimRMac::TxACKData, this, pkt);
}

void
AquaSimRMac::CopyBitmap(Ptr<Packet> pkt,AquaSimAddress data_sender)
{
  int index=-1;
  for (int i=0;i<m_ackDataTableIndex;i++)
    if(ackdata_table[i].node_addr==data_sender) index=i;

  if(index!=-1)
    {//memcpy(pkt->accessdata(),ackdata_table[index].bitmap,sizeof(m_bitMap));
    }
  else
    NS_LOG_INFO("AquaSimRMac:CopyBitMap: Node" << m_device->GetAddress() <<
		  " I can't find the entry of the sender " << data_sender);
}

bool
AquaSimRMac::IsSafe()
{
  bool safe_status=true;
  if(RMAC_FORBIDDED!=m_macStatus) return safe_status;
  double start_time=Simulator::Now().GetDouble()-m_cycleStartTime;
  double ending_time=start_time+m_maxShortPacketTransmissiontime;
  for(int i=0;i<m_reservedTimeTableIndex;i++)
    {
      double t1=reserved_time_table[i].start_time;
      double d1=reserved_time_table[i].duration;
      if((ending_time>t1)&&((t1+d1)>start_time)) safe_status=false;
    }
  return safe_status;
}

void
AquaSimRMac::TxACKData(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress() << Seconds(Simulator::Now()));

  AquaSimHeader asHeader;
  RMacHeader rHeader;
  pkt->RemoveHeader(asHeader);
  pkt->PeekHeader(rHeader);

  if(!IsSafe()) {
    // It is not safe to send this ACKData since it collides with reserved time  slot
    pkt=0;
    return;
  }

  Time totalTxTime = Seconds( ((asHeader.GetSerializedSize() + rHeader.GetSerializedSize()) * m_encodingEfficiency
			+ m_phyOverhead) / m_bitRate );
  asHeader.SetTxTime(totalTxTime);
  m_macStatus=RMAC_IDLE;
  NS_LOG_INFO("AquaSimRMac:TxACKData: node " << m_device->GetAddress() <<
		" is in RMAC_IDLE at " << Seconds(Simulator::Now()));

  if(SLEEP==m_device->TransmissionStatus())
    {
      PowerOn();
      m_device->SetTransmissionStatus(SEND);
      asHeader.SetTimeStamp(Simulator::Now());
      pkt->AddHeader(asHeader);

      SendDown(pkt);
      NS_LOG_INFO("AquaSimRMac:TxACKData: node " << m_device->GetAddress() << " at " << Seconds(Simulator::Now()));
      m_device->SetTransmissionStatus(SLEEP);
      Simulator::Schedule(totalTxTime, &AquaSimRMac::StatusProcess, this, m_device->TransmissionStatus());
      return;
    }

  if(NIDLE==m_device->TransmissionStatus())
    {
      m_device->SetTransmissionStatus(SEND);
      asHeader.SetTimeStamp(Simulator::Now());
      pkt->AddHeader(asHeader);

      SendDown(pkt);
      NS_LOG_INFO("AquaSimRMac:TxACKData: node " << m_device->GetAddress() << " at " << Seconds(Simulator::Now()));
      m_device->SetTransmissionStatus(NIDLE);
      Simulator::Schedule(totalTxTime, &AquaSimRMac::StatusProcess, this, m_device->TransmissionStatus());
      return;
    }

  if(RECV==m_device->TransmissionStatus())
    {
      InterruptRecv(totalTxTime.GetDouble());
      asHeader.SetTimeStamp(Simulator::Now());
      pkt->AddHeader(asHeader);

      SendDown(pkt);
      NS_LOG_INFO("AquaSimRMac:TxACKData: node " << m_device->GetAddress() << " at " << Seconds(Simulator::Now()));
      m_device->SetTransmissionStatus(NIDLE);
      Simulator::Schedule(totalTxTime, &AquaSimRMac::StatusProcess, this, m_device->TransmissionStatus());
      return;
    }

  if (SEND==m_device->TransmissionStatus())
    {
      NS_LOG_INFO("AquaSimRMac:TxACKDATA: node " << m_device->GetAddress() << " send data too fast");
      pkt=0;
      return;
    }
}

void
AquaSimRMac::ProcessShortACKNDPacket(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());
  AquaSimHeader asHeader;
  RMacHeader rHeader;
  pkt->PeekHeader(asHeader);
  pkt->PeekHeader(rHeader);

  AquaSimAddress sender=rHeader.GetSenderAddr();
  double t4=Simulator::Now().ToDouble(Time::S);
  double t3=asHeader.GetTimeStamp().ToDouble(Time::S);

  double t2=rHeader.GetArrivalTime().ToDouble(Time::S);
  double t1=rHeader.GetTs();

  double latency=((t4-t1)-(t3-t2))/2.0;
  bool newone=true;

  pkt=0;

  for (int i=0;i<R_TABLE_SIZE;i++)
    {
      if (short_latency_table[i].node_addr==sender)
	{
	 short_latency_table[i].sumLatency+=latency;
	 short_latency_table[i].num++;
	 short_latency_table[i].last_update_time=Simulator::Now().GetDouble();
	 short_latency_table[i].latency =
		    short_latency_table[i].sumLatency/short_latency_table[i].num;
	 newone=false;
	}
    }

  if(newone)
    {
      if(m_shortLatencyTableIndex>=R_TABLE_SIZE)
        {
	  NS_LOG_INFO("AquaSimRMac:ProcessNDPacket: arrival table is full");
	  return;
        }
      short_latency_table[m_shortLatencyTableIndex].node_addr=sender;
      short_latency_table[m_shortLatencyTableIndex].sumLatency+=latency;
      short_latency_table[m_shortLatencyTableIndex].num++;
      short_latency_table[m_shortLatencyTableIndex].last_update_time=Simulator::Now().GetDouble();
      short_latency_table[m_shortLatencyTableIndex].latency =
	 short_latency_table[m_shortLatencyTableIndex].sumLatency/short_latency_table[m_shortLatencyTableIndex].num;
      m_shortLatencyTableIndex++;
    }
  for(int i=0;i<m_shortLatencyTableIndex;i++)
    {
      NS_LOG_INFO("node " << m_device->GetAddress()  << " to node " << short_latency_table[i].node_addr <<
		     " short latency is " << short_latency_table[i].latency <<
		     " and number is " << short_latency_table[i].num);
    }
  return;
}


void
AquaSimRMac::ProcessSYN(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this);
  RMacHeader rHeader;
  pkt->PeekHeader(rHeader);

  AquaSimAddress sender=rHeader.GetSenderAddr();
  double interval=rHeader.GetInterval();
  double tduration=rHeader.GetDuration();
  pkt=0;

  double t1=-1.0;
  for (int i=0;i<R_TABLE_SIZE;i++)
    if (short_latency_table[i].node_addr==sender)
      t1=short_latency_table[i].latency;

 if(t1==-1.0)
   {
     NS_LOG_INFO("AquaSimRMac:ProcessSYN: I receive a SYN from unknown neighbor " << sender);
     return;
   }

 interval-=t1;
 double t2=m_nextPeriod-(Simulator::Now().ToDouble(Time::S)-m_cycleStartTime);
 double d=interval-t2;

  if (d>=0.0)
    {
      while (d>=m_periodInterval) d-=m_periodInterval;
    }
  else
    {
      while (d+m_periodInterval<=0.0) d+=m_periodInterval;
    }


  bool newone=true;

  if(d<0) d=d+m_periodInterval;

  for (int i=0;i<R_TABLE_SIZE;i++)
    if (period_table[i].node_addr==sender)
      {
	 period_table[i].difference=d;
	 period_table[i].last_update_time=Simulator::Now().GetDouble();
	 period_table[i].duration =tduration;
	 newone=false;
      }

  if(newone)
    {
      if(m_periodTableIndex>=R_TABLE_SIZE)
        {
          NS_LOG_INFO("ProcessSYN: period_table is full");
	        return;
        }
      period_table[m_periodTableIndex].node_addr=sender;
      period_table[m_periodTableIndex].difference=d;
      period_table[m_periodTableIndex].last_update_time=Simulator::Now().GetDouble();
      period_table[m_periodTableIndex].duration=tduration;
      m_periodTableIndex++;
    }

  for(int i=0;i<m_periodTableIndex;i++)
    NS_LOG_INFO("node " << m_device->GetAddress() << " to node " << period_table[i].node_addr <<
		" period difference is " << period_table[i].difference);
  return;
}


/*
 this program is used to handle the received packet,
it should be virtual function, different class may have
different versions.
*/
void
AquaSimRMac::RecvProcess(Ptr<Packet> pkt)
{
  AquaSimHeader asHeader;
  RMacHeader rHeader;
  AquaSimPtTag ptag;
  pkt->PeekHeader(asHeader);
  pkt->PeekHeader(rHeader);
  pkt->PeekPacketTag(ptag);

  AquaSimAddress dst=asHeader.GetNextHop();
  int ptype=rHeader.GetPtype();
  double elapsed_time=Simulator::Now().GetDouble()-m_cycleStartTime;
  double ack_window=m_maxShortPacketTransmissiontime+m_theta;

  NS_LOG_FUNCTION(this << Seconds(Simulator::Now()));

  // check if it is in the revack window
  if(elapsed_time<=ack_window)
    {
      ProcessACKRevPacket(pkt);
      return;
    }

  if (asHeader.GetErrorFlag())
    {
      NS_LOG_INFO("AquaSimRMac:RecvProcess node " << m_device->GetAddress() <<
		    " gets a corrupted packet at " << Seconds(Simulator::Now()));
      pkt=0;
      return;
    }

  if(dst==AquaSimAddress::GetBroadcast())
    {
      NS_LOG_INFO("AquaSimRMac:RecvProcess node " << m_device->GetAddress() <<
		  " gets a broadcast packet at " << Seconds(Simulator::Now()) <<
		  " and type is " << ptype);
      if (ptype==P_ND) ProcessNDPacket(pkt); //this is ND packet
      if (ptype==P_SYN) ProcessSYN(pkt); // this is ACK_ND packet
      // uptarget_->recv(pkt, this);
      return;
    }

  //  if (ptag.GetPacketType()==AquaSimPtTag::P_ACKREV) {ProcessACKRevPacket(pkt); return;}
  if(dst==m_device->GetAddress())
    {
      NS_LOG_INFO("AquaSimRMac:RecvProcess node " << m_device->GetAddress() <<
		  " gets a packet at " << Seconds(Simulator::Now()) <<
		  " and type is " << ptype);
      if (ptype==P_SACKND) ProcessShortACKNDPacket(pkt);
      if (ptype==P_REV) ProcessRevPacket(pkt);
      if (ptype==P_DATA) ProcessDataPacket(pkt);
      // if (ptag.GetPacketType()==AquaSimPtTag::P_ACKREV) ProcessACKRevPacket(pkt);
      if(ptype==P_ACKDATA) ProcessACKDataPacket(pkt);
      // printf("underwaterbroadcastmac:this is my packet \n");
      //  uptarget_->recv(pkt, this);
      return;
    }
  NS_LOG_INFO("AquaSimRMac:RecvProcess node " << m_device->GetAddress() <<
		  " this is neither broadcast nor my packet " << dst <<
		  ", just drop it at " << Seconds(Simulator::Now()));
  pkt=0;
  return;
}


void
AquaSimRMac::TxData(AquaSimAddress receiver)
{
  NS_LOG_FUNCTION(this << Seconds(Simulator::Now()));

  if (m_txBuffer.IsEmpty())
    {
      NS_LOG_DEBUG("AquaSimRMac:TxData: what?! I dont have data to send");
      return;
    }

  if(m_macStatus!=RMAC_TRANSMISSION)
    {
      NS_LOG_DEBUG("AquaSimRMac:TxData: node " << m_device->GetAddress() << " is not in transmission state");
      return;
    }

  if(m_device->TransmissionStatus()==SLEEP) PowerOn();

  m_macStatus=RMAC_TRANSMISSION;

  Ptr<Packet> pkt = m_txBuffer.next();

  AquaSimHeader asHeader;
  RMacHeader rHeader;
  AquaSimPtTag ptag;
  pkt->RemoveHeader(asHeader);
  pkt->RemoveHeader(rHeader);
  pkt->RemovePacketTag(ptag);
  //hdr_uwvb* hdr2=hdr_uwvb::access(pkt);

  // printf("RMac:node %d TxData at time %f data type is %d offset is%d and size is %d and offset is %d and size is%d uwvb offset is %d and size is %d\n",index_,NOW,hdr2->mess_type,cmh,sizeof(hdr_cmn),datah,sizeof(hdr_rmac),hdr2,sizeof(hdr_uwvb));

  rHeader.SetPtype(P_DATA);
  rHeader.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  rHeader.SetPktNum( m_numSend);
  rHeader.SetDataNum(m_numData);
  rHeader.SetBlockNum(m_numBlock);

  m_numSend++;
  m_numData++;

  //size()=large_packet_size_;
  asHeader.SetNextHop(receiver);
  asHeader.SetDirection(AquaSimHeader::DOWN);
  //addr_type()=NS_AF_ILINK;
  ptag.SetPacketType(AquaSimPtTag::PT_RMAC);

  Time totalTxTime = Seconds( ((asHeader.GetSerializedSize() + rHeader.GetSerializedSize()) * m_encodingEfficiency
 			+ m_phyOverhead) / m_bitRate );
  asHeader.SetTxTime(totalTxTime);

  NS_LOG_INFO("AquaSimRMac:TxData node " << m_device->GetAddress() << " at time " <<
	      Seconds(Simulator::Now()) << " packet data_num=" <<
	      rHeader.GetDataNum() << " class data_num=" << m_numData);
  TransmissionStatus status=m_device->TransmissionStatus();

  pkt->AddHeader(asHeader);
  pkt->AddHeader(rHeader);
  pkt->AddPacketTag(ptag);

  if(NIDLE==status)
    {
      m_device->SetTransmissionStatus(SEND);
      SendDown(pkt);
      NS_LOG_INFO("AquaSimRMac:node " << m_device->GetAddress() << " TxData at " << Seconds(Simulator::Now()));
      m_device->SetTransmissionStatus(NIDLE);
      Simulator::Schedule(totalTxTime, &AquaSimRMac::StatusProcess, this, m_device->TransmissionStatus());
    }
  if(RECV==status)
    {
      InterruptRecv(totalTxTime.GetDouble());
      SendDown(pkt);
      NS_LOG_INFO("AquaSimRMac:node " << m_device->GetAddress() << " TxData at " << Seconds(Simulator::Now()));
      m_device->SetTransmissionStatus(NIDLE);
      Simulator::Schedule(totalTxTime, &AquaSimRMac::StatusProcess, this, m_device->TransmissionStatus());
    }
  if (SEND==status)
    {
      NS_LOG_INFO("AquaSimRMac:TxData: queue send data too fast");
      pkt=0;
    }

  if (m_txBuffer.IsEnd())
    {
      NS_LOG_INFO("AquaSimRMac:node " << m_device->GetAddress() << " is in state MAC_WAIT_ACKDATA");
      m_macStatus=RMAC_WAIT_ACKDATA;

      //  double txtime=Timer*PeriodInterval_;
      double txtime=3*m_periodInterval;
      NS_LOG_INFO("AquaSimRMac:node " << m_device->GetAddress() << " TxData at " << Seconds(Simulator::Now()));
      m_timeoutEvent = Simulator::Schedule(Seconds(txtime), &AquaSimRMac::ResetMacStatus, this);

      //  num_block++;
      PowerOff();
    }
  else
    {
      double it=m_SIF+totalTxTime.GetDouble();
      NS_LOG_INFO("AquaSimRMac:node " << m_device->GetAddress() << " schedule  next data packet , interval=" <<
		  it << " at time " << Seconds(Simulator::Now()));
      m_transmissionEvent = Simulator::Schedule(Seconds(it), &AquaSimRMac::TxData, this, m_transReceiver);
    }
}


void
AquaSimRMac::ResumeTxProcess()
{
  NS_LOG_FUNCTION(this << Seconds(Simulator::Now()));

  if(!m_txBuffer.IsFull())
    {
          //if(callback_) callback_->handle(&status_event);
    }
}

/*
 this program is used to handle the transmitted packet,
it should be virtual function, different class may have
different versions.
*/
void
AquaSimRMac::TxProcess(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());

  if (m_device->m_setHopStatus)
    {
      AquaSimHeader asHeader;
      pkt->RemoveHeader(asHeader);

      asHeader.SetNextHop(AquaSimAddress(m_device->GetNextHop()) );
      asHeader.SetErrorFlag(false); //set off the error flag
      // printf("rmac:TxProcess: node %d set next hop to %d\n",index_,cmh->next_hop());
    }

  m_txBuffer.AddNewPacket(pkt);
  NS_LOG_INFO("AquaSimRMac:TxProcess: node " << m_device->GetAddress() << " put new data packets in txbuffer");
  if(!m_txBuffer.IsFull())
    {
        //if(callback_) callback_->handle(&status_event);
    }
  return;
}

void
AquaSimRMac::StatusProcess(TransmissionStatus state)
{
  NS_LOG_FUNCTION(this << m_device->GetAddress());

  if(state==SLEEP) PowerOff();
}

} // namespace ns3
