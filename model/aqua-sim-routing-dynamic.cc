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

#include "aqua-sim-routing-dynamic.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-pt-tag.h"
#include "ns3/ipv4-header.h"
#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/nstime.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimDynamicRouting");
NS_OBJECT_ENSURE_REGISTERED(AquaSimDynamicRoutingTable);

AquaSimDynamicRoutingTable::AquaSimDynamicRoutingTable()
{
}

TypeId
AquaSimDynamicRoutingTable::GetTypeId()
{
  static TypeId tid = TypeId("ns3::AquaSimDynamicRoutingTable")
    ;
  return tid;
}

void
AquaSimDynamicRoutingTable::Print(AquaSimAddress id)
{
  NS_LOG_FUNCTION(this << id << Simulator::Now().GetSeconds());
	for (t_table::iterator it = m_rt.begin(); it != m_rt.end(); it++) {
    NS_LOG_INFO(id << "," << (*it).first << "," <<
                  (*it).second.first << "," << it->second.second);
	}
}

int
AquaSimDynamicRoutingTable::IfChg ()
{
	return m_chg;
}

void
AquaSimDynamicRoutingTable::Clear()
{
	m_rt.clear();
}

void
AquaSimDynamicRoutingTable::RemoveEntry(AquaSimAddress dest)
{
	m_rt.erase(dest);
}

void
AquaSimDynamicRoutingTable::AddEntry(AquaSimAddress dest, DN next)
{
	m_rt[dest] = next;
}

AquaSimAddress
AquaSimDynamicRoutingTable::Lookup(AquaSimAddress dest)
{
	t_table::iterator it = m_rt.find(dest);
	if (it == m_rt.end())
		return AquaSimAddress::GetBroadcast();
	else
		return (*it).second.first; //add by jun
}

uint32_t
AquaSimDynamicRoutingTable::Size()
{
	return m_rt.size();
}

void
AquaSimDynamicRoutingTable::Update(t_table* newrt, AquaSimAddress Source_N) //add by jun
{
	DN tp;
	AquaSimAddress tmp;
	m_chg=0;


	if (Lookup(Source_N) == AquaSimAddress::GetBroadcast())
  {
		tp.first= Source_N;
		tp.second=1;
		AddEntry(Source_N, tp);
		m_chg=1;
	}

	for( t_table:: iterator it=newrt->begin(); it !=newrt->end(); it++)
  {

		if( it->first == NodeId() )
			continue;

		if (Lookup((*it).first) != AquaSimAddress::GetBroadcast() )
    {
			tmp = Lookup((*it).first);

			if (m_rt[it->first].second.GetAsInt() > (*it).second.second.GetAsInt() +1 )
      {
				RemoveEntry((*it).first);
				tp.first=Source_N;
				tp.second= AquaSimAddress((*it).second.second.GetAsInt() + 1);
				AddEntry((*it).first, tp);
				m_chg=1;
			}
		}
		else{
			tp.first= Source_N;
			tp.second=AquaSimAddress((*it).second.second.GetAsInt() + 1);
			AddEntry((*it).first, tp);
			m_chg=1;
		}
	}
}

/**** AquaSimDynamicRouting_PktTimer ****/

void
AquaSimDynamicRouting_PktTimer::Expire()
{
  m_routing->SendDRoutingPkt();
  //m_routing->ResetDRoutingPktTimer();
  double delay = (GetUpdateInterval() + m_routing->BroadcastJitter(10));
  Simulator::Schedule(Seconds(delay),&AquaSimDynamicRouting_PktTimer::Expire,this);
}


/**** AquaSimDynamicRouting ****/

NS_OBJECT_ENSURE_REGISTERED(AquaSimDynamicRouting);

AquaSimDynamicRouting::AquaSimDynamicRouting() : m_pktTimer(this, 50)
{
  AquaSimDynamicRouting(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
}

AquaSimDynamicRouting::AquaSimDynamicRouting(AquaSimAddress id) : m_pktTimer(this, 50)
{
  m_raAddr = id;
  m_rTable.SetNodeId(id);
  m_coun=0;

  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();

  m_pktTimer.SetFunction(&AquaSimDynamicRouting_PktTimer::Expire,&m_pktTimer);
  m_pktTimer.Schedule(Seconds(0.0000001+BroadcastJitter(10)));
}

TypeId
AquaSimDynamicRouting::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimDynamicRouting")
      .SetParent<AquaSimRouting>()
      .AddConstructor<AquaSimDynamicRouting>()
      .AddAttribute("AccessibleVar", "Accessible Variable.",
        IntegerValue(0),
        MakeIntegerAccessor (&AquaSimDynamicRouting::m_accessibleVar),
        MakeIntegerChecker<int> ())
    ;
  return tid;
}

