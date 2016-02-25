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

#include "aqua-sim-mac-goal.h"
#include "aqua-sim-header.h"

#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/mobility-model.h"


//#include "vbf/vectorbasedforward.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimGoal");
NS_OBJECT_ENSURE_REGISTERED(AquaSimGoal);


int AquaSimGoal::m_reqPktSeq = 0;


//---------------------------------------------------------------------
void AquaSimGoal_BackoffTimer::expire()
{
	mac_->ProcessBackoffTimeOut(this);
}


//---------------------------------------------------------------------
void AquaSimGoal_PreSendTimer::expire()
{
	mac_->ProcessPreSendTimeout(this);
}

void
AquaSimGoal_AckTimeoutTimer::expire()
{
  mac_->ProcessAckTimeout(this);
}

////---------------------------------------------------------------------
//void GOAL_RepTimeoutTimer::expire(Event *e)
//{
//	mac_->processRepTimeout(this);
//}


//---------------------------------------------------------------------
void AquaSimGoalDataSendTimer::expire()
{
	mac_->ProcessDataSendTimer(this);
}


//---------------------------------------------------------------------
void AquaSimGoal_SinkAccumAckTimer::expire()
{
	mac_->ProcessSinkAccumAckTimeout();
}


void AquaSimGoal_NxtRoundTimer::expire()
{
	mac_->ProcessNxtRoundTimeout();
}

//---------------------------------------------------------------------
AquaSimGoal::AquaSimGoal(): m_maxBurst(1), m_dataPktInterval(0.0001), m_guardTime(0.05),
	m_TSQ(Seconds(0.01), Seconds(1)), m_maxRetransTimes(6),
	SinkAccumAckTimer(this), m_sinkSeq(0), m_qsPktNum(0),
	m_nxtRoundTimer(this)
{
	m_estimateError=Seconds(0.005);
	m_recvedListAliveTime = Seconds(100.0);
	m_nxtRoundMaxWaitTime = Seconds(1.0);
	m_propSpeed = 1500.0;
	m_isForwarding = false;
	m_txRadius = 3000.0; //static for now... UnderwaterChannel::Transmit_distance();
	m_maxDelay = Seconds(m_txRadius/m_propSpeed);
	m_pipeWidth = 100.0;
	//m_dataPktSize = 300;   //Byte
	m_backoffType = VBF;

	m_maxBackoffTime = 4*m_maxDelay+m_VBF_MaxDelay*1.5+Seconds(2);
  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
}

AquaSimGoal::~AquaSimGoal()
{
}

TypeId
AquaSimGoal::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimGoal")
      .SetParent<AquaSimMac>()
      .AddConstructor<AquaSimGoal>()
      .AddAttribute("MaxBurst", "The maximum number of packets sent in one burst. default is 5",
	IntegerValue(5),
	MakeIntegerAccessor (&AquaSimGoal::m_maxBurst),
	MakeIntegerChecker<int>())
      .AddAttribute("VBFMaxDelay", "Max delay for VBF.",
	TimeValue(Seconds(2.0)),
	MakeTimeAccessor (&AquaSimGoal::m_VBF_MaxDelay),
	MakeTimeChecker())
      .AddAttribute("MaxRetxTimes", "Max retry times.",
	IntegerValue(6),
	MakeIntegerAccessor (&AquaSimGoal::m_maxRetransTimes),
	MakeIntegerChecker<int>())
    ;
  return tid;
}


//---------------------------------------------------------------------
void AquaSimGoal::StatusProcess()
{
	if( SEND == m_device->TransmissionStatus() ){
		m_device->SetTransmissionStatus(NIDLE);
	}
}

//---------------------------------------------------------------------
void AquaSimGoal::RecvProcess(Ptr<Packet> pkt)
{
	AquaSimHeader ash;
	AlohaHeader mach; //TODO change all AlohaHeader's here to a base case header for mac.
  AquaSimGoalAckHeader goalAckh;
	pkt->PeekHeader(ash);
	pkt->PeekHeader(mach);
  pkt->PeekHeader(goalAckh);

	Address dst = mach.GetDA();

	if( ash.GetErrorFlag() )
	{
		//printf("broadcast:node %d  gets a corrupted packet at  %f\n",index_,NOW);
		/*if(drop_)
			drop_->recv(pkt,"Error/Collision");
		else*/
      pkt=0;

		return;
	}

  //TODO general packet type needs to be supported
	//TODO fix broadcast throughout this file.

	if( dst == m_device->GetAddress() /*|| dst == Address(0xffffffff)*/ ) {  //MAC_BROADCAST;
 /*		switch(cmh->ptype() ) {
		case PT_GOAL_REQ:                               //req is broadcasted, it is also data-ack
			ProcessReqPkt(pkt);          //both for me and overhear
			break;
		case PT_GOAL_REP:                               //unicast, but other's should overhear
			ProcessRepPkt(pkt);
			break;
		case PT_GOAL_ACK:
			if( goalAckh.GetPush() )
				ProcessPSHAckPkt(pkt);
			else
				ProcessAckPkt(pkt);
			break;
		default:
			ProcessDataPkt(pkt);
			//free data if it's not for this node
			//
			return;
			;
		}
*/
	}
	/*packet to other nodes*/
	//else if( cmh->ptype() == PT_GOAL_REP ) {
		/*based on other's ack, reserve
		   timeslot to avoid receive-receive collision*/
		/*ProcessOverhearedRepPkt(pkt);
	}
  */

  pkt=0;
}

//---------------------------------------------------------------------
void AquaSimGoal::TxProcess(Ptr<Packet> pkt)
{
	//this node must be the origin
  AquaSimHeader ash;
  //vbh here too;
  pkt->RemoveHeader(ash);

	//hdr_ip* iph = hdr_ip::access(pkt);
	//schedule immediately after receiving. Or cache first then schedule periodically

  /*
	n->update_position();
	vbh->info.ox = n->X();
	vbh->info.oy = n->Y();
	vbh->info.oz = n->Z(); */

	//ash->size() += sizeof(Address)*2;  //plus the size of MAC header: Source address and destination address
  ash.SetTxTime(GetTxTime(ash.GetSerializedSize()));
  ash.SetNumForwards(0);	//use this area to store number of retrans

	//iph->daddr() = vbh->target_id.addr_;
	//iph->saddr() = index_;
	//suppose Sink has broadcast its position before. To simplify the simulation, we
	//directly get the position of the target node (Sink)
  /*
	UnderwaterSensorNode* tn = (UnderwaterSensorNode*)(Node::get_node_by_address(vbh->target_id.addr_));
	tn->update_position();
	vbh->info.tx = tn->X();
	vbh->info.ty = tn->Y();
	vbh->info.tz = tn->Z();*/

	m_originPktSet[ash.GetUId()] = Simulator::Now();
  pkt->AddHeader(ash);
	Insert2PktQs(pkt);

  //callback to higher level, should be implemented differently
  //Scheduler::instance().schedule(&CallbackHandler,&CallbackEvent, GOAL_CALLBACK_DELAY);
}

