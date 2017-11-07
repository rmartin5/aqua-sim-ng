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

#include "aqua-sim-routing-dbr.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-pt-tag.h"

#include "ns3/ipv4-header.h"
#include "ns3/log.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimDBR");

/*#define	BEACON_RESCHED				\
	beacon_timer_->resched(m_bInt + 		\
			Random::uniform(2*m_bDesync*m_bInt) - \
			m_bDesync*m_bInt);
*/

// parameters to control the delay
#define	DBR_MAX_DELAY	0.2	// maximal propagation delay for one hop
#define DBR_MAX_RANGE	100	// maximal transmmition range
#define DBR_MIN_BACKOFF	0.0	// minimal backoff time for the packet

#define USE_FLOODING_ALG 0	// test for pure flooding protocol
#define DBR_USE_ROUTEFLAG
#define DBR_MAX_HOPS	3
#define DBR_DEPTH_THRESHOLD 0.0
#define DBR_SCALE	1.0

DBR_BeaconTimer::~DBR_BeaconTimer()
{
	m_a=0;
	delete m_a;
}

void DBR_BeaconTimer::Expire()
{
	m_a->Beacon_Callback();
}

DBR_SendingTimer::~DBR_SendingTimer()
{
	m_a=0;
	delete m_a;
}

void DBR_SendingTimer::Expire()
{
	m_a->Send_Callback();
}

/*
void DBR_DeadNeighbTimer::expire(Event *e)
{
	a->deadneighb_callback(ne);
}
*/

QueueItemDbr::~QueueItemDbr()
{
	m_p=0;
}

// Insert the item into queue.
// The queue is sorted by the expected sending time
// of the packet.
void MyPacketQueue::insert(QueueItemDbr *q)
{
	QueueItemDbr *tmp;
	std::deque<QueueItemDbr*>::iterator iter;

	// find the insert point
	iter = m_dq.begin();
	while (iter != m_dq.end())
	{
		tmp = *iter;
		if (tmp->m_sendTime > q->m_sendTime)
		{
			m_dq.insert(iter, q);
			return;
		}
		iter++;
	}

	// insert at the end of the queue
	m_dq.push_back(q);
}

// Check if packet p in queue needs to be updated.
// If packet is not found, or previous sending time
// is larger than current one, return true.
// Otherwise return false.
bool
MyPacketQueue::update(Ptr<Packet> p, double t)
{
	Ptr<Packet> pkt;
	uint32_t curID;
	std::deque<QueueItemDbr*>::iterator iter;
  DBRHeader dbrh;
	AquaSimHeader ash;
	p->RemoveHeader(ash);
  p->PeekHeader(dbrh);
	p->AddHeader(ash);

	// get current packet ID
	curID = dbrh.GetPacketID();

	// search the queue
	iter = m_dq.begin();
	while (iter != m_dq.end())
	{
		((*iter)->m_p)->RemoveHeader(ash);
    ((*iter)->m_p)->PeekHeader(dbrh);
		((*iter)->m_p)->AddHeader(ash);
		if (dbrh.GetPacketID() == curID)
		{ // entry found
			if ((*iter)->m_sendTime > t)
			{
				m_dq.erase(iter);
				return true;
			}
			else
				return false;
		}
	}

	// not found
	return true;
}

// Find the item in queue which has the same packet ID
// as p, and remove it.
// If such a item is found, return true, otherwise
// return false.
bool
MyPacketQueue::purge(Ptr<Packet> p)
{
	Ptr<Packet> pkt;
	uint32_t curID;
	std::deque<QueueItemDbr*>::iterator iter;
  DBRHeader dbrh;
	AquaSimHeader ash;
	p->RemoveHeader(ash);
  p->PeekHeader(dbrh);
	p->AddHeader(ash);

	// get current packet ID
	curID = dbrh.GetPacketID();

	// search the queue
	iter = m_dq.begin();
	while (iter != m_dq.end())
	{
		((*iter)->m_p)->RemoveHeader(ash);
    ((*iter)->m_p)->PeekHeader(dbrh);
		((*iter)->m_p)->AddHeader(ash);
		if (dbrh.GetPacketID() == curID)
		{
			m_dq.erase(iter);
			return true;
		}
		iter++;
	}

	return false;
}

// Dump all the items in queue for debug
void MyPacketQueue::dump()
{
	std::deque<QueueItemDbr*>::iterator iter;
  DBRHeader dbrh;
	AquaSimHeader ash;
	int i = 0;

	iter = m_dq.begin();
	while (iter != m_dq.end())
	{
		((*iter)->m_p)->RemoveHeader(ash);
    ((*iter)->m_p)->PeekHeader(dbrh);
		((*iter)->m_p)->AddHeader(ash);
    NS_LOG_INFO("MyPacketQueue::dump:[" << i << "] packetID " <<
      dbrh.GetPacketID() << ", send time " << (*iter)->m_sendTime);
		iter++;
		i++;
	}
}

NeighbTable::NeighbTable(/*AquaSimDBR* a*/)
{
    int i;

    m_numEnts = 0;
    m_maxEnts = 100;

    // create the default table with size 100
    m_tab = new NeighbEnt* [100];
    //m_a = a;

    for (i = 0; i < 100; i++)
        m_tab[i] = new NeighbEnt();
}

NeighbTable::~NeighbTable()
{
    int i;

    for (i = 0; i < m_maxEnts; i++)
        delete m_tab[i];

    delete[] m_tab;
}

TypeId
NeighbTable::GetTypeId()
{
  static TypeId tid = TypeId("ns3::NeighbTable")
    ;
  return tid;
}

/*
static int neighbEntCmp(const void *a, const void *b)
{
	nsaddr_t ia = ((const NeighbEnt*)a)->net_id;
	nsaddr_t ib = ((const NeighbEnt*)b)->net_id;

	if (ia > ib) return 1;
	if (ia < ib) return -1;
	return 0;
}
*/