/*int AquaSimDynamicRouting::command(int argc, const char*const* argv) {
  if (argc == 2) {
	if (strcasecmp(argv[1], "start") == 0) {
	  m_pktTimer.resched(0.0000001+broadcast_jitter(10));
	  return TCL_OK;
	}
	else if (strcasecmp(argv[1], "print_rtable") == 0) {
	  if (logtarget_ != 0) {
		sprintf(logtarget_->pt_->buffer(), "P %f _%d_ Routing Table",
				Simulator::Now(),
				RaAddr());
		logtarget_->pt_->dump();
	  }
	  else {
		fprintf(stdout, "%f _%d_ If you want to print this routing table "
						"you must create a trace file in your tcl script",
						Simulator::Now(),
						RaAddr());
	  }
	  return TCL_OK;
	}
  }
  else if (argc == 3) {
	// Obtains corresponding dmux to carry packets to upper layers
	if (strcmp(argv[1], "port-dmux") == 0) {
	  dmux_ = (PortClassifier*)TclObject::lookup(argv[2]);
	  if (dmux_ == 0) {
		fprintf(stderr, "%s: %s lookup of %s failed\n",
				__FILE__,
				argv[1],
				argv[2]);
		return TCL_ERROR;
	  }
	  return TCL_OK;
	}
	// Obtains corresponding tracer
	else if (strcmp(argv[1], "log-target") == 0 ||
	  strcmp(argv[1], "tracetarget") == 0) {
	  logtarget_ = (Trace*)TclObject::lookup(argv[2]);
	if (logtarget_ == 0)
	  return TCL_ERROR;
	return TCL_OK;
	  }
  }

}
*/


bool
AquaSimDynamicRouting::Recv(Ptr<Packet> p)
{
	AquaSimHeader ash;
  Ipv4Header iph;
	AquaSimPtTag ptag;
	p->RemoveHeader(ash);
  p->PeekPacketTag(ptag);
	//struct hdr_cmn* ch = HDR_CMN(p);
	//struct hdr_ip* ih = HDR_IP(p);
	//ch->uw_flag() = true;

	if (ash.GetSAddr() == RaAddr()) {
		// If there exists a loop, must drop the packet
		if (ash.GetNumForwards() > 0) {
      NS_LOG_INFO("Recv: there exists a loop, dropping packet=" << p);
			//drop(p, DROP_RTR_ROUTE_LOOP);
      p=0;
			return false;
		}
		// else if this is a packet I am originating, must add IP header
		/*else if (ash.GetNumForwards() == 0)
			TODO ash.size() += IP_HDR_LEN;*/
	}
	else if( ash.GetNextHop() != AquaSimAddress::GetBroadcast() && ash.GetNextHop() != RaAddr() )
  {
    NS_LOG_INFO("Recv: duplicate, dropping packet=" << p);
		//drop(p, DROP_MAC_DUPLICATE);
    p=0;
		return false;
	}

	uint8_t numForward = ash.GetNumForwards() + 1;
  ash.SetNumForwards(numForward);
  p->AddHeader(ash);

	// If it is a protoname packet, must process it
	if (ptag.GetPacketType() == AquaSimPtTag::PT_UW_DROUTING)
  {
		RecvDRoutingPkt(p);
		return true;
	}
	// Otherwise, must forward the packet (unless TTL has reached zero)
	else
  {
    p->RemoveHeader(iph);
    uint8_t ttl = iph.GetTtl() - 1;
		if (ttl == 0) {
      NS_LOG_INFO("Recv: RTR TTL == 0, dropping packet=" << p);
			//drop(p, DROP_RTR_TTL);
      p=0;
			return false;
		}
    iph.SetTtl(ttl);
    p->AddHeader(iph);
		ForwardData(p);
	}
  return true;
}

void
AquaSimDynamicRouting::RecvDRoutingPkt(Ptr<Packet> p)
{
	DRoutingHeader drh;
	p->PeekHeader(drh);

	// All routing messages are sent from and to port RT_PORT,
	// so we check it.
	//assert(ih->sport() == RT_PORT);
	//assert(ih->dport() == RT_PORT);
	// take out the packet, rtable

	DN temp_DN;
	t_table temp_rt;
	AquaSimAddress temp1;
	uint32_t size = p->GetSize();
	uint8_t *data = new uint8_t[size];
	p->CopyData(data,size);
	for(uint i=0; i < drh.GetEntryNum(); i++) {

		temp1 = *((AquaSimAddress*)data);
		data += sizeof(AquaSimAddress);

		temp_DN.first = *((AquaSimAddress*)data);
		//temp2 = *((AquaSimAddress*)data);
		data += sizeof(AquaSimAddress);

		//temp_DN[temp2]== *((int*)data);
		temp_DN.second = *((int*)data);
		data += sizeof(int);
		temp_rt[temp1]=temp_DN;
	}


	m_rTable.Update(&temp_rt, drh.GetPktSrc());

	if (m_rTable.IfChg() ==1) {
		m_pktTimer.SetUpdateInterval(30.0);
	}

	if (m_rTable.IfChg() ==0)
	{
		m_coun++;
	}

	if (m_coun ==2) {
		m_pktTimer.SetUpdateInterval(100.0);
		m_coun=0;
	}

	// Release resources
  p=0;
}