//---------------------------------------------------------------------
//packet will be sent DataSendTime seconds later
Ptr<Packet>
AquaSimGoal::MakeReqPkt(std::set<Ptr<Packet> > DataPktSet, Time DataSendTime, Time TxTime)
{
	//m_device->UpdatePosition();  //out of date.
	Ptr<Packet> pkt = Create<Packet>();
  AquaSimHeader ash;
	AlohaHeader mach;
  AquaSimGoalReqHeader goalReqh;
	Ptr<Packet> DataPkt = *(DataPktSet.begin());
	//hdr_uwvb* vbh = hdr_uwvb::access(DataPkt);
	  Ptr<MobilityModel> model = m_device->GetNode()->GetObject<MobilityModel>();

	goalReqh.SetRA(Address()/*(Address)0xffffffff*/); //MAC_BROADCAST;
  goalReqh.SetSA(m_device->GetAddress());
	//goalReqh.SetDA(vbh->target_id.addr_);  //sink address
	m_reqPktSeq++;
  goalReqh.SetReqID(m_reqPktSeq);
  goalReqh.SetSenderPos(model->GetPosition());
	goalReqh.SetSendTime(DataSendTime-Simulator::Now());
	goalReqh.SetTxTime(TxTime);
	//goalReqh.SetSinkPos.setValue(vbh->info.tx, vbh->info.ty, vbh->info.tz);
	//goalReqh.SetSourcePos.setValue(vbh->info.ox, vbh->info.oy, vbh->info.oz);

	//ash ptype() = PT_GOAL_REQ;
	ash.SetDirection(AquaSimHeader::DOWN);
	ash.SetErrorFlag(false);
	ash.SetNextHop(Address()/*(Address)0xffffffff*/); //MAC_BROADCAST
  // ash size() = goalReqh->size(m_backoffType)+(NSADDR_T_SIZE*DataPktSet.size())/8+1;
        //sizeof(Address) = 10 in this case.
	ash.SetTxTime(GetTxTime(goalReqh.size(m_backoffType)+(10*DataPktSet.size())/8+1));
	//ash->addr_type() = NS_AF_ILINK;
	ash.SetTimeStamp(Simulator::Now());

  mach.SetDA(goalReqh.GetRA());
  mach.SetSA(m_device->GetAddress());

  uint32_t size = sizeof(uint)+sizeof(int)*DataPktSet.size();
  uint8_t *data = new uint8_t[size];
  *((uint*)data) = DataPktSet.size();
  data += sizeof(uint);

  AquaSimHeader ashLocal;

	for( std::set<Ptr<Packet> >::iterator pos = DataPktSet.begin();
		pos != DataPktSet.end(); pos++) {
	    (*pos)->PeekHeader(ashLocal);
	    *((int*)data) = ashLocal.GetUId();
	    data += sizeof(int);
	}
  Ptr<Packet> tempPacket = Create<Packet>(data,size);
  pkt->AddAtEnd(tempPacket);
  pkt->AddHeader(ash);
  pkt->AddHeader(mach);
  pkt->AddHeader(goalReqh);
	return pkt;
}


//---------------------------------------------------------------------
void
AquaSimGoal::ProcessReqPkt(Ptr<Packet> ReqPkt)
{
  AquaSimGoalReqHeader goalReqh;
  ReqPkt->PeekHeader(goalReqh);
	int		PktID;
	bool	IsLoop = false;

	AquaSimGoal_AckTimeoutTimer* AckTimeoutTimer = NULL;
	std::set<AquaSimGoal_AckTimeoutTimer*>::iterator pos;
	Ptr<Packet> pkt = Create<Packet>();

	std::set<int> DuplicatedPktSet;
	std::set<int> AvailablePktSet;

	//check duplicated packet in the request.
  uint32_t size = ReqPkt->GetSize();
  uint8_t *data = new uint8_t[size];
  ReqPkt->CopyData(data,size);

	uint PktNum = *((uint*)data);
	data += sizeof(uint);
	for( uint i=0; i<PktNum; i++) {
		PktID = *((int*)data);
		if( m_recvedList.count(PktID) != 0 ) {
			if( m_recvedList[PktID].Sender == goalReqh.GetSA() )
				DuplicatedPktSet.insert(PktID);
		}

		AvailablePktSet.insert(PktID);

		data += sizeof(int);
	}

	if( !DuplicatedPktSet.empty() ) {
		//make Ack packet with PUSH flag and send immediately
		Ptr<Packet> AckPkt = MakeAckPkt(DuplicatedPktSet, true, goalReqh.GetReqID());
    AquaSimHeader ash;
    AckPkt->PeekHeader(ash);
		Time Txtime = GetTxTime(ash.GetSize());
		Time SendTime = m_TSQ.GetAvailableTime(Simulator::Now(), Txtime);
		m_TSQ.Insert(SendTime, SendTime+Txtime);
		PreSendPkt(AckPkt, SendTime-Simulator::Now());
	}

	if( DuplicatedPktSet.size()==PktNum ) {
		//all packets are duplicated, don't need to do further process
		return;  //need to reserve slot for receiving???
	}

	//check if it is implicit ack
	//check the DataPktID carried by the request packet
	std::set<int>::iterator pointer = AvailablePktSet.begin();
	while( pointer != AvailablePktSet.end() ) {
		PktID = *pointer;

		pos = m_ackTimeoutTimerSet.begin();
		while( pos!=m_ackTimeoutTimerSet.end() ) {
			AckTimeoutTimer = *pos;
			if( AckTimeoutTimer->PktSet().count(PktID) != 0 ) {
				pkt = AckTimeoutTimer->PktSet().operator[](PktID);
				pkt=0;
				AckTimeoutTimer->PktSet().erase(PktID);

				IsLoop = true;
			}
			pos++;
		}

		//check if the packets tranmitted later is originated from this node
		//In other words, is there a loop?
		if( m_originPktSet.count(PktID) != 0 ) {
			IsLoop = true;
		}

		pointer++;
	}


	//clear the empty entries
	pos = m_ackTimeoutTimerSet.begin();
	while( pos!=m_ackTimeoutTimerSet.end() ) {
		AckTimeoutTimer = *pos;
		if( AckTimeoutTimer->PktSet().empty() ) {
			if( AckTimeoutTimer->IsRunning() ) {
				AckTimeoutTimer->Cancel();
			}

			m_ackTimeoutTimerSet.erase(pos);
			pos = m_ackTimeoutTimerSet.begin();

			delete AckTimeoutTimer;
			m_isForwarding = false;
		}
		else {
			pos++;
		}
	}


	GotoNxtRound();  //try to send data. prepareDataPkts can check if it should send data pkts

	//start to backoff
	if( !IsLoop && m_TSQ.CheckCollision(Simulator::Now()+goalReqh.GetSendTime(),
              Simulator::Now()+goalReqh.GetSendTime()+goalReqh.GetTxTime()) ) {

		Time BackoffTimeLen = GetBackoffTime(ReqPkt);
		//this node is in the forwarding area.
		if( BackoffTimeLen > 0.0 ) {
			AquaSimGoal_BackoffTimer* backofftimer = new AquaSimGoal_BackoffTimer(this);
      AquaSimGoalRepHeader goalReph;
			Time RepPktTxtime = GetTxTime(goalReph.size(m_backoffType));
			Time RepSendTime = m_TSQ.GetAvailableTime(BackoffTimeLen+Simulator::Now()+
                            JitterStartTime(RepPktTxtime),RepPktTxtime );
			SchedElem* SE = new SchedElem(RepSendTime, RepSendTime+RepPktTxtime);
			//reserve the sending interval for Rep packet
			m_TSQ.Insert(SE);

			backofftimer->ReqPkt() = ReqPkt->Copy();
			backofftimer->SetSE(SE);
			backofftimer->BackoffTime() = BackoffTimeLen;
			backofftimer->SetFunction(&ns3::AquaSimGoal_BackoffTimer::expire,backofftimer);
			backofftimer->Schedule(RepSendTime-Simulator::Now());
			m_backoffTimerSet.insert(backofftimer);
			//avoid send-recv collision or recv-recv collision at this node
			m_TSQ.Insert(Simulator::Now()+goalReqh.GetSendTime(),
				     Simulator::Now()+goalReqh.GetSendTime()+goalReqh.GetTxTime());
			return;
		}
	}

	//reserve time slot for receiving data packet
	//avoid recv-recv collision at this node
	m_TSQ.Insert(Simulator::Now()+goalReqh.GetSendTime(), Simulator::Now()+
                        goalReqh.GetSendTime()+goalReqh.GetTxTime(), true);
}