void NeighbTable::Dump(void)
{
	int i;

	for (i = 0; i < m_numEnts; i++)
  {
    NS_LOG_INFO("NeighbTable::dump: m_tab[" << i << "]: " << m_tab[i]->m_netID
      << " position(" << m_tab[i]->m_location.x << "," << m_tab[i]->m_location.y
      << "," << m_tab[i]->m_location.z << ")");
  }
}

void
NeighbTable::EntDelete(const NeighbEnt *ne)
{
	int l, r, m;
	int i;
	NeighbEnt *owslot;

	// binary search
	l = 0; r = m_numEnts - 1;
	while (l <= r)
	{
		m = l + (r - l)/2;
		if (m_tab[m]->m_netID < ne->m_netID)
			l = m + 1;
		else if  (ne->m_netID < m_tab[m]->m_netID)
			r = m - 1;
		else
			// m is the entry to be deleted
			break;
	}

	if (l > r)
		// no found!
		return;

	owslot = m_tab[m];

	// slide the entries
	i = m + 1;
	while (i < m_numEnts)
		m_tab[i - 1] = m_tab[i+1];

	m_tab[m_numEnts-1] = owslot;
	m_numEnts--;
}

#if 0
/**
 * Add a neighbor entry ne into the table.
 * The table is sorted by node address.
 */

NeighbEnt*
NeighbTable::EntAdd(const NeighbEnt *ne)
{
	int l, r, m;
	int i;
	NeighbEnt *owslot;

	if (m_numEnts >= m_maxEnts)
	{
		fprintf(stderr, "Neighbor table is full!\n");
		return 0;
	}

	// binary search
	l = 0; r = m_numEnts - 1;
	while (l <= r)
	{
		m = l + (r - l)/2;
		if (m_tab[m]->m_netID < ne->m_netID)
			l = m + 1;
		else if (m_tab[m]->m_netID > ne->m_netID)
			r = m - 1;
		else
		{
			// the entry is existing
			// update the info
			m_tab[m]->m_location.x = ne->m_location.x;
			m_tab[m]->m_location.y = ne->m_location.y;
			m_tab[m]->m_location.z = ne->m_location.z;

			return m_tab[m];
		}
	}

	// the entry should go to l
	owslot = m_tab[m_numEnts];

	// slide the entries after l
	i = m_numEnts - 1;
	while (i >= l)
		m_tab[i+1] = m_tab[i--];

	m_tab[l] = owslot;
	m_tab[l]->m_netID = ne->m_netID;
	m_tab[l]->m_location.x = ne->m_location.x;
	m_tab[l]->m_location.y = ne->m_location.y;
	m_tab[l]->m_location.z = ne->m_location.z;
	m_numEnts++;

	return m_tab[l];
}
#else
NeighbEnt*
NeighbTable::EntAdd(const NeighbEnt *ne)
{
	//NeighbEnt **pte;
	NeighbEnt *pe;
	int i, j;
	int l, r, m;

	// find if the neighbor is already existing
	for (i = 0; i < m_numEnts; i++)
		if (m_tab[i]->m_netID == ne->m_netID)
		{
			m_tab[i]->m_location.x = ne->m_location.x;
			m_tab[i]->m_location.y = ne->m_location.y;
			m_tab[i]->m_location.z = ne->m_location.z;

			return m_tab[i];
		}

	/*
	if (pte = (NeighbEnt**)bsearch(ne, m_tab, m_numEnts,
				sizeof(NeighbEnt *), neighbEntCmp))
	{
		(*pte)->m_netID = ne->m_netID;	// it doesn't hurt to rewrite it!
		(*pte)->m_location.x = ne->m_location.x;
		(*pte)->m_location.y = ne->m_location.y;
		(*pte)->m_location.z = ne->m_location.z;

		return (*pte);
	}
	*/

	// need we increase the size of table
	if (m_numEnts == m_maxEnts)
	{
		NeighbEnt **tmp = m_tab;
		m_maxEnts *= 2;			// double the space
		m_tab = new NeighbEnt* [m_maxEnts];
		bcopy(tmp, m_tab, m_numEnts*sizeof(NeighbEnt *));

		for (i = m_numEnts; i < m_maxEnts; i++)
			m_tab[i] = new NeighbEnt();

		delete[] tmp;
	}

	// get the insert point
	if (m_numEnts == 0)
		i = 0;
	else
	{
		l = 0;
		r = m_numEnts - 1;

		while (r > l)
		{
			m = l + (r - l) / 2;
			if (ne->m_netID < m_tab[m]->m_netID)
				r = m - 1;
			else
				l = m + 1;
		}

		if (r < l)
			i = r + 1;
		else
			if (ne->m_netID < m_tab[r]->m_netID)
				i = r;
			else
				i = r + 1;
	}

	// assign an unused slot to i
	if (i <= (m_numEnts - 1))
		pe = m_tab[m_numEnts];

	// adjust the entries after insert point i
	j = m_numEnts - 1;
	while (j >= i)
	{
		m_tab[j+1] = m_tab[j];
		j--;
	}

	if (i <= (m_numEnts - 1))
		m_tab[i] = pe;
	m_tab[i]->m_netID = ne->m_netID;
	m_tab[i]->m_location.x = ne->m_location.x;
	m_tab[i]->m_location.y = ne->m_location.y;
	m_tab[i]->m_location.z = ne->m_location.z;
	m_numEnts++;

	return m_tab[i];
}
#endif	// 0

/*
 * update the neighbor entry's routeFlag field with va
 */

void NeighbTable::UpdateRouteFlag(AquaSimAddress addr, int val)
{
	int i;

	for (i = 0; i < m_numEnts; i++)
	{
		if (m_tab[i]->m_netID == addr)
		{
			m_tab[i]->m_routeFlag = val;
			return;
		}
	}
}