double
AquaSimDynamicRouting::BroadcastJitter(double range)
{
	return range*m_rand->GetValue();
}


void
AquaSimDynamicRouting::SendDRoutingPkt()
{
	Ptr<Packet> p = Create<Packet>();
  AquaSimHeader ash;
  DRoutingHeader drh;
  Ipv4Header iph;
  AquaSimPtTag ptag;

	//add by jun
	//struct hdr_uw_drouting_pkt* ph = HDR_UW_DROUTING_PKT(p);

	drh.SetPktSrc(RaAddr());
	drh.SetPktLen(7);
	drh.SetPktSeqNum(m_seqNum++);
	drh.SetEntryNum(m_rTable.Size());

	drh.SetPktLen(sizeof(drh.GetPktLen())+sizeof(drh.GetPktSrc()) +
                  sizeof(drh.GetPktSeqNum())+sizeof(drh.GetEntryNum()) );

  uint32_t size = (m_rTable.Size())*(3*sizeof(AquaSimAddress));
  uint8_t *data = new uint8_t[size];

	for(t_table::iterator it=m_rTable.m_rt.begin(); it!=m_rTable.m_rt.end(); it++)
  {
		*(AquaSimAddress*)data = it->first;
		data += sizeof(it->first);
		*(AquaSimAddress*)data = it->second.first;
		data += sizeof(it->second.first);
		*(AquaSimAddress*)data = it->second.second;
		data += sizeof(it->second.second);
	}

  Ptr<Packet> tempPacket = Create<Packet>(data,size);
  p->AddAtEnd(tempPacket);

  ptag.SetPacketType(AquaSimPtTag::PT_UW_DROUTING);

	ash.SetSAddr(RaAddr());
	ash.SetDAddr(AquaSimAddress::GetBroadcast());
	//ih->sport() = RT_PORT;
	//ih->dport() = RT_PORT;
	iph.SetTtl(1);

	ash.SetDirection(AquaSimHeader::DOWN);
	//ash.size() = IP_HDR_LEN + ph->pkt_len()+DataSize;
	ash.SetErrorFlag(false);
	ash.SetNextHop(AquaSimAddress::GetBroadcast());
	//ash.addr_type() = NS_AF_INET;
	//ash.uw_flag() = true;

  p->AddHeader(ash);
  p->AddHeader(drh);
  p->AddHeader(iph);
  p->AddPacketTag(ptag);
  Time jitter = Seconds(m_rand->GetValue()*0.5);
  Simulator::Schedule(jitter,&AquaSimRouting::SendDown,this,p,ash.GetNextHop(),jitter);
}

void
AquaSimDynamicRouting::ResetDRoutingPktTimer()
{
  m_pktTimer.Schedule( Seconds(50 + BroadcastJitter(10)) );
}

void
AquaSimDynamicRouting::ForwardData(Ptr<Packet> p)
{
  AquaSimHeader ash;
  p->RemoveHeader(ash);
	//struct hdr_ip* ih = HDR_IP(p);

	//double t = NOW;
	if (ash.GetDirection() == AquaSimHeader::UP &&
	    (ash.GetDAddr() == AquaSimAddress::GetBroadcast() || ash.GetDAddr() == RaAddr()))
  {
		//ash.size() -= IP_HDR_LEN;
    NS_LOG_INFO("ForwardData: dmux->recv not implemented yet for packet=" << p);
    //dmux_->recv(p, (Handler*)NULL); //should be sending to dmux
    //SendUp should handle dmux...
    if(!SendUp(p))
      NS_LOG_WARN("ForwardData: Something went wrong when passing packet up.");
		return;
	}
	else
  {
		ash.SetDirection(AquaSimHeader::DOWN);
		//ash.addr_type() = NS_AF_INET;
		if (ash.GetDAddr() == AquaSimAddress::GetBroadcast())
			ash.SetNextHop(AquaSimAddress::GetBroadcast());
		else
    {
			AquaSimAddress next_hop = m_rTable.Lookup(ash.GetDAddr());

			if (next_hop == AquaSimAddress::GetBroadcast())
      {
        NS_LOG_DEBUG("ForwardData: Node " << RaAddr() <<
            " can not forward a packet destined to " << ash.GetDAddr() <<
            " at time " << Simulator::Now().GetSeconds());
				//drop(p, DROP_RTR_NO_ROUTE);
        p=0;
				return;
			}
			else
				ash.SetNextHop(next_hop);
		}
    Simulator::Schedule(Seconds(0.0),&AquaSimRouting::SendDown,this,p,ash.GetNextHop(),Seconds(0));
	}
}