//---------------------------------------------------------------------
Ptr<Packet>
AquaSimGoal::MakeRepPkt(Ptr<Packet> ReqPkt, Time BackoffTime)
{
	//m_device->UpdatePosition();  //out of date
  AquaSimGoalReqHeader reqPktHeader;
  ReqPkt->PeekHeader(reqPktHeader);
	Ptr<Packet> pkt = Create<Packet>();
  AquaSimHeader ash;
  AlohaHeader mach; //TODO update this.
  AquaSimGoalRepHeader repH;
  Ptr<MobilityModel> model = m_device->GetNode()->GetObject<MobilityModel>();


	repH.SetSA(m_device->GetAddress());
	repH.SetRA(reqPktHeader.GetSA());
	repH.SetReqID(reqPktHeader.GetReqID());
	repH.SetReplyerPos(model->GetPosition());
	repH.SetSendTime(reqPktHeader.GetSendTime());	//update this item when being sent out
	repH.SetBackoffTime(BackoffTime);

	ash.SetDirection(AquaSimHeader::DOWN);
	//ash.ptype() = PT_GOAL_REP;
	ash.SetErrorFlag(false);
	ash.SetNextHop(repH.GetRA());			//reply the sender of request pkt
	//ash.size() = hdr_GOAL_rep::size(m_backoffType);
	//ash.addr_type() = NS_AF_ILINK;
	ash.SetTimeStamp(Simulator::Now());

	mach.SetDA(repH.GetRA());
	mach.SetSA(repH.GetSA());

  pkt->AddHeader(ash);
  pkt->AddHeader(mach);
  pkt->AddHeader(repH);
	return pkt;
}


//---------------------------------------------------------------------
void
AquaSimGoal::ProcessRepPkt(Ptr<Packet> RepPkt)
{
  AquaSimGoalRepHeader repH;
  RepPkt->PeekHeader(repH);
	//here only process the RepPkt for this node( the request sender)
	std::set<AquaSimGoalDataSendTimer*>::iterator pos = m_dataSendTimerSet.begin();

	while( pos != m_dataSendTimerSet.end() ) {

		if( (*pos)->ReqID() == repH.GetReqID() ) {

			if( repH.GetBackoffTime() < (*pos)->MinBackoffTime() ) {
				(*pos)->NxtHop() = repH.GetSA();
				(*pos)->MinBackoffTime() = repH.GetBackoffTime();
				(*pos)->SetRep(true);
			}
			break;
		}

		pos++;
	}
}


//---------------------------------------------------------------------
void
AquaSimGoal::ProcessOverhearedRepPkt(Ptr<Packet> RepPkt)
{
	//process the overheared reply packet
	//if this node received the corresponding req before,
	//cancel the timer
  AquaSimGoalRepHeader repH;
  RepPkt->PeekHeader(repH);
  AquaSimGoalRepHeader repHLocal;
	std::set<AquaSimGoal_BackoffTimer*>::iterator pos = m_backoffTimerSet.begin();
	while( pos != m_backoffTimerSet.end() ) {
	    (*pos)->ReqPkt()->PeekHeader(repHLocal);
		if( repHLocal.GetReqID() == repH.GetReqID()) {

			/*if( repH.GetBackoffTime() < (*pos)->GetBackoffTime() ) {
				if( (*pos).isRunning() ) {
					(*pos).Cancel();
				}
				m_TSQ.remove((*pos)->SE());
				delete (*pos)->SE();
				(*pos)->ReqPkt() =0;
				}*/
			if( (*pos)->IsRunning() ) {
				(*pos)->Cancel();
			}
			//change the type of reserved slot to avoid recv-recv collision at this node
			(*pos)->SE()->IsRecvSlot = true;
			/*m_TSQ.remove((*pos)->SE());
			delete (*pos)->SE();*/
			(*pos)->ReqPkt()=0;
		}
		pos++;
	}

	//reserve time slot for the corresponding Data Sending event.
	//avoid recv-recv collision at neighbors
	Vector ThisNode;
  AquaSimHeader ash;
  RepPkt->PeekHeader(ash);
	//m_device->UpdatePosition();  //out of date
  Ptr<MobilityModel> model = m_device->GetNode()->GetObject<MobilityModel>();
  ThisNode = model->GetPosition();
	Time PropDelay = Seconds(Dist(ThisNode, repH.GetReplyerPos())/m_propSpeed);
	Time BeginTime = Simulator::Now() + (repH.GetSendTime() - 2*PropDelay-ash.GetTxTime() );
	m_TSQ.Insert(BeginTime-m_guardTime, BeginTime+repH.GetTxTime()+m_guardTime);

}