#ifdef	DBR_USE_ROUTEFLAG
NeighbEnt *
NeighbTable::EntFindShadowest(Vector location)
{
	NeighbEnt *ne = 0;
	int i;
	double t;

  t = location.z;

  NS_LOG_DEBUG("NeighbTable::EntFindShadowest: location=(" <<
      location.x << "," << location.y << "," << location.z <<
			") has " << m_numEnts << " neighbors");

	for (i = 0; i < m_numEnts; i++)
	{
    NS_LOG_DEBUG("NeighbTable::EntFindShadowest: [" << m_tab[i]->m_netID <<
			"] position(" << m_tab[i]->m_location.x << "," <<
      m_tab[i]->m_location.y << "," << m_tab[i]->m_location.z << ")");

		if (m_tab[i]->m_routeFlag == 1)
		{
			ne = m_tab[i];
			return ne;
		}

		if (m_tab[i]->m_location.z > t)
		{
		    t = m_tab[i]->m_location.z;
		    ne = m_tab[i];
		}
	}
	return ne;
}
#else
NeighbEnt *
NeighbTable::EntFindShadowest(Vector location)
{
    NeighbEnt *ne = 0;
    int i;
    double t;

    t = location.z;

    for (i = 0; i < m_numEnts; i++)
    {
  NS_LOG_DEBUG("NeighbTable::EntFindShadowest: " << m_a->GetAddress() <<
    "[" << m_tab[i]->m_netID << "] position(" << m_tab[i]->m_location.x << "," <<
    m_tab[i]->m_location.y << "," << m_tab[i]->m_location.z << ")");

        if (m_tab[i]->m_location.z > t)
        {
            t = m_tab[i]->m_location.z;
            ne = m_tab[i];
        }
    }

    return ne;
}
#endif	// DBR_USE_ROUTEFLAG

/*
 *  ASPktCache
 */
NS_OBJECT_ENSURE_REGISTERED(ASPktCache);

ASPktCache::ASPktCache()
{
	//int i;

	m_maxSize = 1500;
	m_size = 0;

	m_pCache = new int[1500];

	//for (i = 0; i < 100; i++)
	//	m_pCache[i] = new Packet;
}

ASPktCache::~ASPktCache()
{
	//int i;

	//for (i = 0; i < m_maxSize; i++)
	//	delete m_pCache[i];

	delete[] m_pCache;
}

TypeId
ASPktCache::GetTypeId()
{
  static TypeId tid = TypeId("ns3::ASPktCache")
    ;
  return tid;
}

int
ASPktCache::AccessPacket(int p)
{
	int i, j;
	int tmp;

	for (i = 0; i < m_size; i++) {
		if (m_pCache[i] == p) {
			// if the pkt is existing
			// put it to the tail
			tmp = p;
			for (j = i; j < m_size - 1; j++)
				m_pCache[j] = m_pCache[j+1];
			m_pCache[m_size-1] = tmp;

			return 1;
		}
	}

	return 0;
}

void
ASPktCache::AddPacket(int p)
{
	if (m_size == m_maxSize) {
    NS_LOG_WARN("Cache is full!");
		return;
	}

	m_pCache[m_size] = p;
	m_size++;

	return;
}

void
ASPktCache::DeletePacket(int p)
{
}

void
ASPktCache::Dump(void)
{
	int i;

	for (i = 0; i < m_size; i++)
  {
    NS_LOG_INFO("[" << i << "]: " << m_pCache[i]);
  }
}

#if 0
// test main function
int main(void)
{
	ASPktCache pc;
	Packet p1, p2, p3;

	pc.AddPacket(&p1);
	pc.AddPacket(&p2);
	pc.AddPacket(&p3);

	pc.Dump();

	pc.AccessPacket(&p1);
	pc.Dump();

	return 0;
}
#endif

/*
 *  AquaSimDBR
 */
NS_OBJECT_ENSURE_REGISTERED(AquaSimDBR);

AquaSimDBR::AquaSimDBR() :
	m_bInt(DBR_BEACON_INT), m_bDesync(DBR_BEACON_DESYNC), m_pktCnt(0)
{
	m_nTab = new NeighbTable();

	// create packet cache
	m_pc = new ASPktCache();

	m_rand = CreateObject<UniformRandomVariable> ();

  // setup the timer
	m_beaconTimer = new DBR_BeaconTimer(this);
  m_beaconTimer->SetFunction(&DBR_BeaconTimer::Expire,m_beaconTimer);
  m_beaconTimer->Schedule(Seconds(m_rand->GetValue(0.0,m_bInt)));

	m_sendTimer = new DBR_SendingTimer(this);
  m_sendTimer->SetFunction(&DBR_SendingTimer::Expire,m_sendTimer);
	//m_sendTimer->Schedule(Seconds(m_rand->GetValue(0.0,m_bInt)));
}

AquaSimDBR::~AquaSimDBR()
{
	delete m_nTab;

	// packet cache
	delete m_pc;
}

TypeId
AquaSimDBR::GetTypeId()
{
  static TypeId tid = TypeId ("ns3::AquaSimDBR")
    .SetParent<AquaSimRouting> ()
    .AddConstructor<AquaSimDBR> ()
  ;
  return tid;
}

int64_t
AquaSimDBR::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_rand->SetStream(stream);
  return 1;
}

// construct the beacon packet
Ptr<Packet>
AquaSimDBR::MakeBeacon(void)
{
	Ptr<Packet> p = Create<Packet>();
	//NS_ASSERT(p != NULL);

  AquaSimHeader ash;
  DBRHeader dbrh;
  AquaSimPtTag ptag;

	// setup header
	ash.SetNextHop(AquaSimAddress::GetBroadcast());
	//ash.addr_type_ = AF_INET;
  ash.SetDirection(AquaSimHeader::DOWN);
	ash.SetSize(dbrh.Size() + IP_HDR_LEN);
  ash.SetDAddr(AquaSimAddress::GetBroadcast());
  ash.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
  //ash->dport() = DBR_PORT;

  ptag.SetPacketType(AquaSimPtTag::PT_DBR);

  Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
  if (model == 0)
    {
      NS_LOG_DEBUG("MobilityModel does not exist for device " << GetNetDevice());
    }

  // fill in the location info
  dbrh.SetPosition(model->GetPosition());
  dbrh.SetMode(DBRH_BEACON);
	dbrh.SetNHops(1);

	p->AddHeader(dbrh);
  p->AddHeader(ash);
  p->AddPacketTag(ptag);
	return p;
}

