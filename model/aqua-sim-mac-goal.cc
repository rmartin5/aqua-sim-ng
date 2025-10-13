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
#include "aqua-sim-header-mac.h"
#include "aqua-sim-pt-tag.h"
#include "aqua-sim-header-routing.h"

#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/mobility-model.h"

#include <stdlib.h>
#include <stdio.h>


#include "aqua-sim-routing-vbf.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimGoal");
NS_OBJECT_ENSURE_REGISTERED(AquaSimGoal);


int AquaSimGoal::m_reqPktSeq = 0;


//---------------------------------------------------------------------
AquaSimGoal_BackoffTimer::~AquaSimGoal_BackoffTimer()
{
	mac_=0;
	m_SE=0;
	m_ReqPkt=0;
}

void AquaSimGoal_BackoffTimer::expire()
{
	mac_->ProcessBackoffTimeOut(this);
}


//---------------------------------------------------------------------
AquaSimGoal_PreSendTimer::~AquaSimGoal_PreSendTimer()
{
	mac_=0;
	m_pkt=0;
}

void AquaSimGoal_PreSendTimer::expire()
{
	mac_->ProcessPreSendTimeout(this);
}

AquaSimGoal_AckTimeoutTimer::~AquaSimGoal_AckTimeoutTimer()
{
	mac_=0;
  for (std::map<int, Ptr<Packet> >::iterator it=m_PktSet.begin(); it!=m_PktSet.end(); ++it)
		it->second=0;
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
AquaSimGoalDataSendTimer::~AquaSimGoalDataSendTimer()
{
	mac_=0;
	m_SE=0;
	m_DataPktSet.clear();
}

void AquaSimGoalDataSendTimer::expire()
{
	mac_->ProcessDataSendTimer(this);
}


//---------------------------------------------------------------------
AquaSimGoal_SinkAccumAckTimer::~AquaSimGoal_SinkAccumAckTimer()
{
	mac_=0;
}

void AquaSimGoal_SinkAccumAckTimer::expire()
{
	mac_->ProcessSinkAccumAckTimeout();
}

AquaSimGoal_NxtRoundTimer::~AquaSimGoal_NxtRoundTimer()
{
	mac_=0;
}

void AquaSimGoal_NxtRoundTimer::expire()
{
	mac_->ProcessNxtRoundTimeout();
}

//---------------------------------------------------------------------
AquaSimGoal::AquaSimGoal(): m_dataPktInterval(0.0001), m_guardTime(0.05),
	m_TSQ(Seconds(0.01), Seconds(1)), m_maxRetransTimes(3),
	SinkAccumAckTimer(this), m_sinkSeq(0), m_qsPktNum(0),
	m_nxtRoundTimer(this)
{
	m_estimateError=Seconds(0.005);
	m_recvedListAliveTime = Seconds(100.0);
	m_nxtRoundMaxWaitTime = Seconds(1.0);
	m_propSpeed = 1500.0;
	m_isForwarding = false;
	m_backoffType = VBF;
//	m_maxBackoffTime = 4 * m_maxDelay + m_VBF_MaxDelay * 1.5 + Seconds(2);
	m_rand = CreateObject<UniformRandomVariable> ();
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
      .AddAttribute ("MaxBurst", "The maximum number of packets sent in one burst. default is 5",
		IntegerValue (5),
		MakeIntegerAccessor (&AquaSimGoal::m_maxBurst),
		MakeIntegerChecker<int>())
      .AddAttribute("MaxRetransTimes","The maximum number of packet retransmission. default is 3",
        IntegerValue(3),
        MakeIntegerAccessor (&AquaSimGoal::m_maxRetransTimes),
        MakeIntegerChecker<int>())
      .AddAttribute ("VBFMaxDelay", "Max delay for VBF.",
		TimeValue (Seconds(2.0)),
		MakeTimeAccessor (&AquaSimGoal::m_VBF_MaxDelay),
		MakeTimeChecker())
	  .AddAttribute ("PipeWidth", "VBF pipeline width.",
		DoubleValue (100),
		MakeDoubleAccessor (&AquaSimGoal::m_pipeWidth),
		MakeDoubleChecker<double>())
	  .AddAttribute ("TxRadius", "Max transmission range",
	  	DoubleValue (1000),
		MakeDoubleAccessor (&AquaSimGoal::m_txRadius),
		MakeDoubleChecker<double>())
	  .AddAttribute ("MaxDelay", "Max transmission delay",
		TimeValue (Seconds(1)),
		MakeTimeAccessor (&AquaSimGoal::m_maxDelay),
		MakeTimeChecker())
	  .AddAttribute ("MaxBackoffTime", "Max backoff time",
		TimeValue (Seconds(5)),
		MakeTimeAccessor (&AquaSimGoal::m_maxBackoffTime),
		MakeTimeChecker())
    ;
  return tid;
}

int64_t
AquaSimGoal::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_rand->SetStream(stream);
  return 1;
}


//---------------------------------------------------------------------
bool AquaSimGoal::RecvProcess(Ptr<Packet> pkt)
{
//	NS_LOG_FUNCTION (this);

	AquaSimHeader ash;
	MacHeader mach;
	AquaSimGoalAckHeader goalAckh;
	AquaSimPtTag ptag;
	pkt->RemoveHeader(ash);
	pkt->PeekHeader(mach);
	pkt->PeekPacketTag(ptag);
	pkt->AddHeader(ash);

	AquaSimAddress dst = mach.GetDA();
//	NS_LOG_FUNCTION (this << "me:" << AquaSimAddress::ConvertFrom(m_device->GetAddress()) << " pkt dst:" << dst);

	if( ash.GetErrorFlag() )
	{
		return false;
	}

	if( dst == m_device->GetAddress() || dst == AquaSimAddress::GetBroadcast() ) {
 		switch(ptag.GetPacketType() ) {
			case AquaSimPtTag::PT_GOAL_REQ: //req is broadcasted, it is also data-ack
				NS_LOG_FUNCTION ("me:" << AquaSimAddress::ConvertFrom(m_device->GetAddress()) << " Pkt Dst:" << dst << " type: GOAL_REQ");
				ProcessReqPkt(pkt);          //both for me and overhear
				break;
			case AquaSimPtTag::PT_GOAL_REP: //unicast, but other's should overhear
				NS_LOG_FUNCTION ("me:" << AquaSimAddress::ConvertFrom(m_device->GetAddress()) << " Pkt Dst:" << dst << " type: GOAL_REP");
				ProcessRepPkt(pkt);
				break;
			case AquaSimPtTag::PT_GOAL_ACK: {
				NS_LOG_FUNCTION ("me:" << AquaSimAddress::ConvertFrom(m_device->GetAddress()) << " Pkt Dst:" << dst << " type: GOAL_ACK");
				pkt->RemoveHeader(ash);
				pkt->RemoveHeader(mach);
				pkt->PeekHeader(goalAckh);
				pkt->AddHeader(mach);
				pkt->AddHeader(ash);
				if( goalAckh.GetPush() )
					ProcessPSHAckPkt(pkt);
				else
					ProcessAckPkt(pkt);
				}
				break;
			default:
				NS_LOG_FUNCTION ("Pkt type: GOAL_DATA");
				ProcessDataPkt(pkt);
				//free data if it's not for this node
				break;
		}
	}
	/*packet to other nodes*/
	else if( ptag.GetPacketType() == AquaSimPtTag::PT_GOAL_REP ) {
		/*based on other's ack, reserve
		   timeslot to avoid receive-receive collision*/
		ProcessOverhearedRepPkt(pkt);
	}
  //pkt=0;
	return true;
}