//---------------------------------------------------------------------
void
AquaSimGoal::PrepareDataPkts()
{
	/*if( m_isForwarding || (m_qsPktNum == 0) )
		return;*/
  AquaSimGoalReqHeader reqH;
  AquaSimHeader ash;

	//int		SinkAddr = -1;  //not used.
	Time DataTxTime = Seconds(0);
	Time DataSendTime = Seconds(0);
	Time ReqSendTime = Seconds(0);
	Time ReqPktTxTime = GetTxTime(reqH.size(m_backoffType));
	Ptr<Packet> pkt;
	Ptr<Packet> ReqPkt;
	AquaSimGoalDataSendTimer* DataSendTimer = new AquaSimGoalDataSendTimer(this);
	//GOAL_RepTimeoutTimer* RepTimeoutTimer = new GOAL_RepTimeoutTimer(this);

	if( m_PktQs.size() == 0 ) {
    NS_LOG_WARN("PrepareDataPkts: size of m_PktQs should not be 0, something must be wrong");
	}

	std::map<Address, AquaSimGoal_PktQ>::iterator pos = m_PktQs.begin();
	for(int i=0; i<m_sinkSeq; i++) {
		pos++;
	}
	m_sinkSeq = (m_sinkSeq+1)%m_PktQs.size();

	for( int i=0; i<m_maxBurst && (!m_PktQs[pos->first].Q_.empty()); i++) {
		pkt = m_PktQs[pos->first].Q_.front();
		DataSendTimer->DataPktSet().insert(pkt);
		m_PktQs[pos->first].Q_.pop_front();
		--m_qsPktNum;
    pkt->PeekHeader(ash);
		DataTxTime += ash.GetTxTime() + m_dataPktInterval;
	}
	//additional m_dataPktInterval is considered, subtract it.
	DataTxTime -= m_dataPktInterval;

	//erase empty entry
	if( m_PktQs[pos->first].Q_.empty() )
		m_PktQs.erase(pos->first);



	ReqPkt = MakeReqPkt(DataSendTimer->DataPktSet(), DataSendTime, DataTxTime);
  AquaSimHeader ashLocal;
  ReqPkt->PeekHeader(ashLocal);
	ReqPktTxTime = GetTxTime(ashLocal.GetSize());

	ReqSendTime = m_TSQ.GetAvailableTime(Simulator::Now()+JitterStartTime(ReqPktTxTime), ReqPktTxTime);
	m_TSQ.Insert(ReqSendTime, ReqSendTime+ReqPktTxTime); //reserve time slot for sending REQ

	//reserve time slot for sending DATA
	DataSendTime = m_TSQ.GetAvailableTime(Simulator::Now()+m_maxBackoffTime+2*m_maxDelay+m_estimateError, DataTxTime, true);
	DataSendTimer->SetSE( m_TSQ.Insert(DataSendTime, DataSendTime+DataTxTime) );

  ReqPkt->PeekHeader(reqH);
	DataSendTimer->SetReqID(reqH.GetReqID());
	DataSendTimer->TxTime() = DataTxTime;
	DataSendTimer->MinBackoffTime() = Seconds(100000.0);
	DataSendTimer->SetRep(false);

	//send REQ
	PreSendPkt(ReqPkt, ReqSendTime-Simulator::Now());

	DataSendTimer->SetFunction(&AquaSimGoalDataSendTimer::expire,DataSendTimer);
	DataSendTimer->Schedule(DataSendTime - Simulator::Now());
	m_dataSendTimerSet.insert(DataSendTimer);
}

//---------------------------------------------------------------------
void
AquaSimGoal::SendDataPkts(std::set<Ptr<Packet> > DataPktSet, Address NxtHop, Time TxTime)
{
	std::set<Ptr<Packet> >::iterator pos = DataPktSet.begin();
	Time DelayTime = Seconds(0.00001);  //the delay of sending data packet
  AquaSimHeader ash;
  AlohaHeader mach;   //TODO change.
	AquaSimGoal_AckTimeoutTimer* AckTimeoutTimer = new AquaSimGoal_AckTimeoutTimer(this);

	while( pos != DataPktSet.end() ) {
    (*pos)->RemoveHeader(ash);
    (*pos)->RemoveHeader(mach);
    ash.SetNextHop(NxtHop);
    mach.SetDA(NxtHop);
    mach.SetSA(m_device->GetAddress());
    (*pos)->AddHeader(ash);
    (*pos)->AddHeader(mach);

		PreSendPkt((*pos)->Copy(), DelayTime);

		AckTimeoutTimer->PktSet().operator[](ash.GetUId()) = (*pos);

		DelayTime += m_dataPktInterval + ash.GetTxTime();
		pos++;
	}

  AckTimeoutTimer->SetFunction(&AquaSimGoal_AckTimeoutTimer::expire, AckTimeoutTimer);
  AckTimeoutTimer->Schedule(2*m_maxDelay+TxTime+this->m_nxtRoundMaxWaitTime+m_estimateError+MilliSeconds(0.5));
	m_ackTimeoutTimerSet.insert(AckTimeoutTimer);
}