// send beacon pkt only once at the beginning
void
AquaSimDBR::SendBeacon(void)
{
	Ptr<Packet> p = MakeBeacon();
  AquaSimHeader ash;
  p->RemoveHeader(ash);

	if (p)
	{
		NS_ASSERT(!ash.GetErrorFlag());
		if (ash.GetDirection() == AquaSimHeader::UP)
			ash.SetDirection(AquaSimHeader::DOWN);
    p->AddHeader(ash);
    Simulator::Schedule(Seconds(0),&AquaSimRouting::SendDown,this,
                          p,AquaSimAddress::GetBroadcast(),Seconds(0));
	}
	else
	{
    NS_LOG_WARN("AquaSimDBR::SendBeacon: ERROR, can't make new beacon!");
		//abort();
	}
}

// fetch the packet from the sending queue
// and broadcast.
void
AquaSimDBR::Send_Callback(void)
{
	QueueItemDbr *q;
  DBRHeader dbrh;
	AquaSimHeader ash;

	// we're done if there is no packet in queue
	if (m_pq.empty())
		return;

	// send the first packet out
	q = m_pq.front();
	m_pq.pop();
  Simulator::Schedule(Seconds(0),&AquaSimRouting::SendDown,this,
                        q->m_p,AquaSimAddress::GetBroadcast(),Seconds(0));

	// put the packet into cache
	(q->m_p)->RemoveHeader(ash);
  (q->m_p)->PeekHeader(dbrh);
	(q->m_p)->AddHeader(ash);
	m_pc->AddPacket(dbrh.GetPacketID());

	// reschedule the timer if there are
	// other packets in the queue
	if (!m_pq.empty())
	{
		q = m_pq.front();
		m_latest = q->m_sendTime;
    m_sendTimer->Schedule(Seconds(m_latest - Simulator::Now().ToDouble(Time::S)));
	}
}

void
AquaSimDBR::Beacon_Callback(void)
{
	return;  //...

	Ptr<Packet> p = MakeBeacon();

  AquaSimHeader ash;
  p->RemoveHeader(ash);

	if (p)
	{
		NS_ASSERT(!ash.GetErrorFlag());
		if (ash.GetDirection() == AquaSimHeader::UP)
        ash.SetDirection(AquaSimHeader::DOWN);
		//fprintf(stderr, "%d is broadcasting beacon pkt src: %d, dst: %d\n",
		//		mn_->address(), iph->saddr(), iph->daddr());

    p->AddHeader(ash);
    Simulator::Schedule(Seconds(m_rand->GetValue()*DBR_JITTER),&AquaSimRouting::SendDown,this,
                          p,AquaSimAddress::GetBroadcast(),Seconds(0));
	}
	else
	{
    NS_LOG_WARN("AquaSimDBR::Beacon_Callback: Error, Can't make new beacon!");
		exit(-1);
	}
	//BEACON_RESCHED
}

void
AquaSimDBR::DeadNeighb_Callback(NeighbEnt *ne)
{
	m_nTab->EntDelete(ne);
}