//---------------------------------------------------------------------
bool AquaSimGoal::TxProcess(Ptr<Packet> pkt)
{
	NS_LOG_FUNCTION (this << "Me=" << AquaSimAddress::ConvertFrom(m_device->GetAddress()));

	//this node must be the origin
	AquaSimHeader ash;
	VBHeader vbh;
	MacHeader mach;
	AquaSimPtTag ptag;
	AquaSimGoalAckHeader goalAckh;
	AquaSimGoalRepHeader goalReph;
	AquaSimGoalReqHeader goalReqh;

	pkt->RemoveHeader(ash);
	pkt->PeekPacketTag(ptag);

	if (ptag.GetPacketType() == 0) { // AquaSimPtTag::PT_UWVB == 0, for VBF
		//new packet from routing layer, should have no mac layer headers yet

		NS_LOG_FUNCTION ("Pkt.ptag==PT_UWVB");
//		ptag.SetPacketType(AquaSimPtTag::PT_GOAL_ACK);
//		pkt->ReplacePacketTag(ptag);
	}
	else {// xia: not DATA
		pkt->RemoveHeader(mach);
		switch(ptag.GetPacketType() ) {
			case AquaSimPtTag::PT_GOAL_REQ:
				NS_LOG_FUNCTION ("Pkt.ptag==PT_GOAL_REQ");
				pkt->RemoveHeader(goalReqh);
				break;
			case AquaSimPtTag::PT_GOAL_REP:
				NS_LOG_FUNCTION ("Pkt.ptag==PT_GOAL_REP");
				pkt->RemoveHeader(goalReph);
				break;
			case AquaSimPtTag::PT_GOAL_ACK:
				NS_LOG_FUNCTION ("Pkt.ptag==PT_GOAL_ACK");
				pkt->RemoveHeader(goalAckh);
				break;
			default:
				break;
		}
	}
//	pkt->RemoveHeader(vbh);// aqua-sim-ng original

	pkt->PeekHeader(vbh); // xia edited

	//hdr_ip* iph = hdr_ip::access(pkt);
	//schedule immediately after receiving. Or cache first then schedule periodically

	//m_device->UpdatePosition();
	Ptr<MobilityModel> model = m_device->GetNode()->GetObject<MobilityModel>();
	vbh.SetExtraInfo_o(Vector(model->GetPosition().x,
				model->GetPosition().y,
				model->GetPosition().z) );

	ash.SetSize(ash.GetSize() + sizeof(AquaSimAddress)*2);  //plus the size of MAC header: Source address and destination address
	ash.SetTxTime(GetTxTime(ash.GetSerializedSize()));
	ash.SetNumForwards(0);	//use this area to store number of retrans

	ash.SetDAddr(vbh.GetTargetAddr());
	ash.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));

	m_originPktSet[ash.GetUId()] = Simulator::Now();

	switch(ptag.GetPacketType() )
	{
		case AquaSimPtTag::PT_GOAL_REQ:
			pkt->AddHeader(goalReqh);
			break;
		case AquaSimPtTag::PT_GOAL_REP:
			pkt->AddHeader(goalReph);
			break;
		case AquaSimPtTag::PT_GOAL_ACK:
			pkt->AddHeader(goalAckh);
			break;
		case AquaSimPtTag::PT_UWVB: // xia: data pkt is different
			mach.SetSA(ash.GetSAddr());
			mach.SetDA(ash.GetNextHop());
			break;
		default:
			break;
	}

	pkt->AddHeader(mach);
	pkt->AddHeader(ash);

	Insert2PktQs(pkt);

  //callback to higher level, should be implemented differently
  //Scheduler::instance().schedule(&CallbackHandler,&CallbackEvent, GOAL_CALLBACK_DELAY);
	return true;
}

//---------------------------------------------------------------------
//packet will be sent DataSendTime seconds later
Ptr<Packet>
AquaSimGoal::MakeReqPkt(std::set<Ptr<Packet> > DataPktSet, Time DataSendTime, Time TxTime)
{
	NS_LOG_FUNCTION(this << "DataSendTime:" << DataSendTime.ToDouble(Time::S)
					<< "TxTime:" << TxTime.ToDouble(Time::S));

	//m_device->UpdatePosition();  //out of date.
	Ptr<Packet> pkt = Create<Packet>();
	AquaSimHeader ash;
	MacHeader mach;
	AquaSimGoalReqHeader goalReqh;
	AquaSimPtTag ptag;
	AquaSimPtTag ptagTemp;
	VBHeader vbh;
	Ptr<Packet> DataPkt = *(DataPktSet.begin());

	if (DataPkt == nullptr)
	{
		NS_LOG_DEBUG("MakeReqPkt: No DataPkt found. "
				"May be a potential problem for target position.");
	}

	DataPkt->RemoveHeader(ash);	//test to ensure this isn't a null header buffer at this point
	DataPkt->RemoveHeader(mach);

	// Xia: Get the info in vbh
	if(DataPkt->PeekPacketTag(ptagTemp))
	{
		switch(ptagTemp.GetPacketType() )
		{
			case AquaSimPtTag::PT_GOAL_REQ:
			{
				AquaSimGoalReqHeader goalReqhTemp;
				DataPkt->RemoveHeader(goalReqhTemp);
				DataPkt->PeekHeader(vbh);
				DataPkt->AddHeader(goalReqhTemp);
				break;
			}
			case AquaSimPtTag::PT_GOAL_REP:
			{
				AquaSimGoalRepHeader goalRephTemp;
				DataPkt->RemoveHeader(goalRephTemp);
				DataPkt->PeekHeader(vbh);
				DataPkt->AddHeader(goalRephTemp);
				break;
			}
			case AquaSimPtTag::PT_GOAL_ACK:
			{
				AquaSimGoalAckHeader goalAckhTemp;
				DataPkt->RemoveHeader(goalAckhTemp);
				DataPkt->PeekHeader(vbh);
				DataPkt->AddHeader(goalAckhTemp);
				break;
			}
			default: // DATA
				DataPkt->PeekHeader(vbh); // xia added
				break;
		}
	}
	DataPkt->AddHeader(mach);
	DataPkt->AddHeader(ash);

	Ptr<MobilityModel> model = m_device->GetNode()->GetObject<MobilityModel>();
	goalReqh.SetRA(AquaSimAddress::GetBroadcast());
	goalReqh.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
	goalReqh.SetDA(vbh.GetTargetAddr());  //sink address

	m_reqPktSeq++;
	goalReqh.SetReqID(m_reqPktSeq);
	goalReqh.SetSenderPos(model->GetPosition());

//	goalReqh.SetSendTime(DataSendTime-Simulator::Now()); // aqua-sim-ng original

	// Xia: REQ's DataSendTime = 0,
	// thus DataSendTime-Simulator::Now() = -XX.XX, incorrect!!!
	goalReqh.SetSendTime(DataSendTime); // xia edited,  send REQ immediately!

	goalReqh.SetTxTime(TxTime);
	goalReqh.SetSinkPos(vbh.GetExtraInfo().t);
	goalReqh.SetSourcePos(vbh.GetExtraInfo().o);

	// Xia: Set the Pkt type as GOAL_REQ
	ptag.SetPacketType(AquaSimPtTag::PT_GOAL_REQ);
	ash.SetDirection(AquaSimHeader::DOWN);
	ash.SetErrorFlag(false);
	ash.SetNextHop(AquaSimAddress::GetBroadcast());
	ash.SetSize(goalReqh.size(m_backoffType)+(NSADDR_T_SIZE*DataPktSet.size())/8+1);
	ash.SetTxTime(GetTxTime(ash.GetSize()));
	//ash->addr_type() = NS_AF_ILINK;
	ash.SetTimeStamp(Simulator::Now());


	mach.SetDA(goalReqh.GetRA());
	mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );

	uint32_t size = sizeof(uint)+sizeof(int)*DataPktSet.size();
	uint8_t *data = new uint8_t[size];
	*((uint*)data) = DataPktSet.size();
	data += sizeof(uint);

	AquaSimHeader ashLocal;

	for( std::set<Ptr<Packet> >::iterator pos = DataPktSet.begin();
		pos != DataPktSet.end(); pos++)
	{
	    (*pos)->PeekHeader(ashLocal);
	    *((int*)data) = ashLocal.GetUId();
	    data += sizeof(int);
	}

	// Xia: important!!! back to the start point!
	data -= size;

	Ptr<Packet> tempPacket = Create<Packet>(data, size);
	pkt->AddAtEnd(tempPacket);
	pkt->AddHeader(goalReqh);
	pkt->AddHeader(mach);
	pkt->AddHeader(ash);
	pkt->AddPacketTag(ptag);

	NS_LOG_FUNCTION ("Me=" << AquaSimAddress::ConvertFrom(m_device->GetAddress()) <<
			"make REQ for Pktid=" <<  ash.GetUId());

	return pkt;
}