//---------------------------------------------------------------------
void
AquaSimGoal::ProcessDataPkt(Ptr<Packet> DataPkt)
{
  AquaSimHeader ash;
  AlohaHeader mach;   //TODO update this...
  DataPkt->RemoveHeader(ash);
  DataPkt->PeekHeader(mach);
	//hdr_uwvb* vbh = hdr_uwvb::access(DataPkt);

	if( m_recvedList.count(ash.GetUId()) != 0 ) {
		//duplicated packet, free it.
		DataPkt=0;
		return;
	}

	PurifyRecvedList();
	m_recvedList[ash.GetUId()].RecvTime = Simulator::Now();
	m_recvedList[ash.GetUId()].Sender = mach.GetSA();

	if( /* TODO update*/ /*vbh->target_id.addr_ ==*/ ash.GetDAddr() == m_device->GetAddress() ) {
		//ack this data pkt if this node is the destination
		//ack!!! insert the uid into AckSet.
		if( SinkAccumAckTimer.IsRunning() ) {
			SinkAccumAckTimer.Cancel();
		}

    SinkAccumAckTimer.SetFunction(&AquaSimGoal_SinkAccumAckTimer::expire,&SinkAccumAckTimer);
    SinkAccumAckTimer.Schedule(ash.GetTxTime()+ m_dataPktInterval*2);
		SinkAccumAckTimer.AckSet().insert( ash.GetUId() );

		if( m_sinkRecvedList.count(ash.GetUId()) != 0 )
			DataPkt=0;
		else {
			m_sinkRecvedList.insert(ash.GetUId());
			//ash->size() -= sizeof(Address)*2;
			SendUp(DataPkt);
		}

	}
	else {
		//forward this packet later
    ash.SetNumForwards(0);
		Insert2PktQs(DataPkt);
	}
}

//---------------------------------------------------------------------
Ptr<Packet>
AquaSimGoal::MakeAckPkt(std::set<int> AckSet, bool PSH,  int ReqID)
{
	Ptr<Packet> pkt = Create<Packet>();
  AquaSimHeader ash;
  AlohaHeader mach;   //TODO change this...
  AquaSimGoalAckHeader goalAckh;

	goalAckh.SetSA(m_device->GetAddress());
	goalAckh.SetRA(Address()/*(Address)0xffffffff*/);  //MAC_BROADCAST
	goalAckh.SetPush(PSH);
	if( PSH ) {
		goalAckh.SetReqID(ReqID);
	}

	//ash.ptype() = PT_GOAL_ACK;
	ash.SetDirection(AquaSimHeader::DOWN);
	ash.SetErrorFlag(false);
	ash.SetNextHop(goalAckh.GetRA());   //reply the sender of request pkt
//	ash.size() = goalAckh size() + (NSADDR_T_SIZE*AckSet.size())/8+1;
	//ash.addr_type() = NS_AF_ILINK;

	mach.SetDA(goalAckh.GetRA());
	mach.SetSA(goalAckh.GetSA());

	uint32_t size = sizeof(int)*AckSet.size()+sizeof(uint);
	uint8_t *data = new uint8_t[size];

	*((uint*)data) = AckSet.size();
	data += sizeof(uint);

	for( std::set<int>::iterator pos=AckSet.begin();
		pos != AckSet.end(); pos++)   {
	    *((int*)data) = *pos;
	    data += sizeof(int);
	}
  Ptr<Packet> tempPacket = Create<Packet>(data,size);
  pkt->AddAtEnd(tempPacket);
  pkt->AddHeader(goalAckh);
  pkt->AddHeader(ash);
  pkt->AddHeader(mach);
  return pkt;
}

//---------------------------------------------------------------------
void
AquaSimGoal::ProcessAckPkt(Ptr<Packet> AckPkt)
{
	//cancel the Ack timeout timer and release data packet.
	int PktID;
	AquaSimGoal_AckTimeoutTimer* AckTimeoutTimer;
	Ptr<Packet> pkt;
	std::set<AquaSimGoal_AckTimeoutTimer*>::iterator pos;

	//check the DataPktID carried by the ack packet
  uint32_t size = AckPkt->GetSize();
  uint8_t *data = new uint8_t[size];
  AckPkt->CopyData(data,size);
	uint PktNum = *((uint*)data);
	data += sizeof(uint);

	for( uint i=0; i < PktNum; i++) {
		PktID = *((int*)data);

		pos = m_ackTimeoutTimerSet.begin();

		while( pos != m_ackTimeoutTimerSet.end() ) {
			AckTimeoutTimer = *pos;
			if( AckTimeoutTimer->PktSet().count(PktID) != 0 ) {
				pkt = AckTimeoutTimer->PktSet().operator[](PktID);
				pkt=0;
				AckTimeoutTimer->PktSet().erase(PktID);
			}
			pos++;
		}

		data += sizeof(int);
	}

	//clear the empty entries
	pos = m_ackTimeoutTimerSet.begin();
	while( pos != m_ackTimeoutTimerSet.end() ) {
		AckTimeoutTimer = *pos;

		if( AckTimeoutTimer->PktSet().empty() ) {
			//all packet are acked
			if( AckTimeoutTimer->IsRunning() ) {
				AckTimeoutTimer->Cancel();
			}

			m_ackTimeoutTimerSet.erase(pos);
			pos = m_ackTimeoutTimerSet.begin();
			delete AckTimeoutTimer;

			m_isForwarding = false;
		}
		else {
			pos++;
		}
	}

	GotoNxtRound();
}


//---------------------------------------------------------------------
void
AquaSimGoal::ProcessPSHAckPkt(Ptr<Packet> AckPkt)
{
  AquaSimGoalAckHeader goalAckh;
  AckPkt->PeekHeader(goalAckh);
	int ReqID = goalAckh.GetReqID();
	AquaSimGoalDataSendTimer* DataSendTimer=NULL;
	std::set<AquaSimGoalDataSendTimer*>::iterator pos = m_dataSendTimerSet.begin();

	while( pos != m_dataSendTimerSet.end() ) {
		if( (*pos)->ReqID() == ReqID ) {
			DataSendTimer = *pos;
			break;
		}
		pos++;
	}

	if( DataSendTimer != NULL ) {
		//the reply is from this node??
		//check the duplicated DataPktID carried by the ack packet
    uint32_t size = AckPkt->GetSize();
    uint8_t *data = new uint8_t[size];
    AckPkt->CopyData(data,size);
		uint PktNum = *((uint*)data);
		data += sizeof(uint);

		for(uint i=0; i<PktNum; i++) {
			std::set<Ptr<Packet> >::iterator pointer = DataSendTimer->DataPktSet().begin();
			while( pointer != DataSendTimer->DataPktSet().end() ) {
        AquaSimHeader ash;
        (*pointer)->PeekHeader(ash);
				if( ash.GetUId() == *((int*)data) ) {
					DataSendTimer->DataPktSet().erase(*pointer);
					break;
				}
				pointer++;
			}

			data+= sizeof(int);
		}

		if( DataSendTimer->DataPktSet().empty() ) {
			if( DataSendTimer->IsRunning() ) {
				DataSendTimer->Cancel();
			}
			m_TSQ.Remove(DataSendTimer->SE());

			m_dataSendTimerSet.erase(DataSendTimer);
			delete DataSendTimer;

			m_isForwarding = false;
			GotoNxtRound();
		}

		return;
	}

}