void
AquaSimDBR::ForwardPacket(Ptr<Packet> p, int flag)
{
	NS_LOG_FUNCTION(this);

  AquaSimHeader ash;
  DBRHeader dbrh;
  AquaSimPtTag ptag;
  p->RemoveHeader(ash);
  p->RemoveHeader(dbrh);
  p->RemovePacketTag(ptag);
	NeighbEnt *ne;

	double delay = 0.0;

	Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
  if (model == 0)
    {
      NS_LOG_DEBUG("MobilityModel does not exist for device " << GetNetDevice());
    }

  NS_LOG_DEBUG("AquaSimDBR::DeadNeighb_Callback: node=" <<
      GetNetDevice()->GetAddress() << " is forwarding to " << ash.GetDAddr());
      //Address::instance().get_nodeaddr(iph->daddr()));

	// common settings for forwarding
  ash.SetDirection(AquaSimHeader::DOWN);
	//ash->addr_type_ = AF_INET;
	ash.SetSize(dbrh.Size() + IP_HDR_LEN);
  ptag.SetPacketType(AquaSimPtTag::PT_DBR);

	// make decision on next hop based on packet mode
	switch (dbrh.GetMode())
	{
	case DBRH_DATA_GREEDY:
		ne = m_nTab->EntFindShadowest(model->GetPosition());
		if (ne)
		{
			ash.SetNextHop(ne->m_netID);
      NS_LOG_DEBUG("AquaSimDBR::DeadNeighb_Callback: node=" <<
          GetNetDevice()->GetAddress() << " -> " << ne->m_netID);
		}
		else
		{
      NS_LOG_DEBUG("AquaSimDBR::DeadNeighb_Callback: " <<
        GetNetDevice()->GetAddress() << " put pkt into recovery mode!");

			// put packet into recovery mode
			ash.SetNextHop(AquaSimAddress::GetBroadcast());
			dbrh.SetMode(DBRH_DATA_RECOVER);
			dbrh.SetPrevHop(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
			dbrh.SetOwner(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
			dbrh.SetNHops(DBR_MAX_HOPS);		// set the range of broadcasting by hops
		}
		break;
	case DBRH_DATA_RECOVER:
    NS_LOG_DEBUG("AquaSimDBR::DeadNeighb_Callback: " <<
      GetNetDevice()->GetAddress() << " forward in recovery alogrithm!");

		if (m_pc->AccessPacket(dbrh.GetPacketID()))
		{
			// each node only broadcasts once
      NS_LOG_DEBUG("AquaSimDBR::DeadNeighb_Callback: " <<
        GetNetDevice()->GetAddress() << " packet is already in cache!");

			return;
		}
		else
			m_pc->AddPacket(dbrh.GetPacketID());

		// can we find other greedy node?
		ne = m_nTab->EntFindShadowest(model->GetPosition());
		if (ne == NULL)
		{
			if (dbrh.GetNHops() <= 0)
			{
        NS_LOG_DEBUG("AquaSimDBR::ForwardPacket: " <<
            GetNetDevice()->GetAddress() << " drops pkt! (nhops < 0)");

        p=0;
        //drop(p, DROP_RTR_TTL);
				return;
			}
			else
			{
				// broadcasting
				ash.SetNextHop(AquaSimAddress::GetBroadcast());
				dbrh.SetMode(DBRH_DATA_RECOVER);
				dbrh.SetOwner(dbrh.GetPrevHop());
				dbrh.SetPrevHop(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
				dbrh.SetNHops((dbrh.GetNHops()-1));

				// set broadcasting delay
				delay = m_rand->GetValue() * DBR_JITTER;
			}
		}
		else if (ne->m_netID != dbrh.GetPrevHop())
		{
			// new route is found, put pkt back to greedy alg
			ash.SetNextHop(ne->m_netID);
			dbrh.SetMode(DBRH_DATA_GREEDY);
      NS_LOG_DEBUG("AquaSimDBR::ForwardPacket: back to greedy:" <<
          GetNetDevice()->GetAddress() << " -> " << ne->m_netID);
		}
		else
		{
      NS_LOG_DEBUG("AquaSimDBR::ForwardPacket: address:" <<
          GetNetDevice()->GetAddress() << ":dbrh->nhops=" << dbrh.GetNHops());

			if (dbrh.GetNHops() <= 0)
			{
        NS_LOG_DEBUG("AquaSimDBR::ForwardPacket: " <<
            GetNetDevice()->GetAddress() << " drops pkt! (nhops < 0)");

        p=0;
				//drop(p, DROP_RTR_TTL);
				return;
			}
			else
			{
        NS_LOG_DEBUG("AquaSimDBR::ForwardPacket: " <<
            GetNetDevice()->GetAddress() << " is broadcasting!");

				// broadcasting
				ash.SetNextHop(AquaSimAddress::GetBroadcast());
        dbrh.SetMode(DBRH_DATA_RECOVER);
        dbrh.SetOwner(dbrh.GetPrevHop());
        dbrh.SetPrevHop(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
        dbrh.SetNHops((dbrh.GetNHops()-1));

				// set broadcasting delay
				delay = m_rand->GetValue() * DBR_JITTER;
			}
		}
		break;
	default:
    NS_LOG_WARN("AquaSimDBR::ForwardPacket: Wrong! The packet can't be forwarded!");
		//abort();
		break;
	}

	// schedule the sending
	NS_ASSERT(!ash.GetErrorFlag());
	p->AddHeader(dbrh);
  p->AddHeader(ash);
  p->AddPacketTag(ptag);
  Simulator::Schedule(Seconds(delay),&AquaSimRouting::SendDown,this,
                        p,ash.GetNextHop(),Seconds(0));
}

// get the info from beacon pkt
void
AquaSimDBR::BeaconIn(Ptr<Packet> p)
{
  AquaSimHeader ash;
  DBRHeader dbrh;
  p->RemoveHeader(ash);
  p->PeekHeader(dbrh);
	p->AddHeader(ash);

  AquaSimAddress src = ash.GetSAddr();
	//nsaddr_t src = Address::instance().get_nodeaddr(iph->saddr());

	// create NeighbEnt
	NeighbEnt *ne;
	ne = new NeighbEnt();

  NS_LOG_DEBUG("AquaSimDBR::BeaconIn: " << GetNetDevice()->GetAddress() <<
    " got beacon from " << src);

	ne->m_location.x = dbrh.GetPosition().x;
	ne->m_location.y = dbrh.GetPosition().y;
	ne->m_location.z = dbrh.GetPosition().z;
	ne->m_netID = src;

	m_nTab->EntAdd(ne);

	delete ne;

	// we consumed the packet, free it!
	p=0;
}

#if 1
bool
AquaSimDBR::Recv(Ptr<Packet> p, const Address &dest, uint16_t protocolNumber)
{
	NS_LOG_FUNCTION(this);

  AquaSimHeader ash;
  DBRHeader dbrh;
  //Ipv4Header iph;
  AquaSimPtTag ptag;

	p->RemoveHeader(ash);
  if (ash.GetNumForwards() <= 0)  //no headers //TODO create specalized Application instead of using this hack.
	{
		//p->AddHeader(iph);
		//populate dbr header??
		p->AddHeader(dbrh);
	}

  p->PeekHeader(dbrh);
  p->AddHeader(ash);
  p->PeekPacketTag(ptag);

	//double x, y, z;

  AquaSimAddress src = ash.GetSAddr();
  AquaSimAddress dst = ash.GetDAddr();
	//nsaddr_t src = Address::instance().get_nodeaddr(iph->saddr());
	//nsaddr_t dst = Address::instance().get_nodeaddr(iph->daddr());

  Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
  if (model == 0)
    {
      NS_LOG_DEBUG("MobilityModel does not exist for device " << GetNetDevice());
    }

	//mn_->getLoc(&x, &y, &z);

	if (dbrh.GetMode() == DBRH_BEACON)
	{// although we eliminate the beacon pkt
	 // but we still reserve the handler for
	 // furture control logic

    NS_LOG_DEBUG("AquaSimDBR::Recv: address:" <<
        GetNetDevice()->GetAddress() << ": beacon pkt is received.");

		// self is not one of the neighbors
		if (src != GetNetDevice()->GetAddress())
			BeaconIn(p);
		return true;
	}

	if ((src == GetNetDevice()->GetAddress()) &&
	    (ash.GetNumForwards() == 0))
	{// packet I'm originating

  p->RemoveHeader(ash);
  p->RemoveHeader(dbrh);
//  p->RemoveHeader(iph);
  p->RemovePacketTag(ptag);

	ash.SetDirection(AquaSimHeader::DOWN);
	//ash->addr_type_ = AF_INET;
  ptag.SetPacketType(AquaSimPtTag::PT_DBR);
	ash.SetSize(dbrh.Size() + IP_HDR_LEN);
	ash.SetNextHop(AquaSimAddress::GetBroadcast());
	//iph.SetTtl(128);

	// setup DBR header
	dbrh.SetMode(DBRH_DATA_GREEDY);
	//dbrh.GetPacketID((int)GetNetDevice()->GetAddress());
	dbrh.SetPacketID(m_pktCnt++);
	dbrh.SetDepth(model->GetPosition().z);		// save the depth info

	// broadcasting the pkt
	NS_ASSERT(!ash.GetErrorFlag());
	//p->AddHeader(iph);
	p->AddHeader(dbrh);
  p->AddHeader(ash);
  p->AddPacketTag(ptag);
  Simulator::Schedule(Seconds(0),&AquaSimRouting::SendDown,this,
                        p,ash.GetNextHop(),Seconds(0));
	return true;
	}

	if ((src == GetNetDevice()->GetAddress()) &&
	    (dbrh.GetMode() == DBRH_DATA_GREEDY))
	{// Wow, it seems some one is broadcasting the pkt for us,
	 // so we need to dismiss the timer for the pkt

    NS_LOG_DEBUG("AquaSimDBR::Recv: address:" <<
        GetNetDevice()->GetAddress() << ": got the pkt I've sent");

    p=0;
		//drop(p, DROP_RTR_ROUTE_LOOP);
		return false;
	}

	if (dst == GetNetDevice()->GetAddress())
	{// packet is for me

    NS_LOG_DEBUG("AquaSimDBR::Recv: address:" <<
        GetNetDevice()->GetAddress() << ": packet is delivered!");

		// we may need to send it to upper layer agent
    if (!SendUp(p))
		  NS_LOG_WARN("DataForSink: Something went wrong when passing packet up to dmux.");

		return true;
	}

	// packet I'm forwarding
	HandlePktForward(p);
	return true;
}

#ifdef USE_FLOODING_ALG
void
AquaSimDBR::HandlePktForward(Ptr<Packet> p)
{
	AquaSimHeader ash;
	DBRHeader dbrh;
	AquaSimPtTag ptag;
	//Ipv4Header iph;
  p->RemoveHeader(ash);
	p->PeekHeader(dbrh);
  //p->PeekHeader(iph);
  p->RemovePacketTag(ptag);

	/*int ttl_ = iph.GetTtl()-1;
	iph.SetTtl(ttl_);
	if (ttl_ == 0)
	{
    p=0;
		//drop(p, DROP_RTR_TTL);
		return;
	}*/

	// Is this pkt recieved before?
	// each node only broadcasts same pkt once
	if (m_pc->AccessPacket(dbrh.GetPacketID()))
	{
    p=0;
		//drop(p, DROP_RTR_TTL);
		return;
	}
	else
		m_pc->AddPacket(dbrh.GetPacketID());

	// common settings for forwarding
	ash.SetNumForwards((ash.GetNumForwards()+1));
	ash.SetDirection(AquaSimHeader::DOWN);
	//ash->addr_type_ = AF_INET;
	ptag.SetPacketType(AquaSimPtTag::PT_DBR);
	ash.SetSize(dbrh.Size() + IP_HDR_LEN);
	ash.SetNextHop(AquaSimAddress::GetBroadcast());

	// finally broadcasting it!
	NS_ASSERT(!ash.GetErrorFlag());
//	p->AddHeader(dbrh);
  p->AddHeader(ash);
  p->AddPacketTag(ptag);
  Simulator::Schedule(Seconds(m_rand->GetValue()*DBR_JITTER),
                        &AquaSimRouting::SendDown,this,
                        p,AquaSimAddress::GetBroadcast(),Seconds(0));
}
#else
// Forward the packet according to its mode
// There are two modes right now: GREEDY and RECOVERY
// The node will broadcast all RECOVERY pakets, but
// it will drop the GREEDY pakets from upper level.
void
AquaSimDBR::HandlePktForward(Ptr<Packet> p)
{
  AquaSimHeader ash;
  DBRHeader dbrh;
//  Ipv4Header iph;
  AquaSimPtTag ptag;
  p->RemoveHeader(ash);
  p->RemoveHeader(dbrh);
  //p->PeekHeader(iph);
  p->RemovePacketTag(ptag);

	double delta;
	double delay = .0;

	//double x, y, z;
  Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
  if (model == 0)
    {
      NS_LOG_DEBUG("MobilityModel does not exist for device " << GetNetDevice());
    }

	/*if ((iph.GetTtl()-1) == 0)
	{
    p=0;
		//drop(p, DROP_RTR_TTL);
		return;
	}*/

	/*
	// dump the queue
	fprintf(stderr, "-------- Node id: %d --------\n", mn_->address());
	fprintf(stderr, " curID: %d\n", dbrh->packetID());
	m_pq.dump();
	*/

#if 0
	// search sending queue for p
	if (m_pq.purge(p))
	{
		drop(p, DROP_RTR_TTL);
		return;
	}
#endif

	// common settings for forwarding
  ash.SetNumForwards((ash.GetNumForwards()+1));
  ash.SetDirection(AquaSimHeader::DOWN);
	//ash->addr_type_ = AF_INET;
	ptag.SetPacketType(AquaSimPtTag::PT_DBR);
	ash.SetSize(dbrh.Size() + IP_HDR_LEN);
	ash.SetNextHop(AquaSimAddress::GetBroadcast());

	switch (dbrh.GetMode())
	{
	case DBRH_DATA_GREEDY:
		//mn_->getLoc(&x, &y, &z);

		// compare the depth
		delta = model->GetPosition().z - dbrh.GetDepth();

		// only forward the packet from lower level
		if (delta < DBR_DEPTH_THRESHOLD)
		{
			//p->AddHeader(dbrh);
      p->AddHeader(ash);
      p->AddPacketTag(ptag);
			m_pq.purge(p);
      p=0;
      //drop(p, DROP_RTR_TTL);
			return;
		}

    NS_LOG_DEBUG("[" << GetNetDevice()->GetAddress() << "]: z=" <<
      model->GetPosition().z << ", depth=" << dbrh.GetDepth() <<
      ", delta=" << delta);

		// update current depth
		dbrh.SetDepth(model->GetPosition().z);

		// compute the delay
		//delay = DBR_DEPTH_THRESHOLD / delta * DBR_SCALE;
		delta = 1.0 - delta / DBR_MAX_RANGE;
		delay = DBR_MIN_BACKOFF + 4.0 * delta * DBR_MAX_DELAY;

		// set time out for the packet

		break;
	case DBRH_DATA_RECOVER:
		if (dbrh.GetNHops() <= 0)
		{
      NS_LOG_DEBUG("AquaSimDBR::Recv: address:" <<
          GetNetDevice()->GetAddress() << ": drops pkt! (nhops < 0)");

      p=0;
			//drop(p, DROP_RTR_TTL);
			return;
		}
		dbrh.SetNHops((dbrh.GetNHops()-1));
		break;
	default:
    NS_LOG_WARN("AquaSimDBR::Recv: Unknown data type!");
		return;
	}

	// make up the DBR header
	dbrh.SetOwner(dbrh.GetPrevHop());
	dbrh.SetPrevHop(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));

  NS_LOG_DEBUG("[" << GetNetDevice()->GetAddress() << "]: delay " <<
    delay << " before broadcasting!");

	if (m_pc == NULL) {
    NS_LOG_WARN("AquaSimDBR::Recv: packet cache pointer is null!\n");
		exit(-1);
	}

	// Is this pkt recieved before?
	// each node only broadcasts the same pkt once
	if (m_pc->AccessPacket(dbrh.GetPacketID()))
	{
		p=0;
    //drop(p, DROP_RTR_TTL);
		return;
	}
	//else
	//	m_pc->AddPacket(dbrh.GetPacketID());

	// put the packet into sending queue
	double expected_send_time = Simulator::Now().ToDouble(Time::S) + delay;

	p->AddHeader(dbrh);
  p->AddHeader(ash);
  p->AddPacketTag(ptag);
	QueueItemDbr *q = new QueueItemDbr(p, expected_send_time);

	/*
	m_pq.insert(q);
	QueueItemDbr *qf;
	qf = m_pq.front();
	send_timer_->resched(qf->m_sendTime - NOW);
	*/

	if (m_pq.empty())
	{
		m_pq.insert(q);
		m_latest = expected_send_time;
    m_sendTimer->Schedule(Seconds(delay));
	}
	else
	{
		if (m_pq.update(p, expected_send_time))
		{
			m_pq.insert(q);

			// update the sending timer
			if (expected_send_time < m_latest)
			{
				m_latest = expected_send_time;
        m_sendTimer->Schedule(Seconds(delay));
			}
		}
	}
}
#endif	// end of USE_FLOODING_ALG
#else
bool
AquaSimDBR::Recv(Ptr<Packet> p, const Address &dest, uint16_t protocolNumber)
{
	NS_LOG_FUNCTION(this);

  AquaSimHeader ash;
  DBRHeader dbrh;
  //Ipv4Header iph;
	if (p->GetSize() <= 32)
	{
		//p->AddHeader(iph);
		p->AddHeader(dbrh);
		p->AddHeader(ash);
	}

  p->RemoveHeader(ash);
  p->PeekHeader(dbrh);
	p->AddHeader(ash);

  AquaSimAddress src = ash.GetSAddr();
  AquaSimAddress dst = ash.GetDAddr();
	//nsaddr_t src = Address::instance().get_nodeaddr(iph->saddr());
	//nsaddr_t dst = Address::instance().get_nodeaddr(iph->daddr());

  //NS_LOG_DEBUG("AquaSimDBR::Recv: address:" << GetNetDevice()->GetAddress() <<
  //  " receives pkt from " << src << " to " << dst);
  Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
  if (model == 0)
    {
      NS_LOG_DEBUG("MobilityModel does not exist for device " << GetNetDevice());
    }

	if (dbrh.GetMode() == DBRH_BEACON)
	{// beacon packet
		// self is not one of the neighbors
		if (src != GetNetDevice()->GetAddress())
			BeaconIn(p);
		return true;
	}
	else if ((src == GetNetDevice()->GetAddress()) &&
		(ash.GetNumForwards() == 0))
	{// packet I'm originating

    NS_LOG_DEBUG("AquaSimDBR::Recv: " << src << " generates data packet.");

    //edit headers
    p->RemoveHeader(ash);
    p->RemoveHeader(dbrh);
    //p->RemoveHeader(iph);

    ash.SetDirection(AquaSimHeader::DOWN);
    ash.SetSize(ash.GetSize() + IP_HDR_LEN + 8);
    //iph.SetTtl(128);
		dbrh.SetMode(DBRH_DATA_GREEDY);
		dbrh.SetPacketID(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt());
	}
	else if ((src == GetNetDevice()->GetAddress()) &&
		(dbrh.GetMode() == DBRH_DATA_GREEDY))
	{// duplicate packet, discard

    NS_LOG_DEBUG("AquaSimDBR::Recv: got the pkt I've sent.");

    p=0;
		//drop(p, DROP_RTR_ROUTE_LOOP);
		return false;
	}
	else if (dst == GetNetDevice()->GetAddress())
	{// packet is for me

    NS_LOG_DEBUG("Packet is delivered!");
		//p->AddHeader(iph);
		p->AddHeader(dbrh);
    p->AddHeader(ash);
		// we may need to send it to upper layer agent
		if (!SendUp(p))
		  NS_LOG_WARN("DataForSink: Something went wrong when passing packet up to dmux.");

		return true;
	}
/*------------------------------------------------
	else if (dst == IP_BROADCAST)
	{
		if (dbrh->mode() == DBRH_BEACON)
		{
			// self is not one of the neighbors
			if (src != mn_->address())
				beaconIn(p);
			return;
		}
	}
------------------------------------------------*/
	else
	{// packet I'm forwarding
		/*int ttl_ = iph.GetTtl()-1;
		iph.SetTtl(ttl_);
		if (ttl_ == 0)
		{
      p=0;
			//drop(p, DROP_RTR_TTL);
			return false;
		}*/

		if((dbrh.GetMode() == DBRH_DATA_RECOVER) &&
			(dbrh.GetOwner() == GetNetDevice()->GetAddress()))
		{
			//fprintf(stderr, "got the pkt I've sent\n");
      p=0;
      //drop(p, DROP_RTR_ROUTE_LOOP);
			// it seems this neighbor couldn't find a greedy node
			//m_nTab->updateRouteFlag(dbrh->prev_hop(), 0);
			return false;
		}
	}

  NS_LOG_DEBUG("AquaSimDBR::Recv: owner:" << dbrh.GetOwner() << ", prev-hop:"
    << dbrh.GetPrevHop() << ", cur:" << GetNetDevice()->GetAddress());

	// it's time to forward the pkt now
	//p->AddHeader(iph);
	p->AddHeader(dbrh);
  p->AddHeader(ash);
	ForwardPacket(p);
  return true;
}
#endif

bool
AquaSimDBR::Recv2(Ptr<Packet> p, const Address &dest, uint16_t protocolNumber)
{
  AquaSimHeader ash;
  DBRHeader dbrh;
  //Ipv4Header iph;
	if (p->GetSize() <= 32)
	{
		//p->AddHeader(iph);
		p->AddHeader(dbrh);
		p->AddHeader(ash);
	}

  p->RemoveHeader(ash);
  p->PeekHeader(dbrh);
	p->AddHeader(ash);

  AquaSimAddress src = ash.GetSAddr();
  AquaSimAddress dst = ash.GetDAddr();
	//nsaddr_t src = Address::instance().get_nodeaddr(iph->saddr());
	//nsaddr_t dst = Address::instance().get_nodeaddr(iph->daddr());

  //NS_LOG_DEBUG("AquaSimDBR::Recv2: address:" << GetNetDevice()->GetAddress()
  //  << " receives pkt from " << src << " to " << dst);

  Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
  if (model == 0)
    {
      NS_LOG_DEBUG("MobilityModel does not exist for device " << GetNetDevice());
    }

	if (dbrh.GetMode() == DBRH_BEACON)
	{// beacon packet

		// self is not one of the neighbors
		if (src != GetNetDevice()->GetAddress())
			BeaconIn(p);
		return true;
	}
	else if ((src == GetNetDevice()->GetAddress()) &&
		(ash.GetNumForwards() == 0))
	{// packet I'm originating

    NS_LOG_DEBUG("AquaSimDBR::Recv2: " << src << " generates data packet.");

    //edit headers
    p->RemoveHeader(ash);
    p->RemoveHeader(dbrh);
    //p->RemoveHeader(iph);

		ash.SetSize(ash.GetSize() + IP_HDR_LEN + 8);
    ash.SetDirection(AquaSimHeader::DOWN);
    //iph.SetTtl(128);
    dbrh.SetMode(DBRH_DATA_GREEDY);
    dbrh.SetPacketID(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt());
	}
	else if ((src == GetNetDevice()->GetAddress()) &&
		(dbrh.GetMode() == DBRH_DATA_GREEDY))
	{// duplicate packet, discard

    NS_LOG_DEBUG("AquaSimDBR::Recv2: got the pkt I've sent");

    p=0;
		//drop(p, DROP_RTR_ROUTE_LOOP);
		return false;
	}
	else if (dst == GetNetDevice()->GetAddress())
	{// packet is for me

    NS_LOG_DEBUG("Packet is delivered!");
		//p->AddHeader(iph);
		p->AddHeader(dbrh);
    p->AddHeader(ash);
		// we may need to send it to upper layer agent
		if (!SendUp(p))
		  NS_LOG_WARN("DataForSink: Something went wrong when passing packet up to dmux.");

		return false;
	}
/*------------------------------------------------
	else if (dst == IP_BROADCAST)
	{
		if (dbrh->mode() == DBRH_BEACON)
		{
			// self is not one of the neighbors
			if (src != mn_->address())
				beaconIn(p);
			return;
		}
	}
------------------------------------------------*/
	else
	{// packet I'm forwarding

		/*if ((iph.GetTtl()-1) == 0)
		{
      p=0;
			//drop(p, DROP_RTR_TTL);
			return false;
		}*/

		if((dbrh.GetMode() == DBRH_DATA_RECOVER) &&
			(dbrh.GetOwner() == GetNetDevice()->GetAddress()))
		{
			//fprintf(stderr, "got the pkt I've sent\n");
      p=0;
			//drop(p, DROP_RTR_ROUTE_LOOP);
			// it seems this neighbor couldn't find a greedy node
			//m_nTab->updateRouteFlag(dbrh->prev_hop(), 0);
			return false;
		}
	}

  NS_LOG_DEBUG("AquaSimDBR::Recv2: owner:" << dbrh.GetOwner() << ", prev-hop:"
    << dbrh.GetPrevHop() << ", cur:" << GetNetDevice()->GetAddress());

	// it's time to forward the pkt now
	//p->AddHeader(iph);
	p->AddHeader(dbrh);
  p->AddHeader(ash);
	ForwardPacket(p);
  return true;
}

void AquaSimDBR::DoDispose()
{
	NS_LOG_FUNCTION(this);
	m_rand=0;
	delete m_sendTimer;
	delete m_beaconTimer;

	AquaSimRouting::DoDispose();
}

/*
void
AquaSimDBR::Tap(const Ptr<Packet> p)
{
	// add function later
}
*/