//---------------------------------------------------------------------
void
AquaSimGoal::ProcessReqPkt(Ptr<Packet> ReqPkt)
{
	AquaSimHeader ash;
	MacHeader mach;
	AquaSimGoalReqHeader goalReqh;
	ReqPkt->RemoveHeader(ash);
	ReqPkt->RemoveHeader(mach);
	ReqPkt->PeekHeader(goalReqh);
	ReqPkt->AddHeader(mach);
	ReqPkt->AddHeader(ash);

	int		PktID;
	bool	IsLoop = false;

	AquaSimGoal_AckTimeoutTimer* AckTimeoutTimer = NULL;
	std::set<AquaSimGoal_AckTimeoutTimer*>::iterator pos;
	Ptr<Packet> pkt = Create<Packet>();

	std::set<int> DuplicatedPktSet;
	std::set<int> AvailablePktSet;

	//check duplicated packet in the request.

	// xia added: for the info in the payload
	AquaSimHeader ashTmp;
	MacHeader machTmp;
	AquaSimGoalReqHeader goalReqhTmp;
	ReqPkt->RemoveHeader (ashTmp);
	ReqPkt->RemoveHeader (machTmp);
	ReqPkt->RemoveHeader (goalReqhTmp);

	uint32_t size = ReqPkt->GetSize();
	uint8_t *data = new uint8_t[size];
	ReqPkt->CopyData(data,size);
	uint PktNum = *((uint*)data);
	data += sizeof(uint);
	for( uint i=0; i<PktNum; i++)
	{
		PktID = *((int*)data);
		NS_LOG_FUNCTION ("PktID=" << PktID << ", Try to check dup!");

		if( m_recvedList.count(PktID) != 0 )
		{
			NS_LOG_FUNCTION (m_recvedList[PktID].Sender <<
					", " << goalReqh.GetSA());
			if( m_recvedList[PktID].Sender == goalReqh.GetSA() )
				DuplicatedPktSet.insert(PktID);
		}
		AvailablePktSet.insert(PktID);
		data += sizeof(int);
	}
	// xia added
	ReqPkt->AddHeader (goalReqhTmp);
	ReqPkt->AddHeader (machTmp);
	ReqPkt->AddHeader (ashTmp);


	if( !DuplicatedPktSet.empty() )
	{
		//make Ack packet with PUSH flag and send immediately
		NS_LOG_FUNCTION ("Duplicated DATA Pkt, so make Ack packet with PUSH flag and send immediately"); // xia added
		Ptr<Packet> AckPkt = MakeAckPkt(DuplicatedPktSet, true, goalReqh.GetReqID());
		AquaSimHeader ash_;
		AckPkt->PeekHeader(ash_);
		Time Txtime = GetTxTime(ash_.GetSize());
		Time SendTime = m_TSQ.GetAvailableTime(Simulator::Now(), Txtime);
		m_TSQ.Insert(SendTime, SendTime+Txtime);
		PreSendPkt(AckPkt, SendTime-Simulator::Now());
	}

	if( DuplicatedPktSet.size() == PktNum )
	{
		//all packets are duplicated, don't need to do further process
		NS_LOG_FUNCTION ("All packets are duplicated, don't need to do further process"); // xia added
		return;  //need to reserve slot for receiving???
	}

	//check if it is implicit ack // Xia: a.k.a REQ for a forward DATA Pkt
	//check the DataPktID carried by the request packet
	std::set<int>::iterator pointer = AvailablePktSet.begin();
	while( pointer != AvailablePktSet.end() )
	{
		PktID = *pointer;
		NS_LOG_FUNCTION ("PktID in AvailablePktSet = " << PktID); // xia added
		NS_LOG_FUNCTION ("m_ackTimeoutTimerSet.size()= " << m_ackTimeoutTimerSet.size());

		pos = m_ackTimeoutTimerSet.begin();
		while( pos != m_ackTimeoutTimerSet.end() )
		{
			AckTimeoutTimer = *pos;

			if( AckTimeoutTimer->PktSet().count(PktID) != 0 )
			{
				pkt = AckTimeoutTimer->PktSet().operator[](PktID);
				//pkt=0;
				AckTimeoutTimer->PktSet().erase(PktID);

				IsLoop = true;
			}
			pos++;
		}

		//check if the packets tranmitted later is originated from this node
		//In other words, is there a loop?
		if( m_originPktSet.count(PktID) != 0 )
		{
			IsLoop = true;
		}

		pointer++;
	}

	//clear the empty entries
	pos = m_ackTimeoutTimerSet.begin();
	while( pos != m_ackTimeoutTimerSet.end() )
	{
		AckTimeoutTimer = *pos;
		if( AckTimeoutTimer->PktSet().empty() )
		{
			if( AckTimeoutTimer->IsRunning() )
			{
				AckTimeoutTimer->Cancel();
			}

			m_ackTimeoutTimerSet.erase(pos);
			pos = m_ackTimeoutTimerSet.begin();

			delete AckTimeoutTimer;
			m_isForwarding = false;
		}
		else
		{
			pos++;
		}
	}


	GotoNxtRound();  //try to send data. prepareDataPkts can check if it should send data pkts

	//start to back off
	NS_LOG_FUNCTION ("Start to back off, Loop: " << IsLoop <<
			", check collision:" <<
			m_TSQ.CheckCollision(Simulator::Now()+goalReqh.GetSendTime(),
            Simulator::Now()+goalReqh.GetSendTime()+goalReqh.GetTxTime())); // xia added

	// After xia changed the sendtime of goalReqh, the checkcollision function can work!
	if( !IsLoop && m_TSQ.CheckCollision(Simulator::Now()+goalReqh.GetSendTime(),
              Simulator::Now()+goalReqh.GetSendTime()+goalReqh.GetTxTime()) ) {

		Time BackoffTimeLen = GetBackoffTime(ReqPkt);
		NS_LOG_FUNCTION ("Back off time: " << BackoffTimeLen.ToDouble(Time::S)); // Xia added
		// Problem here!!! Back off time < 0 !!!!!!!


		//this node is in the forwarding area.
		if( BackoffTimeLen.GetDouble() > 0.0 ) {
			NS_LOG_FUNCTION ("This node is in the forwarding area, start to back off."); // Xia added

			AquaSimGoal_BackoffTimer* backofftimer = new AquaSimGoal_BackoffTimer(this);
			AquaSimGoalRepHeader goalReph;
			Time RepPktTxtime = GetTxTime(goalReph.size(m_backoffType));
			Time RepSendTime = m_TSQ.GetAvailableTime(BackoffTimeLen+Simulator::Now()+
                            	JitterStartTime(RepPktTxtime),RepPktTxtime );
			SchedElem* SE = new SchedElem(RepSendTime, RepSendTime+RepPktTxtime);

			//reserve the sending interval for Rep packet
			NS_LOG_FUNCTION ("Reserved the sending interval for me to reply REP");
			m_TSQ.Insert(SE);

			backofftimer->ReqPkt() = ReqPkt->Copy();
			backofftimer->SetSE(SE);
			backofftimer->BackoffTime() = BackoffTimeLen;

			backofftimer->SetFunction(&ns3::AquaSimGoal_BackoffTimer::expire,backofftimer);
			backofftimer->Schedule(RepSendTime-Simulator::Now());
			m_backoffTimerSet.insert(backofftimer);

			//avoid send-recv collision or recv-recv collision at this node
			NS_LOG_FUNCTION ("Simulator::Now():" << Simulator::Now().ToDouble(Time::S)
					<< "goalReqh.GetSendTime():" << goalReqh.GetSendTime().ToDouble(Time::S));
			NS_LOG_FUNCTION ("Reserved time for receive this REQ");

			// aqua-sim-ng original:
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
	NS_LOG_FUNCTION(this);

	//m_device->UpdatePosition();  //out of date
	AquaSimGoalReqHeader reqPktHeader;
	AquaSimHeader ashTmp;
	MacHeader machTmp;
	ReqPkt->RemoveHeader(ashTmp);
	ReqPkt->RemoveHeader(machTmp);
	ReqPkt->PeekHeader(reqPktHeader);
	ReqPkt->AddHeader(machTmp);
	ReqPkt->AddHeader(ashTmp);
	Ptr<Packet> pkt = Create<Packet>();
	AquaSimHeader ash;
	MacHeader mach;
	AquaSimGoalRepHeader repH;
	AquaSimPtTag ptag;
	Ptr<MobilityModel> model = m_device->GetNode()->GetObject<MobilityModel>();

	repH.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
	repH.SetRA(reqPktHeader.GetSA());
	repH.SetReqID(reqPktHeader.GetReqID());
	repH.SetReplyerPos(model->GetPosition());
	// xia added
//	NS_LOG_FUNCTION ("repH.SendTime=reqPktHeader.GetSendTime()=" << reqPktHeader.GetSendTime().ToDouble(Time::S));
	repH.SetSendTime(reqPktHeader.GetSendTime());	//update this item when being sent out
	repH.SetBackoffTime(BackoffTime);

	ash.SetDirection(AquaSimHeader::DOWN);
	ptag.SetPacketType(AquaSimPtTag::PT_GOAL_REP);
	ash.SetErrorFlag(false);
	ash.SetNextHop(repH.GetRA());			//reply the sender of request pkt
	ash.SetSize(repH.size(m_backoffType));
	//ash.addr_type() = NS_AF_ILINK;
	ash.SetTimeStamp(Simulator::Now());

	// Xia added
	ash.SetDAddr(ashTmp.GetSAddr());
	ash.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	ash.SetUId(ashTmp.GetUId());

	mach.SetDA(repH.GetRA());
	mach.SetSA(repH.GetSA());

	pkt->AddHeader(repH);
	pkt->AddHeader(mach);
	pkt->AddHeader(ash);
	pkt->AddPacketTag(ptag);

	NS_LOG_FUNCTION ("Me=" << AquaSimAddress::ConvertFrom(m_device->GetAddress()) <<
			"make REP for Pktid=" <<  ash.GetUId());
	return pkt;
}


//---------------------------------------------------------------------
void
AquaSimGoal::ProcessRepPkt(Ptr<Packet> RepPkt)
{
	NS_LOG_FUNCTION (this);
	AquaSimHeader ash;
	MacHeader mach;
	AquaSimGoalRepHeader repH;
	RepPkt->RemoveHeader(ash);
	RepPkt->RemoveHeader(mach);
	RepPkt->PeekHeader(repH);
	RepPkt->AddHeader(mach);
	RepPkt->AddHeader(ash);

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
	AquaSimHeader ash;
	MacHeader mach;
	AquaSimGoalRepHeader repH;
	RepPkt->RemoveHeader(ash);
	RepPkt->RemoveHeader(mach);
	RepPkt->PeekHeader(repH);
	RepPkt->AddHeader(mach);
	RepPkt->AddHeader(ash);

	AquaSimGoalRepHeader repHLocal;
	std::set<AquaSimGoal_BackoffTimer*>::iterator pos = m_backoffTimerSet.begin();
	while( pos != m_backoffTimerSet.end() ) {
			(*pos)->ReqPkt()->RemoveHeader(ash);	//expensive...
			(*pos)->ReqPkt()->RemoveHeader(mach);
	    (*pos)->ReqPkt()->PeekHeader(repHLocal);
	    (*pos)->ReqPkt()->AddHeader(mach);
	    (*pos)->ReqPkt()->AddHeader(ash);
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
			//(*pos)->ReqPkt()=0;
		}
		pos++;
	}

	//reserve time slot for the corresponding Data Sending event.
	//avoid recv-recv collision at neighbors
	Vector ThisNode;
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
	NS_LOG_FUNCTION (this << "me=" << AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	/*if( m_isForwarding || (m_qsPktNum == 0) )
		return;*/
	AquaSimGoalReqHeader reqH;
	AquaSimHeader ash;
	MacHeader mach;

	//int		SinkAddr = -1;  //not used.
	Time DataTxTime = Seconds(0);
	Time DataSendTime = Seconds(0);
	Time ReqSendTime = Seconds(0);
	Time ReqPktTxTime = GetTxTime(reqH.size(m_backoffType));
	Ptr<Packet> pkt;
	Ptr<Packet> ReqPkt;
	AquaSimGoalDataSendTimer* DataSendTimer = new AquaSimGoalDataSendTimer(this);
	//GOAL_RepTimeoutTimer* RepTimeoutTimer = new GOAL_RepTimeoutTimer(this);

	if( m_PktQs.size() == 0 )
	{
		NS_LOG_WARN("PrepareDataPkts: size of m_PktQs should not be 0, something must be wrong");
	}

	std::map<AquaSimAddress, AquaSimGoal_PktQ>::iterator pos = m_PktQs.begin();
	for(int i=0; i<m_sinkSeq; i++)
	{
		pos++;
	}
	m_sinkSeq = (m_sinkSeq+1)%m_PktQs.size();

	for( int i=0; i<m_maxBurst && (!m_PktQs[pos->first].Q_.empty()); i++)
	{
		pkt = m_PktQs[pos->first].Q_.front();

		DataSendTimer->DataPktSet().insert(pkt);
		m_PktQs[pos->first].Q_.pop_front();
		--m_qsPktNum;
		pkt->PeekHeader(ash);

		NS_LOG_FUNCTION ("DataPkt UId = " << ash.GetUId());

		DataTxTime += ash.GetTxTime() + m_dataPktInterval;
	}
	//additional m_dataPktInterval is considered, subtract it.
	DataTxTime -= m_dataPktInterval;

	//erase empty entry
	if( m_PktQs[pos->first].Q_.empty() )
		m_PktQs.erase(pos->first);

	// Xia: Now DataSendTime=0, so should make REQ immediately!!!
	ReqPkt = MakeReqPkt(DataSendTimer->DataPktSet(), DataSendTime, DataTxTime);
	AquaSimHeader ashLocal;
	ReqPkt->PeekHeader(ashLocal);
	ReqPktTxTime = GetTxTime(ashLocal.GetSize());

	//reserve time slot for sending REQ
//	NS_LOG_FUNCTION ("Reserve time slot for sending REQ"); // xia added
	ReqSendTime = m_TSQ.GetAvailableTime(Simulator::Now()+JitterStartTime(ReqPktTxTime), ReqPktTxTime);
	m_TSQ.Insert(ReqSendTime, ReqSendTime+ReqPktTxTime);

	//reserve time slot for sending DATA
//	NS_LOG_FUNCTION ("Reserve time slot for sending DATA"); // xia added
	NS_LOG_FUNCTION ("MaxDelay=" << m_maxDelay);
	DataSendTime = m_TSQ.GetAvailableTime(Simulator::Now() + m_maxBackoffTime
					+ 2 * m_maxDelay + m_estimateError, DataTxTime, true);
	DataSendTimer->SetSE( m_TSQ.Insert(DataSendTime, DataSendTime+DataTxTime) );

	ReqPkt->RemoveHeader(ashLocal);
	ReqPkt->RemoveHeader(mach);
	ReqPkt->PeekHeader(reqH);
	ReqPkt->AddHeader(mach);
	ReqPkt->AddHeader(ashLocal);

	DataSendTimer->SetReqID(reqH.GetReqID());
	DataSendTimer->TxTime() = DataTxTime;
	DataSendTimer->MinBackoffTime() = Seconds(100000.0);
	DataSendTimer->SetRep(false);

	//send REQ
	PreSendPkt(ReqPkt, ReqSendTime-Simulator::Now());

	DataSendTimer->SetFunction(&AquaSimGoalDataSendTimer::expire, DataSendTimer);
	DataSendTimer->Schedule(DataSendTime - Simulator::Now());

	NS_LOG_FUNCTION ("Insert a DataSendTimer, ReqID=" << DataSendTimer->ReqID());
	m_dataSendTimerSet.insert(DataSendTimer);
}

//---------------------------------------------------------------------
void
AquaSimGoal::SendDataPkts(std::set<Ptr<Packet> > DataPktSet, AquaSimAddress NxtHop, Time TxTime)
{
//	NS_LOG_FUNCTION("Next Hop:" << NxtHop.GetAsInt());

	std::set<Ptr<Packet> >::iterator pos = DataPktSet.begin();
	Time DelayTime = Seconds(0.00001);  //the delay of sending data packet
	AquaSimHeader ash;
	MacHeader mach;
	AquaSimGoal_AckTimeoutTimer* AckTimeoutTimer = new AquaSimGoal_AckTimeoutTimer(this);

	while( pos != DataPktSet.end() )
	{
		(*pos)->RemoveHeader(ash);
		(*pos)->RemoveHeader(mach);
		ash.SetNextHop(NxtHop);
		mach.SetDA(NxtHop);
		mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
		(*pos)->AddHeader(mach);
		(*pos)->AddHeader(ash);

		PreSendPkt((*pos)->Copy(), DelayTime);
		// xia added
		NS_LOG_FUNCTION ("ash.GetUId()= " << ash.GetUId());

		AckTimeoutTimer->PktSet().operator[](ash.GetUId()) = (*pos);
		DelayTime += m_dataPktInterval + ash.GetTxTime();
		pos++;
	}

	AckTimeoutTimer->SetFunction(&AquaSimGoal_AckTimeoutTimer::expire,
			AckTimeoutTimer);

	Time ackTimeoutTime = 2 * m_maxDelay + TxTime+ this->m_nxtRoundMaxWaitTime
							+ m_estimateError + MilliSeconds(0.5);
	NS_LOG_FUNCTION ("ackTimeoutTime=" << ackTimeoutTime.ToDouble(Time::S));
	AckTimeoutTimer->Schedule (ackTimeoutTime);
	m_ackTimeoutTimerSet.insert(AckTimeoutTimer);
	NS_LOG_FUNCTION ("m_ackTimeoutTimerSet.size()= " << m_ackTimeoutTimerSet.size());
}


//---------------------------------------------------------------------
void
AquaSimGoal::ProcessDataPkt(Ptr<Packet> DataPkt)
{
	NS_LOG_FUNCTION (this << "Me=" << AquaSimAddress::ConvertFrom(m_device->GetAddress()));

	AquaSimHeader ash;
	MacHeader mach;
	AquaSimGoalAckHeader goalAckh;
	VBHeader vbh;
	DataPkt->RemoveHeader(ash);
	DataPkt->RemoveHeader(mach);
//	DataPkt->RemoveHeader(goalAckh);
	DataPkt->PeekHeader(vbh);
	//hdr_uwvb* vbh = hdr_uwvb::access(DataPkt);

	if( m_recvedList.count(ash.GetUId()) != 0 ) {
		//duplicated packet, free it.
		//DataPkt=0;
		NS_LOG_FUNCTION ("Duplicated packet, free it.");
		return;
	}

	PurifyRecvedList();
	m_recvedList[ash.GetUId()].RecvTime = Simulator::Now();
	m_recvedList[ash.GetUId()].Sender = mach.GetSA();

	if( vbh.GetTargetAddr() == m_device->GetAddress() )
	{
		NS_LOG_FUNCTION ("It's an DATA UId=" << ash.GetUId() << " to me=" << m_device->GetNode()->GetId());
		//ack this data pkt if this node is the destination

		if( SinkAccumAckTimer.IsRunning() ) {
			SinkAccumAckTimer.Cancel();
		}

		SinkAccumAckTimer.SetFunction(&AquaSimGoal_SinkAccumAckTimer::expire,&SinkAccumAckTimer);
		SinkAccumAckTimer.Schedule(ash.GetTxTime()+ m_dataPktInterval*2);


		if (SinkAccumAckTimer.AckSet().count(ash.GetUId()) == 0)
		{//ack!!! insert the uid into AckSet.
			NS_LOG_FUNCTION ("Insert Data pkt into AckSet, Uid=" << ash.GetUId());
			SinkAccumAckTimer.AckSet().insert( ash.GetUId() );
		}

		if( m_sinkRecvedList.count(ash.GetUId()) != 0 )
		{//xia: received this DATA pkt before
			NS_LOG_FUNCTION ("Received this DATA before.");
			;//DataPkt=0;
		}
		else
		{
			NS_LOG_FUNCTION ("Received this DATA first time.");
			m_sinkRecvedList.insert(ash.GetUId());
			ash.SetSize(ash.GetSize() - sizeof(AquaSimAddress)*2);

			//  xia added
			goalAckh.SetSA(ash.GetSAddr());
			goalAckh.SetRA(ash.GetDAddr());
//			goalAckh.SetReqID(ash.GetUId());


//			DataPkt->AddHeader(goalAckh); // xia: data pkt, no ack
			DataPkt->AddHeader(mach);
			DataPkt->AddHeader(ash);
			SendUp(DataPkt); // Xia: send from PHY to MAC
		}

	}
	else {
		//forward this packet later
		NS_LOG_FUNCTION ("It's not a DATA to me=" << m_device->GetNode()->GetId() << ", forward it later.");

		ash.SetNumForwards(0);
//		DataPkt->AddHeader(goalAckh); // xia: data pkt, no ack
		DataPkt->AddHeader(mach);
		DataPkt->AddHeader(ash);
		Insert2PktQs(DataPkt);
	}
}

//---------------------------------------------------------------------
Ptr<Packet>
AquaSimGoal::MakeAckPkt(std::set<int> AckSet, bool PSH,  int ReqID)
{
	NS_LOG_FUNCTION(this << PSH << ReqID);

	Ptr<Packet> pkt = Create<Packet>();
	AquaSimHeader ash;
	MacHeader mach;
	AquaSimGoalAckHeader goalAckh;
	AquaSimPtTag ptag;

	goalAckh.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
	goalAckh.SetRA(AquaSimAddress::GetBroadcast());
	goalAckh.SetPush(PSH);
	if( PSH ) {
		goalAckh.SetReqID(ReqID);
	}

	ptag.SetPacketType(AquaSimPtTag::PT_GOAL_ACK);
	ash.SetDirection(AquaSimHeader::DOWN);
	ash.SetErrorFlag(false);
	ash.SetNextHop(goalAckh.GetRA());   //reply the sender of request pkt
	ash.SetSize(goalAckh.size(m_backoffType) + (NSADDR_T_SIZE*AckSet.size())/8+1);
	//ash.addr_type() = NS_AF_ILINK;

	// Xia added:
	ash.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	ash.SetDAddr(AquaSimAddress::GetBroadcast()); // ???????


	mach.SetDA(goalAckh.GetRA());
	mach.SetSA(goalAckh.GetSA());


	uint32_t size = sizeof(int) * AckSet.size() + sizeof(uint);
//	NS_LOG_FUNCTION ("size:" << size);
	uint8_t *data = new uint8_t[size]; // uint8_t = unsigned char

	*((uint*)data) = AckSet.size();
	data += sizeof(uint);

	for( std::set<int>::iterator pos = AckSet.begin();
		pos != AckSet.end(); pos++)   {
//		NS_LOG_FUNCTION ("Uid in AckSet:" << *pos);

		// xia added
		ash.SetUId(*pos);

	    *((uint*)data) = *pos;
	    data += sizeof(int);
	}

	// Xia: Important!!!
	data -= size;

	Ptr<Packet> tempPacket = Create<Packet>(data, size);
	pkt->AddAtEnd(tempPacket);
	pkt->AddHeader(goalAckh);
	pkt->AddHeader(mach);
	pkt->AddHeader(ash);
	pkt->AddPacketTag(ptag);

	return pkt;
}

//---------------------------------------------------------------------
void
AquaSimGoal::ProcessAckPkt(Ptr<Packet> AckPkt)
{
	NS_LOG_FUNCTION (this); // << "AckPkt size:" << AckPkt->GetSize());
	//cancel the Ack timeout timer and release data packet.
	int PktID;
	AquaSimGoal_AckTimeoutTimer* AckTimeoutTimer;
	Ptr<Packet> pkt;
	std::set<AquaSimGoal_AckTimeoutTimer*>::iterator pos;


	// Xia added:
	AquaSimHeader ash;
	MacHeader mach;
	AquaSimGoalAckHeader goalAckh;
	AquaSimPtTag ptag;
	AckPkt->RemovePacketTag(ptag);
	AckPkt->RemoveHeader(ash);
	AckPkt->RemoveHeader(mach);
	AckPkt->RemoveHeader(goalAckh);
//	NS_LOG_FUNCTION ("After rm header, ackpkt.size():" << AckPkt->GetSize());


	//check the DataPktID carried by the ack packet
	uint32_t size = AckPkt->GetSize();
	uint8_t *data = new uint8_t[size];
	AckPkt->CopyData(data, size);

	uint PktNum = *((uint*)(data));
	data += sizeof(uint);
//	NS_LOG_FUNCTION ("PktNum:" << PktNum);

	for( uint i=0; i < PktNum; i++)
	{
		PktID = *((int*)data);
		NS_LOG_FUNCTION ("PktID:" << PktID);
		NS_LOG_FUNCTION ("m_ackTimeoutTimerSet.size()=" <<
				m_ackTimeoutTimerSet.size());

		pos = m_ackTimeoutTimerSet.begin();

		while( pos != m_ackTimeoutTimerSet.end() )
		{
			AckTimeoutTimer = *pos;
			NS_LOG_FUNCTION ("AckTimeoutTimer->PktSet().size()="
							<< AckTimeoutTimer->PktSet().size());
			if( AckTimeoutTimer->PktSet().count(PktID) != 0 )
			{
				pkt = AckTimeoutTimer->PktSet().operator[](PktID);
				pkt=0;

				// xia added
				// pkt->Cleanup();
				AckTimeoutTimer->PktSet().erase(PktID);
				// xia added
				NS_LOG_FUNCTION ("me:" << AquaSimAddress::ConvertFrom(m_device->GetAddress()) <<
						", Erase this pkt's AckTimeoutTimer.");
			}
			pos++;
		}

		data += sizeof(int);
	}


	//clear the empty entries
	pos = m_ackTimeoutTimerSet.begin();
	while( pos != m_ackTimeoutTimerSet.end() )
	{
		// xia added
		NS_LOG_FUNCTION ("Try to clear empty acktimeouttimer.");

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
	NS_LOG_FUNCTION(this);

	AquaSimHeader ash;
	MacHeader mach;
	AquaSimGoalAckHeader goalAckh;
	AquaSimPtTag ptag;
	AckPkt->RemoveHeader(ash);
	AckPkt->RemoveHeader(mach);
	AckPkt->PeekHeader(goalAckh);
	AckPkt->AddHeader(mach);
	AckPkt->AddHeader(ash);

	int ReqID = goalAckh.GetReqID();
	AquaSimGoalDataSendTimer* DataSendTimer=NULL;
	std::set<AquaSimGoalDataSendTimer*>::iterator pos = m_dataSendTimerSet.begin();

	NS_LOG_FUNCTION ("m_dataSendTimerSet.size()=" << m_dataSendTimerSet.size());
	while( pos != m_dataSendTimerSet.end() ) {
		NS_LOG_FUNCTION ("(*pos)->ReqID()=" << (*pos)->ReqID()
				<< "ReqID=" << ReqID);
		if( (*pos)->ReqID() == ReqID ) {
			DataSendTimer = *pos;
			break;
		}
		pos++;
	}

	if( DataSendTimer != NULL ) {
		//the reply is from this node??
		//check the duplicated DataPktID carried by the ack packet

		// Xia added: remove header to get the info in payload
		AckPkt->RemoveHeader(ash);
		AckPkt->RemoveHeader(mach);
		AckPkt->RemoveHeader(goalAckh);

		uint32_t size = AckPkt->GetSize();
		uint8_t *data = new uint8_t[size];
		AckPkt->CopyData(data, size);
		uint PktNum = *((uint*)data);
		NS_LOG_FUNCTION ("PktNum in AckPkt=" << PktNum);
		data += sizeof(uint);
		NS_LOG_FUNCTION ("PktUId in AckPkt=" << *((int*)data));

		AckPkt->AddHeader(goalAckh);
		AckPkt->AddHeader(mach);
		AckPkt->AddHeader(ash);

		for(uint i=0; i<PktNum; i++) {
			std::set<Ptr<Packet> >::iterator pointer = DataSendTimer->DataPktSet().begin();
			while( pointer != DataSendTimer->DataPktSet().end() ) {
				AquaSimHeader ash;
				(*pointer)->PeekHeader(ash);
				if( ash.GetUId() == *((uint32_t*)data) ) {
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
	NS_LOG_FUNCTION (this);
	AquaSimHeader ash;
	MacHeader mach;
	VBHeader vbh;
//	AquaSimGoalAckHeader goalAckh;

	DataPkt->RemoveHeader(ash);
	DataPkt->RemoveHeader(mach);
//	DataPkt->RemoveHeader(goalAckh); //Xia: no goalAckh,remove this line!
	DataPkt->PeekHeader(vbh);

	if( ash.GetNumForwards() > m_maxRetransTimes ) {
		DataPkt = NULL;
		return;
	}
	else {
	    uint8_t tmp = ash.GetNumForwards();
	    tmp++;
	    ash.SetNumForwards(tmp);	//bit awkward.

		DataPkt->AddHeader(mach);
    	DataPkt->AddHeader(ash);
	}

	if( FrontPush )
	  m_PktQs[vbh.GetTargetAddr()].Q_.push_front(DataPkt);
	else
	  m_PktQs[vbh.GetTargetAddr()].Q_.push_back(DataPkt);

	m_qsPktNum++;

	GotoNxtRound();
}

//---------------------------------------------------------------------
void
AquaSimGoal::ProcessBackoffTimeOut(AquaSimGoal_BackoffTimer *backoff_timer)
{
	NS_LOG_FUNCTION(this);

	Ptr<Packet> RepPkt = MakeRepPkt(backoff_timer->ReqPkt(),
								backoff_timer->BackoffTime());

	/* The time slot for sending reply packet is
	 * already reserved when processing request packet,
	 * so RepPkt is directly sent out.
	 */
	SendoutPkt(RepPkt);
	//backoff_timer->ReqPkt()=0;
	m_backoffTimerSet.erase(backoff_timer);
	delete backoff_timer;
}

//---------------------------------------------------------------------
void
AquaSimGoal::ProcessDataSendTimer(AquaSimGoalDataSendTimer *DataSendTimer)
{
//	NS_LOG_FUNCTION(this);
	if( !DataSendTimer->GotRep() ) {
		//if havenot gotten reply, resend the packet
		NS_LOG_FUNCTION ("Haven't gotten reply, resend the packet"); // xia added

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
		NS_LOG_FUNCTION ("Got Reply, send data pkt.");
		SendDataPkts(DataSendTimer->DataPktSet(),
				DataSendTimer->NxtHop(), DataSendTimer->TxTime());
		DataSendTimer->DataPktSet().clear();
	}

	NS_LOG_FUNCTION ("Erase a DataSendTimer, ReqID=" << DataSendTimer->ReqID());
	m_dataSendTimerSet.erase(DataSendTimer);
	delete DataSendTimer;
}

//---------------------------------------------------------------------
void
AquaSimGoal::ProcessSinkAccumAckTimeout()
{
	NS_LOG_FUNCTION (this);
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
	NS_LOG_FUNCTION(this);
	SendoutPkt(PreSendTimer->Pkt());
	PreSendTimer->Pkt() = NULL;
	m_preSendTimerSet.erase(PreSendTimer);
	delete PreSendTimer;
}

//---------------------------------------------------------------------
void
AquaSimGoal::ProcessAckTimeout(AquaSimGoal_AckTimeoutTimer *AckTimeoutTimer)
{
	NS_LOG_FUNCTION ("Me=" << AquaSimAddress::ConvertFrom(m_device->GetAddress())
		<< "didn't get an (implicit or not) ACK in time.");

	std::map<int, Ptr<Packet> >::iterator pos = AckTimeoutTimer->PktSet().begin();
	while( pos != AckTimeoutTimer->PktSet().end() )
	{
		Insert2PktQs(pos->second, true);
		pos++;
		NS_LOG_FUNCTION ("Re-insert DATA into PktQ");
	}

	AckTimeoutTimer->PktSet().clear();
	m_ackTimeoutTimerSet.erase (AckTimeoutTimer);
	delete AckTimeoutTimer;
	NS_LOG_FUNCTION ("Delete the AckTimeoutTimer, m_ackTimeoutTimerSet.size()= " << m_ackTimeoutTimerSet.size());
	m_isForwarding = false;
	GotoNxtRound();
}


//---------------------------------------------------------------------
void
AquaSimGoal::PreSendPkt(Ptr<Packet> pkt, Time delay)
{
	NS_LOG_FUNCTION (this << "delay " << delay);

	AquaSimGoal_PreSendTimer* PreSendTimer = new AquaSimGoal_PreSendTimer(this);
	PreSendTimer->Pkt() = pkt;
	PreSendTimer->SetFunction(&AquaSimGoal_PreSendTimer::expire, PreSendTimer);
	PreSendTimer->Schedule(delay);
	m_preSendTimerSet.insert(PreSendTimer);
}


//---------------------------------------------------------------------
void
AquaSimGoal::SendoutPkt(Ptr<Packet> pkt)
{
	NS_LOG_FUNCTION(this);

	AquaSimHeader ash;
	MacHeader mach;
	AquaSimPtTag ptag;
	pkt->RemoveHeader(ash);
	pkt->PeekPacketTag(ptag);

	ash.SetTxTime(GetTxTime(ash.GetSize()));

	//update the the Time for sending out the data packets.

	Time txtime=ash.GetTxTime();

	switch( m_device->GetTransmissionStatus() ) {
		case SLEEP:
			PowerOn();
			break;
		case RECV:
			//interrupt reception
			InterruptRecv(txtime.ToDouble(Time::S));
			break;
		case NIDLE:
			//m_device->SetTransmissionStatus(SEND);
			switch( ptag.GetPacketType() )
			{
				case AquaSimPtTag::PT_GOAL_REQ:
					{
						NS_LOG_FUNCTION ("Type: GOAL_REQ");
						AquaSimGoalReqHeader goalReqh;
						pkt->RemoveHeader(mach);
						pkt->RemoveHeader(goalReqh);

						// aqua-sim-ng original: temp < 0
						Time temp = goalReqh.GetSendTime() - (Simulator::Now() - ash.GetTimeStamp());
						// xia added
						NS_LOG_FUNCTION (goalReqh.GetSendTime().ToDouble(Time::S)
								<< Simulator::Now().ToDouble(Time::S)
								<< ash.GetTimeStamp().ToDouble(Time::S));

						// xia added:
						if (temp.GetInteger() < 0) {
							temp = Seconds(0);
						}

//						NS_LOG_FUNCTION ("set goalReqh.SendTime=" << temp.ToDouble(Time::S));
						goalReqh.SetSendTime(temp);
						pkt->AddHeader(goalReqh);
						pkt->AddHeader(mach);
					}
					break;
				case AquaSimPtTag::PT_GOAL_REP:
					{
						NS_LOG_FUNCTION ("Type: GOAL_REP");
						AquaSimGoalRepHeader goalReph;
						pkt->RemoveHeader(mach);
						pkt->RemoveHeader(goalReph);

						Time temp = goalReph.GetSendTime() - (Simulator::Now() - ash.GetTimeStamp());

						goalReph.SetSendTime(temp);
						pkt->AddHeader(goalReph);
						pkt->AddHeader(mach);
					}
					break;
				case AquaSimPtTag::PT_GOAL_ACK:
				{
					NS_LOG_FUNCTION ("Type: GOAL_ACK");
				}
					break;
				default:
					NS_LOG_FUNCTION ("Type: GOAL_DATA");
					break;
			}

			ash.SetTimeStamp(Simulator::Now());
			ash.SetDirection(AquaSimHeader::DOWN);
			pkt->AddHeader(ash);
			SendDown(pkt);
			break;

		//case RECV:
		//	//backoff???
		//	Packet::free(pkt);
		//	break;
		//
		default:
			//status is SEND
			NS_LOG_INFO("SendoutPkt:Node=" << m_device->GetNode() << " send data too fast");
			//pkt=0;
	}

	return;
}

//---------------------------------------------------------------------
double
AquaSimGoal::Dist(Vector Pos1, Vector Pos2)
{
  //redundant - should probably be integrated using something else

	double delta_x = Pos1.x - Pos2.x;
	double delta_y = Pos1.y - Pos2.y;
	double delta_z = Pos1.z - Pos2.z;

	return sqrt(delta_x*delta_x + delta_y*delta_y + delta_z*delta_z);
}


//---------------------------------------------------------------------
Time
AquaSimGoal::GetBackoffTime(Ptr<Packet> ReqPkt)
{
	//do we really need to send the entire packet for something like this???

	AquaSimHeader ash;
	MacHeader mach;
	AquaSimGoalReqHeader goalReqh;
	ReqPkt->RemoveHeader(ash);
	ReqPkt->RemoveHeader(mach);
	ReqPkt->PeekHeader(goalReqh);
	ReqPkt->AddHeader(mach);
	ReqPkt->AddHeader(ash);
	double BackoffTime;

	switch( m_backoffType ) {
		case VBF:
			BackoffTime = GetVBFbackoffTime(goalReqh.GetSourcePos(),
              goalReqh.GetSenderPos(), goalReqh.GetSinkPos());

			NS_LOG_FUNCTION (this << "Backoff type: VBF, "
								"Source pos:" << goalReqh.GetSourcePos() <<
								"Sender pos:" << goalReqh.GetSenderPos() <<
								"Sink pos:" << goalReqh.GetSinkPos() <<
								"Back off time:" << Seconds(BackoffTime).ToDouble(Time::S)); // xia added

			break;
		case HH_VBF:
			NS_LOG_FUNCTION (this << "Backoff type: HH_VBF");
			BackoffTime = GetHH_VBFbackoffTime(goalReqh.GetSenderPos(), goalReqh.GetSinkPos());
			break;
		default:
			NS_LOG_WARN("No such back off type");
			exit(0);
	}

//	return MilliSeconds(BackoffTime); // aqua-sim-ng original
	return Seconds(BackoffTime); // xia edited
}

//---------------------------------------------------------------------
//Backoff Functions
double
AquaSimGoal::GetVBFbackoffTime(Vector Source, Vector Sender, Vector Sink)
{
	double DTimesCosTheta =0.0;
	double p = 0.0;;
	double alpha = 0.0;
	Vector ThisNode;
	//m_device->UpdatePosition();  //out of date
	Ptr<MobilityModel> model = m_device->GetNode()->GetObject<MobilityModel>();
	ThisNode = model->GetPosition();
	NS_LOG_FUNCTION ("This node pos:" << ThisNode);

	if( Dist(Sender, Sink) < Dist(ThisNode, Sink) ){
		NS_LOG_DEBUG ("Can't back off, Dist(Sender, Sink) < Dist(ThisNode, Sink).");
		return -1.0;
	}

	DTimesCosTheta = DistToLine(Sender, Sink);
	p = DistToLine(Source, Sink);

	if( p > m_pipeWidth ){
		NS_LOG_DEBUG ("Can't back off, p > m_pipeWidth.");
		return -1.0;
	}

	alpha = p/m_pipeWidth + (m_txRadius-DTimesCosTheta)/m_txRadius;

	// aqua-sim-ng original:
//	return m_VBF_MaxDelay.ToDouble(Time::MS)*sqrt(alpha)+2*(m_txRadius-Dist(Sender, ThisNode))/m_propSpeed;

	// xia edit:
	return m_VBF_MaxDelay.ToDouble(Time::S)*sqrt(alpha)+2*(m_txRadius-Dist(Sender, ThisNode))/m_propSpeed;
}

//---------------------------------------------------------------------
double
AquaSimGoal::GetHH_VBFbackoffTime(Vector Sender, Vector Sink)
{
	return GetVBFbackoffTime(Sender, Sender, Sink);
}


//---------------------------------------------------------------------
//Line Point1 and Line Point2 is two points on the line
double
AquaSimGoal::DistToLine(Vector LinePoint1, Vector LinePoint2)
{
  Vector ThisNode;
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
	{
		// xia add log lines
		if (m_isForwarding)
			NS_LOG_FUNCTION ("Canceled because m_forwarding==true.");
		else if (m_nxtRoundTimer.IsRunning())
			NS_LOG_FUNCTION ("Canceled because m_nxtRoundTimer.IsRunning().");
		else
			NS_LOG_FUNCTION ("Canceled because m_qsPktNum==0.");

		return;
	}

	NS_LOG_FUNCTION (this);
	m_isForwarding = true;

	m_nxtRoundTimer.SetFunction(&AquaSimGoal_NxtRoundTimer::expire,
			&m_nxtRoundTimer);
	m_nxtRoundTimer.Schedule(
			FemtoSeconds(m_rand->GetValue(0.0,m_nxtRoundMaxWaitTime.ToDouble(Time::S) ) ) );
}

//---------------------------------------------------------------------


Time
AquaSimGoal::JitterStartTime(Time Txtime)
{
	Time BeginTime = 5*Txtime*m_rand->GetValue();
	return BeginTime;
}

void AquaSimGoal::DoDispose()
{
	m_rand=0;
	AquaSimMac::DoDispose();
}

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

ns3::TimeSchedQueue::~TimeSchedQueue()
{
	for (std::list<SchedElem*>::iterator it=m_SchedQ.begin(); it != m_SchedQ.end(); ++it) {
		delete *it;
		*it=0;
	}
	m_SchedQ.clear();
}

//---------------------------------------------------------------------
ns3::SchedElem*
ns3::TimeSchedQueue::Insert(SchedElem *e)
{
	NS_LOG_FUNCTION ("BeginTime:" << (e->BeginTime).ToDouble(Time::S) << "EndTime:"<< (e->EndTime).ToDouble(Time::S)); // xia added
	std::list<SchedElem*>::iterator pos = m_SchedQ.begin();
	while( pos != m_SchedQ.end() )
	{
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
	NS_LOG_FUNCTION (this); // xia added
	m_SchedQ.remove(e);
	delete e;
}


//---------------------------------------------------------------------
ns3::Time
ns3::TimeSchedQueue::GetAvailableTime(Time EarliestTime, Time SlotLen, bool BigInterval)
{
	NS_LOG_FUNCTION (this); // xia added
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

	Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
//	// aqua-sim-ng original:
//	return MilliSeconds(m_rand->GetValue(LowerBeginTime.ToDouble(Time::MS), UpperBeginTime.ToDouble(Time::MS)));

	// xia edited:
	return Seconds(m_rand->GetValue(LowerBeginTime.ToDouble(Time::S),
			UpperBeginTime.ToDouble(Time::S)));
}


//---------------------------------------------------------------------
bool
ns3::TimeSchedQueue::CheckCollision(Time BeginTime, Time EndTime)
{
	NS_LOG_FUNCTION (this); // xia added
	ClearExpiredElems();

	std::list<SchedElem*>::iterator pos = m_SchedQ.begin();
	BeginTime -= m_minInterval;   //consider the guard time
	EndTime += m_minInterval;

	if( m_SchedQ.empty() ) {
		return true; // xia: means no collision
	}
	else {
		while( pos != m_SchedQ.end() ) {
			if( (BeginTime < (*pos)->BeginTime && EndTime > (*pos)->BeginTime)
				|| (BeginTime < (*pos)->EndTime && EndTime > (*pos)->EndTime ) ) {
				return false; // xia: means collision happened
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


} // namespace ns3