//---------------------------------------------------------------------
void
AquaSimGoal::Insert2PktQs(Ptr<Packet> DataPkt, bool FrontPush)
{
	//hdr_uwvb* vbh = hdr_uwvb::access(DataPkt);
  AquaSimHeader ash;
  DataPkt->RemoveHeader(ash);

	if( ash.GetNumForwards() > m_maxRetransTimes ) {
		DataPkt=0;
		return;
	}
	else {
	    uint8_t tmp = ash.GetNumForwards();
	    tmp++;
	    ash.SetNumForwards(tmp);	//bit awkward.
    DataPkt->AddHeader(ash);
	}
	/* vbh not implemented yet
	if( FrontPush )
	  //[vbh->target_id.addr_].Q_.push_front(DataPkt);
	else
	  //m_PktQs[vbh->target_id.addr_].Q_.push_back(DataPkt);
	*/
	m_qsPktNum++;

	GotoNxtRound();
	////how to control??????
	//if( m_qsPktNum==1 ) {  /*trigger forwarding if Queue is empty*/
	//	prepareDataPkts(vbh->target_id.addr_);
	//}
	//else if( SendOnebyOne  ) {
	//	if( !m_isForwarding )
	//		prepareDataPkts(vbh->target_id.addr_);
	//}
	//else{
	//	prepareDataPkts(vbh->target_id.addr_);
	//}
}

//---------------------------------------------------------------------
void
AquaSimGoal::ProcessBackoffTimeOut(AquaSimGoal_BackoffTimer *backoff_timer)
{
	Ptr<Packet> RepPkt = MakeRepPkt(backoff_timer->ReqPkt(),
								backoff_timer->BackoffTime());

	/* The time slot for sending reply packet is
	 * already reserved when processing request packet,
	 * so RepPkt is directly sent out.
	 */
	SendoutPkt(RepPkt);
	backoff_timer->ReqPkt()=0;
	m_backoffTimerSet.erase(backoff_timer);
	delete backoff_timer;
}

//---------------------------------------------------------------------
void
AquaSimGoal::ProcessDataSendTimer(AquaSimGoalDataSendTimer *DataSendTimer)
{
	if( !DataSendTimer->GotRep() ) {
		//if havenot gotten reply, resend the packet
		std::set<Ptr<Packet> >::iterator pos = DataSendTimer->DataPktSet().begin();
		while( pos != DataSendTimer->DataPktSet().end() ) {
			Insert2PktQs(*pos, true); //although GotoNxtRound() is called, it does not send ReqPkt
			pos++;
		}

		DataSendTimer->DataPktSet().clear();

		m_isForwarding = false;
		GotoNxtRound();   //This call is successful, so all packets will be sent together.
	}
	else {
		SendDataPkts(DataSendTimer->DataPktSet(),
				DataSendTimer->NxtHop(), DataSendTimer->TxTime());
		DataSendTimer->DataPktSet().clear();
	}

	m_dataSendTimerSet.erase(DataSendTimer);
	delete DataSendTimer;
}

//---------------------------------------------------------------------
void
AquaSimGoal::ProcessSinkAccumAckTimeout()
{
	Ptr<Packet> AckPkt = MakeAckPkt( SinkAccumAckTimer.AckSet() );
	SinkAccumAckTimer.AckSet().clear();
  AquaSimHeader ash;
  AckPkt->PeekHeader(ash);
	Time AckTxtime = GetTxTime(ash.GetSize());
	Time AckSendTime = m_TSQ.GetAvailableTime(Simulator::Now()+JitterStartTime(AckTxtime), AckTxtime);
	m_TSQ.Insert(AckSendTime, AckSendTime+AckTxtime);
	PreSendPkt(AckPkt, AckSendTime-Simulator::Now());
}

//---------------------------------------------------------------------
void
AquaSimGoal::ProcessPreSendTimeout(AquaSimGoal_PreSendTimer* PreSendTimer)
{
	SendoutPkt(PreSendTimer->Pkt());
	PreSendTimer->Pkt() = NULL;
	m_preSendTimerSet.erase(PreSendTimer);
	delete PreSendTimer;
}

//---------------------------------------------------------------------
void
AquaSimGoal::ProcessAckTimeout(AquaSimGoal_AckTimeoutTimer *AckTimeoutTimer)
{
	//SentPktSet.erase( HDR_CMN(AckTimeoutTimer->pkt())->uid() );
	//Insert2PktQs(AckTimeoutTimer->pkt(), true);
	//AckTimeoutTimer->pkt() = NULL;
	//delete AckTimeoutTimer;
	//m_isForwarding = false;
	//prepareDataPkts();  //???? how to do togther???
	//std::set<Ptr<Packet>>::iterator pos = AckTimeoutTimer->DataPktSet().begin();
	//while( pos != AckTimeoutTimer->DataPktSet().end() ) {
	//	Insert2PktQs(*pos, true);  //right??
	//	pos++;
	//}
	//m_ackTimeoutTimerSet.erase(AckTimeoutTimer);
	//delete AckTimeoutTimer;

	std::map<int, Ptr<Packet> >::iterator pos = AckTimeoutTimer->PktSet().begin();
	while( pos != AckTimeoutTimer->PktSet().end() ) {
		Insert2PktQs(pos->second, true);
		pos++;
	}

	AckTimeoutTimer->PktSet().clear();
	m_ackTimeoutTimerSet.erase(AckTimeoutTimer);
	delete AckTimeoutTimer;

	m_isForwarding = false;
	GotoNxtRound();
}


////---------------------------------------------------------------------
//void AquaSimGoal::processRepTimeout(GOAL_RepTimeoutTimer *RepTimeoutTimer)
//{
//	AquaSimGoalDataSendTimer* DataSendTimer = RepTimeoutTimer->DataSendTimer();
//
//
//	if( !RepTimeoutTimer->GotRep() ) {
//		//if havenot gotten reply, stop datasend timer
//		if( DataSendTimer->status() == TIMER_PENDING ) {
//			DataSendTimer->cancel();
//		}
//
//		std::set<Ptr<Packet>>::iterator pos = DataSendTimer->DataPktSet().begin();
//		while( pos != DataSendTimer->DataPktSet().end() ) {
//			Insert2PktQs(*pos, true); //right??
//			pos++;
//		}
//		m_dataSendTimerSet.erase(DataSendTimer);
//	}
//	RepTimeoutTimerSet_.erase(RepTimeoutTimer);
//	delete RepTimeoutTimer;
//	m_isForwarding = false;
//}


//---------------------------------------------------------------------
void
AquaSimGoal::PreSendPkt(Ptr<Packet> pkt, Time delay)
{
	AquaSimGoal_PreSendTimer* PreSendTimer = new AquaSimGoal_PreSendTimer(this);
	PreSendTimer->Pkt() = pkt;
  PreSendTimer->SetFunction(&AquaSimGoal_PreSendTimer::expire,PreSendTimer);
  PreSendTimer->Schedule(delay);
	m_preSendTimerSet.insert(PreSendTimer);
}


//---------------------------------------------------------------------
void
AquaSimGoal::SendoutPkt(Ptr<Packet> pkt)
{
  AquaSimHeader ash;
  pkt->RemoveHeader(ash);

	ash.SetTxTime(GetTxTime(ash.GetSize()));

	//update the the Time for sending out the data packets.

	Time txtime=ash.GetTxTime();

	switch( m_device->TransmissionStatus() ) {
		case SLEEP:
			PowerOn();
		case RECV:
			//interrupt reception
			InterruptRecv(txtime.ToDouble(Time::S));
		case NIDLE:
			m_device->SetTransmissionStatus(SEND);
      //TODO support packet type within common as header.
      /*switch( cmh->ptype() ) {
				case PT_GOAL_REQ:
					{
						hdr_GOAL_req* vmq = hdr_GOAL_req::access(pkt);
						vmq->SendTime_ -= Simulator::Now() - cmh->timestamp();
					}
					break;
				case PT_GOAL_REP:
					{
						hdr_GOAL_rep* vmp = hdr_GOAL_rep::access(pkt);
						vmp->SendTime_ -= Simulator::Now() - cmh->timestamp();
					}
					break;
				default:
					;
			}*/

			ash.SetTimeStamp(Simulator::Now());
			ash.SetDirection(AquaSimHeader::DOWN);
      pkt->AddHeader(ash);
			SendDown(pkt);
      Simulator::Schedule(txtime, &AquaSimGoal::StatusProcess, this);
			break;

		//case RECV:
		//	//backoff???
		//	Packet::free(pkt);
		//	break;
		//
		default:
			//status is SEND
      NS_LOG_INFO("SendoutPkt:Node=" << m_device->GetNode() << " send data too fast");
			pkt=0;
	}

	return;
}

//---------------------------------------------------------------------
double
AquaSimGoal::Dist(Vector3D Pos1, Vector3D Pos2)
{
  //TODO should probably be integrated using something else
  //return m_device->GetChannel()->Distance(device1, device2);

	double delta_x = Pos1.x - Pos2.x;
	double delta_y = Pos1.y - Pos2.y;
	double delta_z = Pos1.z - Pos2.z;

	return sqrt(delta_x*delta_x + delta_y*delta_y + delta_z*delta_z);
}


//---------------------------------------------------------------------
Time
AquaSimGoal::GetBackoffTime(Ptr<Packet> ReqPkt)
{
  AquaSimGoalReqHeader goalReqh;
  ReqPkt->PeekHeader(goalReqh);
	double BackoffTime;

	switch( m_backoffType ) {
		case VBF:
			BackoffTime = GetVBFbackoffTime(goalReqh.GetSourcePos(),
              goalReqh.GetSenderPos(), goalReqh.GetSinkPos());
			break;
		case HH_VBF:
			BackoffTime = GetHH_VBFbackoffTime(goalReqh.GetSenderPos(), goalReqh.GetSinkPos());
			break;
		default:
      NS_LOG_WARN("No such backoff type");
			exit(0);
	}

	return MilliSeconds(BackoffTime);
}

//---------------------------------------------------------------------
//Backoff Functions
double
AquaSimGoal::GetVBFbackoffTime(Vector3D Source, Vector3D Sender, Vector3D Sink)
{
	double DTimesCosTheta =0.0;
	double p = 0.0;;
	double alpha = 0.0;
	Vector3D ThisNode;
	//m_device->UpdatePosition();  //out of date
	  Ptr<MobilityModel> model = m_device->GetNode()->GetObject<MobilityModel>();
	ThisNode = model->GetPosition();

	if( Dist(Sender, Sink) < Dist(ThisNode, Sink) )
		return -1.0;

	DTimesCosTheta = DistToLine(Sender, Sink);
	p = DistToLine(Source, Sink);

	if( p > m_pipeWidth )
		return -1.0;

	alpha = p/m_pipeWidth + (m_txRadius-DTimesCosTheta)/m_txRadius;

	return m_VBF_MaxDelay.ToDouble(Time::MS)*sqrt(alpha)+2*(m_txRadius-Dist(Sender, ThisNode))/m_propSpeed;
}

//---------------------------------------------------------------------
double
AquaSimGoal::GetHH_VBFbackoffTime(Vector3D Sender, Vector3D Sink)
{
	return GetVBFbackoffTime(Sender, Sender, Sink);
}


//---------------------------------------------------------------------
//Line Point1 and Line Point2 is two points on the line
double
AquaSimGoal::DistToLine(Vector3D LinePoint1, Vector3D LinePoint2)
{
  Vector3D ThisNode;
  //m_device->UpdatePosition();  //out of date
  Ptr<MobilityModel> model = m_device->GetNode()->GetObject<MobilityModel>();
  ThisNode = model->GetPosition();
	double P1ThisNodeDist = Dist(LinePoint1, ThisNode);
	double P1P2Dist = Dist(LinePoint1, LinePoint2);
	double ThisNodeP2Dist = Dist(ThisNode, LinePoint2);
	double CosTheta = (P1ThisNodeDist*P1ThisNodeDist +
				P1P2Dist*P1P2Dist -
				ThisNodeP2Dist*ThisNodeP2Dist)/(2*P1ThisNodeDist*P1P2Dist);
	//the distance from this node to the pipe central axis
	return P1ThisNodeDist*sqrt(1-CosTheta*CosTheta);
}


//---------------------------------------------------------------------
void
AquaSimGoal::PurifyRecvedList()
{
	std::map<int, RecvedInfo> tmp(m_recvedList.begin(), m_recvedList.end());
	std::map<int, RecvedInfo>::iterator pos;
	m_recvedList.clear();

	for( pos=tmp.begin(); pos!=tmp.end(); pos++) {
		if( pos->second.RecvTime > Simulator::Now()-m_recvedListAliveTime )
			m_recvedList.insert(*pos);
	}
}


//---------------------------------------------------------------------
void
AquaSimGoal::ProcessNxtRoundTimeout()
{
	PrepareDataPkts();
}

//---------------------------------------------------------------------
void
AquaSimGoal::GotoNxtRound()
{
	if( m_isForwarding || (m_nxtRoundTimer.IsRunning()) || (m_qsPktNum==0) )
		return;

	m_isForwarding = true;

  m_nxtRoundTimer.SetFunction(&AquaSimGoal_NxtRoundTimer::expire, &m_nxtRoundTimer);
  m_nxtRoundTimer.Schedule(FemtoSeconds(m_rand->GetValue(0.0,m_nxtRoundMaxWaitTime.ToDouble(Time::S) ) ) );
}

//---------------------------------------------------------------------


Time
AquaSimGoal::JitterStartTime(Time Txtime)
{
	Time BeginTime = 5*Txtime*m_rand->GetValue();
	return BeginTime;
}

} // namespace ns3



//---------------------------------------------------------------------
ns3::SchedElem::SchedElem(Time BeginTime_, Time EndTime_, bool IsRecvSlot_)
{
	BeginTime = BeginTime_;
	EndTime = EndTime_;
	IsRecvSlot = IsRecvSlot_;
}

//---------------------------------------------------------------------
ns3::SchedElem::SchedElem(SchedElem& e)
{
	BeginTime = e.BeginTime;
	EndTime = e.EndTime;
}


//---------------------------------------------------------------------
ns3::TimeSchedQueue::TimeSchedQueue(Time MinInterval, Time BigIntervalLen)
{
	m_minInterval = MinInterval;
	m_bigIntervalLen = BigIntervalLen;
}

//---------------------------------------------------------------------
ns3::SchedElem*
ns3::TimeSchedQueue::Insert(SchedElem *e)
{
	std::list<SchedElem*>::iterator pos = m_SchedQ.begin();
	while( pos != m_SchedQ.end() ) {

		if( (*pos)->BeginTime < e->BeginTime )
			break;

		pos++;
	}

	if( pos == m_SchedQ.begin() ) {
		m_SchedQ.push_front(e);
	}
	else if( pos == m_SchedQ.end() ) {
		m_SchedQ.push_back(e);
	}
	else{
		m_SchedQ.insert(pos, e);
	}

	return e;
}


//---------------------------------------------------------------------
ns3::SchedElem*
ns3::TimeSchedQueue::Insert(Time BeginTime, Time EndTime, bool IsRecvSlot)
{
	SchedElem* e = new SchedElem(BeginTime, EndTime, IsRecvSlot);
	return Insert(e);
}


//---------------------------------------------------------------------
void
ns3::TimeSchedQueue::Remove(SchedElem *e)
{
	m_SchedQ.remove(e);
	delete e;
}


//---------------------------------------------------------------------
ns3::Time
ns3::TimeSchedQueue::GetAvailableTime(Time EarliestTime, Time SlotLen, bool BigInterval)
{
	/*
	 * how to select the sending time is critical
	 * random in the availabe time interval
	 */
	ClearExpiredElems();
	Time MinStartInterval = Seconds(0.1);
	Time Interval = Seconds(0.0);
	if( BigInterval ) {
		Interval = m_bigIntervalLen;	//Big Interval is for DATA packet.
		MinStartInterval = Seconds(0);		//DATA packet does not need to Jitter the start time.
	}
	else {
		Interval = m_minInterval;
	}

	Time LowerBeginTime = EarliestTime;
	Time UpperBeginTime = Seconds(-1.0);   //infinite;
	std::list<SchedElem*>::iterator pos = m_SchedQ.begin();


	while( pos != m_SchedQ.end() ) {
		while( pos!=m_SchedQ.end() && (*pos)->IsRecvSlot ) {
			pos++;
		}

		if( pos == m_SchedQ.end() ) {
			break;
		}

		if( (*pos)->BeginTime - LowerBeginTime > SlotLen+Interval+MinStartInterval ) {
			break;
		}
		else {
			LowerBeginTime = std::max((*pos)->EndTime, LowerBeginTime);
		}
		pos++;
	}

	UpperBeginTime = LowerBeginTime + MinStartInterval;

  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
  return MilliSeconds(rand->GetValue(LowerBeginTime.ToDouble(Time::MS), UpperBeginTime.ToDouble(Time::MS)));
}


//---------------------------------------------------------------------
bool
ns3::TimeSchedQueue::CheckCollision(Time BeginTime, Time EndTime)
{
	ClearExpiredElems();

	std::list<SchedElem*>::iterator pos = m_SchedQ.begin();
	BeginTime -= m_minInterval;   //consider the guard time
	EndTime += m_minInterval;

	if( m_SchedQ.empty() ) {
		return true;
	}
	else {
		while( pos != m_SchedQ.end() ) {
			if( (BeginTime < (*pos)->BeginTime && EndTime > (*pos)->BeginTime)
				|| (BeginTime<(*pos)->EndTime && EndTime > (*pos)->EndTime ) ) {
				return false;
			}
			else {
				pos++;
			}
		}
		return true;
	}
}


//---------------------------------------------------------------------
void
ns3::TimeSchedQueue::ClearExpiredElems()
{
	SchedElem* e = NULL;
	while( !m_SchedQ.empty() ) {
		e = m_SchedQ.front();
		if( e->EndTime + m_minInterval < Simulator::Now() )
		{
			m_SchedQ.pop_front();
			delete e;
			e = NULL;
		}
		else
			break;
	}
}


//---------------------------------------------------------------------
void
ns3::TimeSchedQueue::Print(char* filename)
{
	//FILE* stream = fopen(filename, "a");
	std::list<SchedElem*>::iterator pos = m_SchedQ.begin();
	while( pos != m_SchedQ.end() ) {
	    std::cout << "Print(" << (*pos)->BeginTime << ", " << (*pos)->EndTime << ")\t";
		//fprintf(stream, "(%f, %f)\t", (*pos)->BeginTime, (*pos)->EndTime);
	    pos++;
	}
	std::cout << "\n";
	//fprintf(stream, "\n");
	//fclose(stream);
}
