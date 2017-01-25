/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Moshin Raza Jafri <mohsin.jafri@unive.it>
 */


#include "aqua-sim-routing-ddbr.h"
#include "aqua-sim-address.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-header-mac.h"
#include "aqua-sim-mac.h"
#include "aqua-sim-routing-vbf.h"
#include "aqua-sim-routing-dbr.h"
#include "aqua-sim-header.h"
#include "aqua-sim-datastructure.h"
#include "aqua-sim-pt-tag.h"
#include "aqua-sim-routing-buffer.h"

#include "ns3/vector.h"
#include "ns3/ipv4-header.h"
#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/timer.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/double.h"

// #include "underwatersensor/uw_common/uw_hash_table.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimDDBR");
NS_OBJECT_ENSURE_REGISTERED(AquaSimDDBR);

// parameters to control the delay
#define DBR_MAX_DELAY 1 //12(marin,mohsin) maximal propagation delay for one hop//0.2/mohsin
#define DBR_MAX_RANGE 100 // maximal transmmition range //100
#define DBR_MIN_BACKOFF 0.0 // minimal backoff time for the packet
#define DBR_DELTA 100 // minimal backoff time for the packet//18,38,74(for 75)

//#define USE_FLOODING_ALG  // test for pure flooding protocol
#define DBR_USE_ROUTEFLAG
#define DBR_MAX_HOPS  3
#define DBR_DEPTH_THRESHOLD 2.0// 20
#define DBR_SCALE 1.0

AquaSimDDBR::AquaSimDDBR()
{
  // Initialize variables
  m_pkCount = 0;
  cum_time = 0;
  new_pkt = 0;
  tot_pkt = 0;
  m_pc= new ASSPktCache();
  m_nTab = new MNeighbTable();
  m_sendTimer = new DDBR_SendingTimer(this);
  m_sendTimer->SetFunction(&DDBR_SendingTimer::Expire,m_sendTimer);
}

AquaSimDDBR::~AquaSimDDBR()
{
  delete m_nTab;
  delete m_pc;
}

TypeId
AquaSimDDBR::GetTypeId()
{
  static TypeId tid = TypeId ("ns3::AquaSimDDBR")
    .SetParent<AquaSimRouting> ()
    .AddConstructor<AquaSimDDBR> ()
  ;
  return tid;
}

bool
AquaSimDDBR::Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION(this << packet << GetNetDevice()->GetAddress());
  VBHeader vbh;
  AquaSimHeader ash;
  //Ipv4Header iph; //not used, removed to reduce overhead.
// if (packet->GetUid() > 0){
  // packet->PeekHeader(vbh);
// }
  //vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetForwardAddr()));
  //double expected_send_time = Simulator::Now().ToDouble(Time::S);
  Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
  //packet->PeekHeader(ash);
  //if (!ash) std::cout << "HELLO\n";
  if (packet->GetSize() == 32)  //no headers
  {
    // Time txdelay = (Simulator::Now()/10000000);
    //double txdelay = Simulator::Now().ToDouble(Time::S);
    vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    vbh.SetPkNum(packet->GetUid());
    vbh.SetMessType(AS_DATA);
    vbh.SetTargetAddr(AquaSimAddress::ConvertFrom(dest));
    //vbh.SetDepth(model->GetPosition().z);
    vbh.SetTs(Simulator::Now().ToDouble(Time::S));
    // NS_LOG_DEBUG("(Source tx):Depth in Recv " << model->GetPosition().z << " by node::" << GetNetDevice()->GetAddress()<< " from node :: " << addresss << " sendtime of previous is : "<< ash.GetTimeStamp()<<" packet ID: "<<packet->GetUid()<<" saved val:"<<txdelay);
    ash.SetTimeStamp(Simulator::Now());
    ash.SetDirection(AquaSimHeader::DOWN);
    ash.SetNextHop(AquaSimAddress::GetBroadcast());
    ash.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    ash.SetDAddr(AquaSimAddress::ConvertFrom(dest));
    ash.SetErrorFlag(false);
    ash.SetNumForwards(model->GetPosition().z);
    // ash.SetNumForwards(ash.GetNumForwards() + 1);
    // ash.Setsenderdepth(model->GetPosition().z);
    // ash.SetUId(packet->GetUid());
    ash.SetUId(ash.GetUId() + 1);
    //iph.SetTtl(128);
  }
  else
  {
    packet->RemoveHeader(ash);
    packet->RemoveHeader(vbh);
    // NS_LOG_DEBUG("c_dep: " << model->GetPosition().z << " c_node: " << GetNetDevice()->GetAddress());
    ash.SetNumForwards(model->GetPosition().z);
    // ash.SetNumForwards(ash.GetNumForwards() + 1);
    // ash.Setsenderdepth(model->GetPosition().z);
    ash.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    // ash.SetTimeStamp(ash.GetTimeStamp());
    //vbh.SetDepth(model->GetPosition().z);
    vbh.SetTs(Simulator::Now().ToDouble(Time::S));
    vbh.SetMessType(AS_DATA);
    vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    vbh.SetPkNum(packet->GetUid());
    vbh.SetTargetAddr(AquaSimAddress::ConvertFrom(dest));
    // NS_LOG_DEBUG("vbf ts"<< vbh.GetTs());
    // ash.SetUId(packet->GetUid());
    ash.SetUId(ash.GetUId() + 1);
    //iph.SetTtl(128);
    // NS_LOG_DEBUG("Changed sender address is  ::" << ash.GetSAddr()<<  "  and depth of forwarder is ::"<< ash.Getsenderdepth());
  }

  packet->AddHeader(vbh);
  packet->AddHeader(ash);

  AquaSimAddress addresss=ash.GetSAddr();
  AquaSimAddress src = ash.GetSAddr();
  double dep = ash.GetNumForwards();
  double delta = fabs(model->GetPosition().z - dep);
  NS_LOG_DEBUG("Sdr:pkt id:" <<packet->GetUid()<<" c_dep:"<<model->GetPosition().z<<" p_dep:"<<dep<< " prv_sndtime:"<< ash.GetTimeStamp()<<" id:"<<AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress())<<" ash.SAddr:"<<ash.GetSAddr()<<" c_ID:"<<GetNetDevice()->GetAddress()<<" vbh_id:"<< vbh.GetSenderAddr()<<" x:"<<model->GetPosition().x<<" y:"<<model->GetPosition().y);

  // // // ///////////RD-based on energy////
  // double stab_factor = 1;

  // double alpha2 = ( (GetNetDevice()->EnergyModel()->GetEnergy()) / (GetNetDevice()->EnergyModel()->GetInitialEnergy()) );
  // double halfdelta1 = 1; ///
  // double halfdelta = halfdelta1 - alpha2; ///
  // float power = 2.0;
  // float square = pow(halfdelta,power);
  // float thetaconst = 1.27;
  // float rand_tan = thetaconst * (atan(alpha2));
  // double r_factor = rand_tan * square;

  // double speed = 1500;
  // double tau = DBR_MAX_RANGE / speed;
  // double tau_const = 2;
  // double alpha1 = tau_const * tau;
  // double alpha = fabs(DBR_MAX_RANGE - delta);
  // double dep_diff_f = (alpha * alpha1) / DBR_DELTA;
  // float rand_cot = (tau_const - rand_tan);
  // double d_factor =  rand_cot * dep_diff_f;

  // double ldelays = r_factor + d_factor + stab_factor;


if ((GetNetDevice()->EnergyModel()->GetEnergy()) < 1){
NS_LOG_DEBUG("Expired node depth :"<<model->GetPosition().z <<" id: "<<ash.GetUId()<< "at T:"<<(Simulator::Now().ToDouble(Time::S)));
}


  ///DBR-based holding time computation///*****
  double speed = 1500;
  double tau = DBR_MAX_RANGE / speed;
  double tau_const = 2;
  double alpha1 = tau_const * tau;
  double alpha = fabs(DBR_MAX_RANGE - delta);
  double ldelays = ((alpha * alpha1) / DBR_DELTA); ///if delta = R/2

  if (model->GetPosition().z > 495) {//for adding packet to sink
    if (new_pkt < packet->GetUid()){
        tot_pkt = tot_pkt + 1;
        NS_LOG_DEBUG("sink: packet ID:"<<packet->GetUid()<<" tot_pkt:"<<tot_pkt<<" new_pkt: "<<new_pkt<<" c_ID:"<<GetNetDevice()->GetAddress());
        new_pkt = packet->GetUid();
    }
  }
  // Packet Hash Table is used to keep info about experienced pkts.
  vbf_neighborhood *hashPtr= PktTable.GetHash(vbh.GetSenderAddr(), packet->GetUid());
  double delays = 1;
  // m_pq.print();

  // if ((model->GetPosition().z != 10) && (model->GetPosition().z > dep)) {
  //   NS_LOG_DEBUG("we are not considering due to depth : current nodes's depth: "<<model->GetPosition().z<< ": previous node's depth :"<<dep);
  //   packet=0;
  //   // delete m_pc;
  //   return false;
  // }
  NS_LOG_DEBUG(" vbh-mess:"<<vbh.GetMessType()<<" vbh-data:"<<vbh.GetDataType());

  // // Received this packet before ?
  if ((hashPtr != NULL)){
    NS_LOG_DEBUG("Already in list, rcvd before");
    if (model->GetPosition().z < dep)
      {
        m_pq.print();
        MNeighbEnt *ne;
        ne = new MNeighbEnt();
        Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
        NS_LOG_DEBUG("AquaSimDBR::BeaconIn: " << GetNetDevice()->GetAddress() <<" got beacon from " << src<<" x:"<<model->GetPosition().x<<" y:"<<model->GetPosition().y<<" z:"<<model->GetPosition().z);
        ne->m_location.x = model->GetPosition().x;
        ne->m_location.y = model->GetPosition().z;
        ne->m_location.z = dep;
        ne->m_netID = src;
        m_nTab->EntAdd(ne);
        delete ne;

        if (m_pq.purge(packet))
            {
              NS_LOG_DEBUG("Already in list, but purging queued, as incoming is from low depth also, _p_dep" << dep << " c_dep: " << model->GetPosition().z);
              packet=0;
              m_pq.print();
                // if (m_pq.empty())
                //   {
                //     packet=0;
                //     return false;
                //   }
                //   else
                //   {
                //     double expected_send_time = Simulator::Now().ToDouble(Time::S) + ldelays;
                //     int pck = packet->GetUid();
                //     QqueueItem *q = new QqueueItem(packet, expected_send_time, pck);
                //     m_pq.insert(q);
                //     m_latest = expected_send_time;
                //     m_sendTimer->Schedule(Seconds(ldelays));
                //     return false;
                //   }
              return false;
            }
        }
      else if (model->GetPosition().z > dep) {
            double expected_send_time = Simulator::Now().ToDouble(Time::S) + ldelays;
            int pck = packet->GetUid();
            NS_LOG_DEBUG("Already in list, comparing current_rcvd_pckt_depth and provious one, to dcde abt purging queued, incoming is from high depth also");
        if ( (m_pq.purgenow(packet, expected_send_time)) && ((model->GetPosition().z)!=10) )
           {
             if (m_pq.empty())
              {
                QqueueItem *q = new QqueueItem(packet, expected_send_time, pck);
                m_pq.insert(q);
                m_pq.print();
                m_latest = expected_send_time;
                NS_LOG_DEBUG("Already in list, but sending at once_empty");
                m_sendTimer->Schedule(Seconds(ldelays));
                return true;
              }
                else
              {
                if (m_pq.update(packet, expected_send_time, pck))
                {
                  QqueueItem *q = new QqueueItem(packet, expected_send_time, pck);
                  m_pq.insert(q);
                  NS_LOG_DEBUG("Already in list, but sending at once_not empty");
                  m_pq.print();
                // if ((exsend_time > m_latest))
                //     {
                  m_latest = expected_send_time;
                  // NS_LOG_DEBUG(".In update:....still going..m_latest: "<<m_latest);
                  m_sendTimer->Schedule(Seconds(ldelays));
                  return true;
                    // }
                }
              }
            }
      else {
            NS_LOG_DEBUG("Already in list, but not purging queued, as incoming is from low depth also, _p_dep" << dep << " c_dep: " << model->GetPosition().z);
            packet=0;
            return false;
            }
          }
  }
  PktTable.PutInHash(vbh.GetSenderAddr(), vbh.GetPkNum());

  // if ( (model->GetPosition().z != 10) && ((ash.GetUId() > 30) || (delta > DBR_MAX_RANGE) || (delta < DBR_DEPTH_THRESHOLD)) ) {
  if ( (model->GetPosition().z != 10) && (delta < DBR_DEPTH_THRESHOLD) ) {
    NS_LOG_DEBUG("we are not considering due to UId, dth, range, depth: "<<model->GetPosition().z <<" id: "<<ash.GetUId()<<" delta:"<<delta);
    if (m_pq.purge(packet))
      {
        NS_LOG_DEBUG("Already in list, but purging queued, as incoming is from high depth also, _p_dep" << dep << " c_dep: " << model->GetPosition().z);
        packet=0;
        return false;
      }
    packet=0;
    return false;
  }

  if ( (packet->GetUid()) > 50 ) {
      // delete m_pc;
      PktTable.Reset();
      NS_LOG_DEBUG("Removing corrupted packet");
      packet=0;
      return false;
  }
  if (model->GetPosition().z > 495) {
    double c_time = Simulator::Now().ToDouble(Time::S);
    float diff_time = fabs(ash.GetTimeStamp().ToDouble(Time::S) - c_time);
    cum_time = cum_time + fabs(diff_time);
    double avg_delay= (cum_time / tot_pkt );
    // double avg_delay= (cum_time / (tot_pkt + delays )) + delays;previous
    NS_LOG_DEBUG("sink:c_time:"<< c_time<<" c_delay_pkt:" << diff_time << " sendtime: "
      << ash.GetTimeStamp()<<" pck_ID:"<<packet->GetUid()
      <<" t_delay:"<<cum_time<<" avg_delay:"<<avg_delay);
    ConsiderNew(packet);
    return true;
  }
  if (model->GetPosition().z < dep) {
    NS_LOG_DEBUG("(NEW pckt)we are not considering due to depth");//current nodes's depth: "<< model->GetPosition().z << ": previous node's depth :"<<dep);
    packet = 0;
    // delete m_pc;
    return false;
  }
  else if (model->GetPosition().z > dep) {
      // m_pq.print();
      NS_LOG_DEBUG("(NEW pckt)In section of purging");
      if (m_pq.purge(packet))
      {
        NS_LOG_DEBUG("purging .. already sent by low depth node. ");
        packet=0;
        m_pq.print();
        return false;
      }
      NS_LOG_DEBUG("(NEW pckt)Not purging");
  }
  m_pq.print();

  vbh.SetTs(Simulator::Now().ToDouble(Time::S));
  if ((model->GetPosition().z)==10)
  {
        // ConsiderNew(packet);
     double expected_send_time = Simulator::Now().ToDouble(Time::S) + delays;
     int pck = packet->GetUid();
    //       NS_LOG_DEBUG("Energy: Node=" << m_device->GetAddress() <<
    //  ": remaining energy=" << GetNetDevice()->EnergyModel()->GetEnergy() <<
    // ", initial energy=" << GetNetDevice()->EnergyModel()->GetInitialEnergy());
   // if (m_pq.empty())
   //  {
      QqueueItem *q = new QqueueItem(packet, expected_send_time, pck);
      m_pq.insert(q);
      // m_pq.print();
      NS_LOG_DEBUG("....still going.10...");
      m_latest = expected_send_time;
      m_sendTimer->Schedule(Seconds(delays));
    // }
  }

  // NS_LOG_DEBUG("ht: "<<ldelays<<" square:"<<square <<" rand_tan:"<< rand_tan<<" d_factor: "<<d_factor<<" r_factor:"<<r_factor<<" alpha1:"<<alpha1<<
  //             " r_energy=" << GetNetDevice()->EnergyModel()->GetEnergy() <<" rand_cot:"<<rand_cot<<" depth:"<<model->GetPosition().z<<" dep_diff_f:"<<dep_diff_f<<
  //             " delta:"<<delta);

  NS_LOG_DEBUG("computed ht is :"<<ldelays<<" delta: "<<delta<<" alpha: "<<alpha<<" alpha1: "<<alpha1);
  // NS_LOG_DEBUG("computed ht is :"<<ldelays<<" alpha: "<<alpha<<" alpha1: "<<alpha1<<" remaining energy=" << GetNetDevice()->EnergyModel()->GetEnergy() <<" initial energy=" << GetNetDevice()->EnergyModel()->GetInitialEnergy());
  // NS_LOG_DEBUG("ht is:"<<ldelays<<" alpha: "<<alpha<<" delta:"<<delta<<" d_factor: "<<d_factor<<" r_factor:"<<r_factor<<" remaining energy=" << GetNetDevice()->EnergyModel()->GetEnergy() <<" initial energy=" << GetNetDevice()->EnergyModel()->GetInitialEnergy());
  // NS_LOG_DEBUG("ht is:"<<ldelays<<" ran: "<<ran<<" delta:"<<delta<<" eedelays: "<<eedelays<<" logval:"<<logval<<" remaining energy=" << GetNetDevice()->EnergyModel()->GetEnergy() <<" initial energy=" << GetNetDevice()->EnergyModel()->GetInitialEnergy());
  // if (m_pc->AccessPacket(packet->GetUid()))
  // {
  //   packet=0;
  //   return false;
  // }
  // else
  m_pc->AddPacket(packet->GetUid());
  double exsend_time = Simulator::Now().ToDouble(Time::S);
  if ((model->GetPosition().z) != 10)
  {
    double expected_send_time = Simulator::Now().ToDouble(Time::S) + ldelays;
    int pck = packet->GetUid();
    NS_LOG_DEBUG("....still going..in non--source");
    if (m_pq.empty())
    {
      QqueueItem *q = new QqueueItem(packet, expected_send_time, pck);
      m_pq.insert(q);
      m_pq.print();
      m_latest = expected_send_time;
      NS_LOG_DEBUG("....still going..m_latest: "<<m_latest);
      m_sendTimer->Schedule(Seconds(ldelays));
    }
      else
    {
      if (m_pq.update(packet, expected_send_time, pck))
      {
        // delete m_pc;
        // PktTable.Reset();
        NS_LOG_DEBUG("in 1000...In update:m_latest: "<<m_latest<< "expected_send_time:"<<expected_send_time);
        QqueueItem *q = new QqueueItem(packet, expected_send_time, pck);
        m_pq.insert(q);
        // NS_LOG_DEBUG(".In update:....still going..m_latest: "<<m_latest);
        m_pq.print();
      if ((exsend_time > m_latest))
          {
            m_latest = expected_send_time;
            NS_LOG_DEBUG(".In update:....still going....m_latest: "<<m_latest);
            m_sendTimer->Schedule(Seconds(ldelays));
          }
      }
    }
  }
    // ConsiderNew(packet);
    // NS_LOG_DEBUG("we are considering new packet by node::  " << GetNetDevice()->GetAddress() << " from node :: " << addresss << " depth of previous is : "<< vbh.GetDepth()<< "  and number of forwardings are ::"<< ash.GetNumForwards() << "  and depth of forwarder is ::"<< ash.Getsenderdepth());
    return true;
}

void
AquaSimDDBR::ConsiderNew(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this << pkt);
  VBHeader vbh;
  AquaSimHeader ash;
  pkt->RemoveHeader(ash);
  pkt->PeekHeader(vbh);
  pkt->AddHeader(ash);
  // // pkt->AddPacketTag(ptag);
  AquaSimAddress nodeAddr; //, forward_nodeID, target_nodeID; //not used...
  Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
  // NS_LOG_DEBUG("cons_new section, messtype is "<<vbh.GetMessType());
// vbh.SetMessType(AS_DATA);
  switch (vbh.GetMessType()) {
  case AS_DATA:
    //    printf("uwflooding(%d,%d):it is data packet(%d)! it target id is %d  coordinate is %f,%f,%f and range is %f\n",here_.addr_,here_.port_,vbh->pk_num,vbh->target_id.addr_,vbh->info.tx, vbh->info.ty,vbh->info.tz,vbh->range);
    nodeAddr = vbh.GetSenderAddr();
    if ((AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) == nodeAddr))
    {
      // come from the same node, broadcast it
      NS_LOG_DEBUG("cons_new mac send_source_pkts_prep");
      MACprepare(pkt);
      MACsend(pkt,Seconds(0));
      // NS_LOG_DEBUG("Depth of forwarder is :"<< ash.Getsenderdepth());
      return;
    }
    if ((AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress())==vbh.GetTargetAddr()) && ((model->GetPosition().z) > 495))
    {
      //  printf("uwflooding: %d is the target\n", here_.addr_);
      NS_LOG_DEBUG("cons_new data for sink");
      DataForSink(pkt); // process it
    }
    else if ((AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress())==vbh.GetTargetAddr()) && ((model->GetPosition().z) < 495))
    {
      //  printf("uwflooding: %d is the target\n", here_.addr_);
      NS_LOG_DEBUG("cons_new data for extra sink");
      MACprepare(pkt);
      MACsend(pkt, Seconds(0));
    }
    else{
      // printf("uwflooding: %d is the not  target\n", here_.addr_);
      NS_LOG_DEBUG("cons_mac prepare");
      MACprepare(pkt);
      MACsend(pkt, Seconds(0));
      // NS_LOG_DEBUG("Depth of forwarder is ::"<< ash.Getsenderdepth());
    }
    return;
  default:
    pkt=0;
    NS_LOG_DEBUG("cons_new pkt lost messtype is "<<vbh.GetMessType());
    break;
  }
}

// void
// AquaSimDDBR::Reset()
// {
//   PktTable.Reset();
// }

void
AquaSimDDBR::Terminate()
{
  NS_LOG_DEBUG("Terminate: Node=" << m_device->GetAddress() <<
        ": remaining energy=" << GetNetDevice()->EnergyModel()->GetEnergy() <<
        ", initial energy=" << GetNetDevice()->EnergyModel()->GetInitialEnergy());
}

void
AquaSimDDBR::StopSource()
{
}

Ptr<Packet>
AquaSimDDBR::CreatePacket()
{
  Ptr<Packet> pkt = Create<Packet>();
  if (pkt==NULL) return NULL;
  AquaSimHeader ash;
  VBHeader vbh;
  vbh.SetTs(Simulator::Now().ToDouble(Time::S));
  pkt->AddHeader(vbh);
  pkt->AddHeader(ash);
  return pkt;
}

Ptr<Packet>
AquaSimDDBR::PrepareMessage(unsigned int dtype, AquaSimAddress addr,  int msg_type)
{
  Ptr<Packet> pkt = Create<Packet>();
  VBHeader vbh;
  AquaSimHeader ash;
  vbh.SetMessType(msg_type);
  vbh.SetPkNum(m_pkCount);
  m_pkCount++;
  vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
  vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
  vbh.SetTs(Simulator::Now().ToDouble(Time::S));
  pkt->AddHeader(vbh);
  pkt->AddHeader(ash);
  return pkt;
}

void
AquaSimDDBR::MACprepare(Ptr<Packet> pkt)
{
  VBHeader vbh;
  AquaSimHeader ash;
  // NS_LOG_DEBUG("cons_mac prepare"<<m_nTab);
  pkt->RemoveHeader(ash);
  pkt->RemoveHeader(vbh);
  vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
  ash.SetErrorFlag(false);
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetDirection(AquaSimHeader::DOWN);
  pkt->AddHeader(vbh);
  pkt->AddHeader(ash);
}

MNeighbTable::MNeighbTable(/*AquaSimDBR* a*/)
{
    int i;
    m_numEnts = 0;
    m_maxEnts = 10;
    // create the default table with size 100
    // NS_LOG_DEBUG("in MNeighbTable main");
    m_tab = new MNeighbEnt* [100];
    //m_a = a;
    for (i = 0; i < 10; i++)
        m_tab[i] = new MNeighbEnt();
}

MNeighbTable::~MNeighbTable()
{
    int i;

    for (i = 0; i < m_maxEnts; i++)
        delete m_tab[i];

    delete[] m_tab;
}

TypeId
MNeighbTable::GetTypeId()
{
  static TypeId tid = TypeId("ns3::MNeighbTable")
    ;
  return tid;
}

void MNeighbTable::Dump(void)
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
MNeighbTable::EntDelete(const MNeighbEnt *ne)
{
  int l, r, m;
  int i;
  MNeighbEnt *owslot;

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

#else
MNeighbEnt*
MNeighbTable::EntAdd(const MNeighbEnt *ne)
{
  MNeighbEnt *pe;
  int i, j;
  int l, r, m;

  for (i = 0; i < m_numEnts; i++)
    if (m_tab[i]->m_netID == ne->m_netID)
    {
      m_tab[i]->m_location.x = ne->m_location.x;
      m_tab[i]->m_location.y = ne->m_location.y;
      m_tab[i]->m_location.z = ne->m_location.z;
      // NS_LOG_DEBUG("Replacing in EntAdd x:"<< ne->m_location.x<< " y:"<<ne->m_location.y<< " z:"<<ne->m_location.z<<" m_tab[i]->ID: "<<m_tab[i]->m_netID<<" ne->ID:"<<ne->m_netID);
      return m_tab[i];
    }

  // need we increase the size of table
  if (m_numEnts == m_maxEnts)
  {
    MNeighbEnt **tmp = m_tab;
    m_maxEnts *= 2;     // double the space
    m_tab = new MNeighbEnt* [m_maxEnts];
    bcopy(tmp, m_tab, m_numEnts*sizeof(MNeighbEnt *));

    for (i = m_numEnts; i < m_maxEnts; i++)
      m_tab[i] = new MNeighbEnt();

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
  // NS_LOG_DEBUG("in MNeighbTable EntAdd..i:"<<m_tab[i]);
  return m_tab[i];
}
#endif  // 0

void MNeighbTable::UpdateRouteFlag(AquaSimAddress addr, int val)
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

#ifdef  DBR_USE_ROUTEFLAG
MNeighbEnt *
MNeighbTable::EntFindShadowest(Vector location)
{
  MNeighbEnt *ne = 0;
  int i;
  double t;

  t = location.z;

  NS_LOG_DEBUG("NeighbTable::EntFindShadowest: location=(" <<
      location.x << "," << location.y << "," << location.z <<
      ") has " << m_numEnts << " neighbors, m_numEnts:"<<m_numEnts );

  for (i = 0; i < m_numEnts; i++)
  {
    NS_LOG_DEBUG("NeighbTable::EntFindShadowest: [" << m_tab[i]->m_netID <<
      "] position(" << m_tab[i]->m_location.x << "," <<
      m_tab[i]->m_location.y << "," << m_tab[i]->m_location.z << ")");

    if (m_tab[i]->m_routeFlag == 1)
    {
      ne = m_tab[i];
      NS_LOG_DEBUG("in EntFindShadowest..m_routeFlag");
      return ne;
    }

    if (m_tab[i]->m_location.z > t)
    {
      t = m_tab[i]->m_location.z;
      NS_LOG_DEBUG("in EntFindShadowest..m_tab:"<<m_tab[i]);
      ne = m_tab[i];
    }
  }
  return ne;
}
#else
MNeighbEnt *
MNeighbTable::EntFindShadowest(Vector location)
{
    MNeighbEnt *ne = 0;
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
#endif  // DBR_USE_ROUTEFLAG

void
AquaSimDDBR::DeadNeighb_Callback(MNeighbEnt *ne)
{
  m_nTab->EntDelete(ne);
}

void
AquaSimDDBR::MACsend(Ptr<Packet> pkt, Time delay)
{
  VBHeader vbh;
  AquaSimHeader ash;
  // MNeighbEnt *ne;
  // NS_LOG_DEBUG("cons_mac send");
  Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
  // NS_LOG_DEBUG("in MACsend x:"<< model->GetPosition().x<< " y:"<<model->GetPosition().y<< " z:"<<model->GetPosition().z);
  // ne = m_nTab->EntFindShadowest(model->GetPosition());

  // m_nTab->EntAdd(ne);
  // ne = new MNeighbEnt();
  pkt->RemoveHeader(ash);
  pkt->RemoveHeader(vbh);
  if (vbh.GetMessType() == AS_DATA)
    ash.SetSize(64); //(God::instance()->data_pkt_size);
  else
    ash.SetSize(36);
  NS_LOG_DEBUG("vbh.GetMessType() == AS_DATA"<<ash.GetSize());
  pkt->AddHeader(vbh);
  pkt->AddHeader(ash);
  m_pq.print();
  NS_LOG_DEBUG("..the node depth is :"<<ash.GetNumForwards()<<" ash.GetSAddr():"<<ash.GetSAddr());
  NS_LOG_DEBUG(" vbh-mess:"<<vbh.GetMessType()<<" vbh-data:"<<vbh.GetDataType()<<" vbhp:"<<vbh.GetPkNum());
  NS_LOG_DEBUG("Sdr:pkt id:" <<pkt->GetUid()<<" c_dep:"<<model->GetPosition().z<<" id:"<<AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress())<<" ash.SAddr:"<<ash.GetSAddr()<<" c_ID:"<<GetNetDevice()->GetAddress()<<" vbh_id:"<< vbh.GetSenderAddr()<<" x:"<<model->GetPosition().x<<" y:"<<model->GetPosition().y);
  Simulator::Schedule((Seconds(0)), &AquaSimRouting::SendDown,this,
                          pkt,AquaSimAddress::GetBroadcast(),Seconds(0));
}

void
AquaSimDDBR::DataForSink(Ptr<Packet> pkt)
{
  //  printf("DataforSink: the packet is send to demux\n");
  NS_LOG_FUNCTION(this << pkt << "Sending up to dmux.");
  if (!SendUp(pkt))
    NS_LOG_WARN("DataForSink: Something went wrong when passing packet up to dmux.");
}


NS_OBJECT_ENSURE_REGISTERED(ASSPktCache);

ASSPktCache::ASSPktCache()
{
  m_maxSize = 1500;
  m_size = 0;
  // NS_LOG_DEBUG("the size is : "<<m_size);
  m_pCache = new int[1500];

}

ASSPktCache::~ASSPktCache()
{
  delete[] m_pCache;
}

TypeId
ASSPktCache::GetTypeId()
{
  static TypeId tid = TypeId("ns3::ASSPktCache")
    ;
  return tid;
}

int
ASSPktCache::AccessPacket(int p)
{
  int i, j;
  int tmp;

  for (i = 0; i < m_size; i++) {
    if (m_pCache[i] == p) {
      // if the pkt is existing put it to the tail
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
ASSPktCache::AddPacket(int packet)
{
  if (m_size == m_maxSize) {
    NS_LOG_WARN("Cache is full!");
    // NS_LOG_DEBUG("Cache is full!");
    return;
  }
  // NS_LOG_DEBUG("the size is : "<<m_size<<" [" << m_size << "]: " << sizeof(m_pCache));
  m_pCache[m_size] = packet;
  m_size++;
  return;
}

void
ASSPktCache::DeletePacket(int packet)
{
}


void MMyPacketQueue::insert(QqueueItem *q)
{
  QqueueItem *tmp;
  std::deque<QqueueItem*>::iterator iter;
  // find the insert point
  iter = m_dq.begin();
  NS_LOG_DEBUG("inserting...!");
  while (iter != m_dq.end() )
  {
    tmp = *iter;
    NS_LOG_DEBUG("in inserting, tmp->m_sendTime:" << tmp->m_sendTime << " :q->m_sendTime: "<< q->m_sendTime << "stored id: "<< (*iter)->pcket);
    if (tmp->m_sendTime > q->m_sendTime)
    {
      m_dq.insert(iter, q);
      NS_LOG_DEBUG("MMyPacketQueue::dump: time  is " << ((*iter)->m_sendTime) );
      return;
    }
    iter++;
  }
  // insert at the end of the queue
  m_dq.push_back(q);
}

void MMyPacketQueue::inserting(QqueueItem *q)
{
  // QqueueItem *tmp;
  AquaSimHeader ash;
  Ptr<Packet> p;
  std::deque<QqueueItem*>::iterator iter;
  // tmp -> p = 0;
  iter = m_dq.begin();
  // while (iter != m_dq.end() )
  // {
  //   tmp = *iter;
    // NS_LOG_DEBUG("in inserting, tmp->m_sendTime:" << tmp->m_sendTime << " :q->m_sendTime: "<< q->m_sendTime << "stored id: "<< (*iter)->pcket);
  //   // iter++;
  // }
  m_dq.push_back(q);
}

// Check if packet p in queue needs to be updated.
// If packet is not found, or previous sending time
// is larger than current one, return true.
// Otherwise return false.
bool
MMyPacketQueue::update(Ptr<Packet> p, double t, int pckett)
{
  NS_LOG_DEBUG("....we update the queues");
  std::deque<QqueueItem*>::iterator iter;
  AquaSimHeader ash;
  p->PeekHeader(ash);
  int curID = p->GetUid();
  // NS_LOG_DEBUG("....we update the queues, p->GetUid(): "<<p->GetUid()<<" ash.GetNumForwards(): "<<ash.GetNumForwards());
  int depthval = ash.GetNumForwards();
  int itr = 1;
  double d_time = Simulator::Now().ToDouble(Time::S);
  // NS_LOG_DEBUG("....we update the queues");
  iter = m_dq.begin();
  while (iter != m_dq.end())
  {
    NS_LOG_DEBUG(".in iter: "<<itr<<".Queues:stored ID:"<<(*iter)->pcket<<" curID:"<< curID <<" sendtime stored:"<<(*iter)->m_sendTime<<" t-val:"<<t<<" depthval:"<<depthval);
    if ((depthval == 10) && ((*iter)->pcket >= curID) && (((*iter)->pcket) == curID))
    {
      // pkt = 0;
      NS_LOG_DEBUG("....we donot update the queues. in 1000.....");
      return false;
      break;
    }
    if ((depthval == 10) && ((*iter)->pcket > curID))
     {
       NS_LOG_DEBUG("....we update the queues..in 1000....");
       return true;
    }
    if (depthval != 10)// && ((*iter)->pcket > curID))
    {
       NS_LOG_DEBUG("waiting packet time expired.OR not present..low curID");
        if ((((*iter)->m_p) == 0) || (((*iter)->m_sendTime) < d_time))
        {
           NS_LOG_DEBUG("waiting packet time expired.OR not present"<< d_time<< " (*iter)->m_sendTime: "<<(*iter)->m_sendTime);
           m_dq.erase(iter);
           return true;
         }
        else
         return true;
      }
    iter++;
    itr++;
  }
  return true;
}

// Find the item in queue which has the same packet ID
// as p, and remove it.
// If such a item is found, return true, otherwise
// return false.
bool
MMyPacketQueue::purge(Ptr<Packet> p)
{
  int curID;
  std::deque<QqueueItem*>::iterator iter;
  curID = p->GetUid();
  iter = m_dq.begin();
  while (iter != m_dq.end())
  {
    NS_LOG_DEBUG("Searching, curID: "<<curID <<" sch_send_time:"<<(*iter)->m_sendTime << " ,stored id: "<< (*iter)->pcket);
    if (((*iter)->pcket == curID))/// || (curID >= 10) )//|| ((*iter)->pcket >= 10))
    {
      NS_LOG_DEBUG("Removing pck_cur_id: "<<curID <<" due to stored id: "<<(*iter)->pcket);
      (*iter)->m_p = 0;
      return true;
    }
    iter++;
  }
  return false;
}

bool
MMyPacketQueue::purgenow(Ptr<Packet> p, double t)
{
  std::deque<QqueueItem*>::iterator iter;
  int curID = p->GetUid();
  iter = m_dq.begin();
  while (iter != m_dq.end())
  {
    if ( ((*iter)->pcket == curID) && (((*iter)->m_sendTime) > t) )
    {
      NS_LOG_DEBUG("Removing c_id "<<curID <<" with s_time: "<<(*iter)->m_sendTime<<" due to iter id: "<<(*iter)->pcket<<" with new_time: "<<t);
      m_dq.erase(iter);
      // (*iter)->m_p = 0;
      return true;
    }
    iter++;
  }
  return false;
}

void MMyPacketQueue::print()
{
  std::deque<QqueueItem*>::iterator iter;
  // AquaSimHeader ash;
  int i = 0;
  iter = m_dq.begin();
  while (iter != m_dq.end())
  {
    // ((*iter)->m_p)->PeekHeader(ash);
    NS_LOG_DEBUG("MMyPacketQueue::dump: time is " << (*iter)->m_sendTime << " and stored id: "<< (*iter)->pcket);
    iter++;
    i++;
  }
}

// Dump all the items in queue for debug
void MMyPacketQueue::dump()
{
  std::deque<QqueueItem*>::iterator iter;
  DBRHeader dbrh;
  int i = 0;
  iter = m_dq.begin();
  while (iter != m_dq.end())
  {
    ((*iter)->m_p)->PeekHeader(dbrh);
    NS_LOG_INFO("MMyPacketQueue::dump:[" << i << "] packetID " <<
    dbrh.GetPacketID() << ", send time " << (*iter)->m_sendTime);
    iter++;
    i++;
  }
}

void MMyPacketQueue::printall()
{
  // QqueueItem *q;
  std::deque<QqueueItem*>::iterator iter;
  AquaSimHeader ash;
  int i = 0;
  double expected_time = Simulator::Now().ToDouble(Time::S);
  iter = m_dq.begin();
  while (iter != m_dq.end())
  {
    ((*iter)->m_p)->PeekHeader(ash);
    if ((ash.GetUId() != 97) && (ash.GetUId() == (*iter)->pcket) && ((*iter)->m_sendTime > expected_time))/// || (curID >= 10) )//|| ((*iter)->pcket >= 10))
    {
      break;
      return;
    }
    iter++;
    i++;
  }
}

// NS_OBJECT_ENSURE_REGISTERED(DDBR_SendingTimer);
void DDBR_SendingTimer::Expire()
{
  m_a->Send_Callback();
}

void
AquaSimDDBR::Send_Callback(void)
{
  QqueueItem *q;
  AquaSimHeader ash;
  // NS_LOG_DEBUG("we are in callback...");
  // m_pq.print();
  // we're done if there is no packet in queue
  if (m_pq.empty())
    return;
  // send the first packet out
  q = m_pq.front();
  // g = m_pq.front();
  m_pq.pop();
  if (q->m_p == 0){
      // NS_LOG_DEBUG("the packet has lost");
      return;
  }
  Simulator::Schedule(Seconds(0),&AquaSimDDBR::ConsiderNew,this,q->m_p);
  // m_pq.insert(g);
  m_pq.print();
  // NS_LOG_DEBUG("we are in double callback...");
  // put the packet into cache

  // (q->m_p)->PeekHeader(ash);
  // m_pc->AddPacket(ash.GetUId());

  if (!m_pq.empty())
  {
    q = m_pq.front();
      m_pq.pop();
  if (q->m_p == 0){
      return;
    }
      m_latest = q->m_sendTime;
      m_sendTimer->Schedule(Seconds(m_latest - Simulator::Now().ToDouble(Time::S)));
  }
}

#if 1
bool
AquaSimDDBR::Recv1(Ptr<Packet> p, const Address &dest, uint16_t protocolNumber)
{
  AquaSimHeader ash;
  DBRHeader dbrh;
  //Ipv4Header iph;
  AquaSimPtTag ptag;
  p->RemoveHeader(ash);
  p->RemoveHeader(dbrh);
  //p->PeekHeader(iph);
  p->RemovePacketTag(ptag);

  //double x, y, z;

  AquaSimAddress src = ash.GetSAddr();
  AquaSimAddress dst = ash.GetDAddr();
  //nsaddr_t src = Address::instance().get_nodeaddr(iph->saddr());
  //nsaddr_t dst = Address::instance().get_nodeaddr(iph->daddr());

  Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();

  if ((src == GetNetDevice()->GetAddress()) &&
      (ash.GetNumForwards() == 0))
  {// packet I'm originating

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
  dbrh.SetDepth(model->GetPosition().z);    // save the depth info

  // broadcasting the pkt
  NS_ASSERT(!ash.GetErrorFlag());
  p->AddHeader(dbrh);
  p->AddHeader(ash);
  //p->AddHeader(iph);
  p->AddPacketTag(ptag);
  Simulator::Schedule(Seconds(0),&AquaSimRouting::SendDown,this,
                        p,ash.GetNextHop(),Seconds(0));
  return true;
  }

  if ((src == GetNetDevice()->GetAddress()) &&
      (dbrh.GetMode() == DBRH_DATA_GREEDY))
  {// Wow, it seems some one is broadcasting the pkt for us,
   // so we need to dismiss the timer for the pkt

    // NS_LOG_DEBUG("AquaSimDDBR::Recv: address:" <<
    //     GetNetDevice()->GetAddress() << ": got the pkt I've sent");

    p=0;
    //drop(p, DROP_RTR_ROUTE_LOOP);
    return false;
  }

  if (dst == GetNetDevice()->GetAddress())
  {// packet is for me
    // NS_LOG_DEBUG("AquaSimDDBR::Recv: address:" <<
    //     GetNetDevice()->GetAddress() << ": packet is delivered!");
    p->AddHeader(dbrh);
    p->AddHeader(ash);
    //p->AddHeader(iph);
    p->AddPacketTag(ptag);
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
AquaSimDDBR::HandlePktForward(Ptr<Packet> p)
{
  p->RemoveHeader(ash);
  p->PeekHeader(dbrh);
  //p->PeekHeader(iph);
  p->RemovePacketTag(ptag);

  //if (--(iph.GetTtl()) == 0)
  //{
//    p=0;
    //drop(p, DROP_RTR_TTL);
//    return;
//  }

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
  ash.SetNumForwards((ash.GetNumForwards()++));
  ash.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
  ash.SetDirection(AquaSimHeader::DOWN);
  //ash->addr_type_ = AF_INET;
  ptag.SetPacketType(AquaSimPtTag::PT_DBR);
  ash.SetSize(dbrh.Size() + IP_HDR_LEN);
  ash.SetNextHop(AquaSimAddress:GetBroadcast());

  // finally broadcasting it!
  NS_ASSERT(!ash.GetErrorFlag());
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
AquaSimDDBR::HandlePktForward(Ptr<Packet> p)
{
  AquaSimHeader ash;
  DBRHeader dbrh;
  //Ipv4Header iph;
  AquaSimPtTag ptag;
  p->RemoveHeader(ash);
  p->RemoveHeader(dbrh);
  //p->PeekHeader(iph);
  p->RemovePacketTag(ptag);

  double delta;
  double delay = 0;

  //double x, y, z;
  Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
  // if (model == 0)
  //   {
  //     NS_LOG_DEBUG("MobilityModel does not exist for device " << GetNetDevice());
  //   }

  //if ((iph.GetTtl()-1) == 0)
  //{
    //p=0;
    //drop(p, DROP_RTR_TTL);
  //  return;
//  }

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
      p->AddHeader(dbrh);
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
  // case DBRH_DATA_RECOVER:
  //   if (dbrh.GetNHops() <= 0)
  //   {
  //     NS_LOG_DEBUG("AquaSimDDBR::Recv: address:" <<
  //         GetNetDevice()->GetAddress() << ": drops pkt! (nhops < 0)");

  //     p=0;
  //     //drop(p, DROP_RTR_TTL);
  //     return;
  //   }
    dbrh.SetNHops((dbrh.GetNHops()-1));
    break;
  default:
    // NS_LOG_WARN("AquaSimDDBR::Recv: Unknown data type!");
    return;
  }

  // make up the DBR header
  dbrh.SetOwner(dbrh.GetPrevHop());
  dbrh.SetPrevHop(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));

  NS_LOG_DEBUG("[" << GetNetDevice()->GetAddress() << "]: delay " <<
    delay << " before broadcasting!");

  if (m_pc == NULL) {
    // NS_LOG_WARN("AquaSimDDBR::Recv: packet cache pointer is null!\n");
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
  //  m_pc->AddPacket(dbrh.GetPacketID());

  // put the packet into sending queue
  double expected_send_time = Simulator::Now().ToDouble(Time::S) + delay;

  p->AddHeader(dbrh);
  p->AddHeader(ash);
  p->AddPacketTag(ptag);
  int pck = p->GetUid();
  QqueueItem *q = new QqueueItem(p, expected_send_time, pck);

  if (m_pq.empty())
  {
    m_pq.insert(q);
    m_latest = expected_send_time;
    m_sendTimer->Schedule(Seconds(delay));
  }
  else
  {
    if (m_pq.update(p, expected_send_time, pck))
    {
      m_pq.insert(q);

      // update the sending timer
      if (expected_send_time < m_latest)
      {
        m_latest = expected_send_time;
        // m_sendTimer->Schedule(Seconds(delay));
      }
    }
  }
}
#endif  // end of USE_FLOODING_ALG
#else


bool
AquaSimDDBR::Recv1(Ptr<Packet> p, const Address &dest, uint16_t protocolNumber)
{
  AquaSimHeader ash;
  DBRHeader dbrh;
  //Ipv4Header iph;
  p->PeekHeader(ash);
  p->PeekHeader(dbrh);
  //p->PeekHeader(iph);

  AquaSimAddress src = ash.GetSAddr();
  AquaSimAddress dst = ash.GetDAddr();
  //nsaddr_t src = Address::instance().get_nodeaddr(iph->saddr());
  //nsaddr_t dst = Address::instance().get_nodeaddr(iph->daddr());

  //NS_LOG_DEBUG("AquaSimDDBR::Recv: address:" << GetNetDevice()->GetAddress() <<
  //  " receives pkt from " << src << " to " << dst);
  Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
  // if (model == 0)
  //   {
  //     NS_LOG_DEBUG("MobilityModel does not exist for device " << GetNetDevice());
  //   }

  // if (dbrh.GetMode() == DBRH_BEACON)
  // {// beacon packet
  //   // self is not one of the neighbors
  //   if (src != GetNetDevice()->GetAddress())
  //     BeaconIn(p);
  //   return true;
  // }
  else if ((src == GetNetDevice()->GetAddress()) &&
    (ash.GetNumForwards() == 0))
  {// packet I'm originating

    NS_LOG_DEBUG("AquaSimDDBR::Recv: " << src << " generates data packet.");

    //edit headers
    p->RemoveHeader(ash);
    p->RemoveHeader(dbrh);
    //p->RemoveHeader(iph);

    ash.SetDirection(AquaSimHeader::DOWN);
    ash.SetSize(ash.GetSize() + 8);
    //iph.SetTtl(128);
    dbrh.SetMode(DBRH_DATA_GREEDY);
    dbrh.SetPacketID(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt());
  }
  else if ((src == GetNetDevice()->GetAddress()) &&
    (dbrh.GetMode() == DBRH_DATA_GREEDY))
  {// duplicate packet, discard

    NS_LOG_DEBUG("AquaSimDDBR::Recv: got the pkt I've sent.");

    p=0;
    //drop(p, DROP_RTR_ROUTE_LOOP);
    return false;
  }
  else if (dst == GetNetDevice()->GetAddress())
  {// packet is for me

    NS_LOG_DEBUG("Packet is delivered!");
    p->AddHeader(ash);
    p->AddHeader(dbrh);
    //p->AddHeader(iph);
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

    //if (--(iph.GetTtl()) == 0)
    //{
//      p=0;
      //drop(p, DROP_RTR_TTL);
    //  return false;
    //}

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

  NS_LOG_DEBUG("AquaSimDDBR::Recv: owner:" << dbrh.GetOwner() << ", prev-hop:"
    << dbrh.GetPrevHop() << ", cur:" << GetNetDevice()->GetAddress());

  // it's time to forward the pkt now
  p->AddHeader(ash);
  p->AddHeader(dbrh);
  //p->AddHeader(iph);
  // ForwardPacket(p);
  return true;
}
#endif




// bool
// AquaSimDDBR::Recv2(Ptr<Packet> p, const Address &dest, uint16_t protocolNumber)
// {
//   AquaSimHeader ash;
//   DBRHeader dbrh;
//   //Ipv4Header iph;
//   p->PeekHeader(ash);
//   p->PeekHeader(dbrh);
//   //p->PeekHeader(iph);

//   AquaSimAddress src = ash.GetSAddr();
//   AquaSimAddress dst = ash.GetDAddr();
//   //nsaddr_t src = Address::instance().get_nodeaddr(iph->saddr());
//   //nsaddr_t dst = Address::instance().get_nodeaddr(iph->daddr());

//   //NS_LOG_DEBUG("AquaSimDDBR::Recv2: address:" << GetNetDevice()->GetAddress()
//   //  << " receives pkt from " << src << " to " << dst);

//   Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
//   // if (model == 0)
//   //   {
//   //     NS_LOG_DEBUG("MobilityModel does not exist for device " << GetNetDevice());
//   //   }

//   // if (dbrh.GetMode() == DBRH_BEACON)
//   // {// beacon packet

//   //   // self is not one of the neighbors
//   //   if (src != GetNetDevice()->GetAddress())
//   //     BeaconIn(p);
//   //   return true;
//   // }
//   if ((src == GetNetDevice()->GetAddress()) &&
//     (ash.GetNumForwards() == 0))
//   {// packet I'm originating

//     // NS_LOG_DEBUG("AquaSimDDBR::Recv2: " << src << " generates data packet.");

//     //edit headers
//     p->RemoveHeader(ash);
//     p->RemoveHeader(dbrh);
//     //p->RemoveHeader(iph);

//     ash.SetSize(ash.GetSize() + 8);
//     ash.SetDirection(AquaSimHeader::DOWN);
//     //iph.SetTtl(128);
//     dbrh.SetMode(DBRH_DATA_GREEDY);
//     dbrh.SetPacketID(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt());
//   }
//   else if ((src == GetNetDevice()->GetAddress()) &&
//     (dbrh.GetMode() == DBRH_DATA_GREEDY))
//   {// duplicate packet, discard

//     // NS_LOG_DEBUG("AquaSimDDBR::Recv2: got the pkt I've sent");

//     p=0;
//     //drop(p, DROP_RTR_ROUTE_LOOP);
//     return false;
//   }
//   else if (dst == GetNetDevice()->GetAddress())
//   {// packet is for me

//     NS_LOG_DEBUG("Packet is delivered!");
//     p->AddHeader(ash);
//     p->AddHeader(dbrh);
//     //p->AddHeader(iph);
//     // we may need to send it to upper layer agent
//     if (!SendUp(p))
//       NS_LOG_WARN("DataForSink: Something went wrong when passing packet up to dmux.");

//     return false;
//   }
// /*------------------------------------------------
//   else if (dst == IP_BROADCAST)
//   {
//     if (dbrh->mode() == DBRH_BEACON)
//     {
//       // self is not one of the neighbors
//       if (src != mn_->address())
//         beaconIn(p);
//       return;
//     }
//   }
// ------------------------------------------------*/
//   else
//   {// packet I'm forwarding

//     //if ((iph.GetTtl()-1) == 0)
//     //{
// //      p=0;
//       //drop(p, DROP_RTR_TTL);
// //      return false;
// //    }

//     if((dbrh.GetMode() == DBRH_DATA_RECOVER) &&
//       (dbrh.GetOwner() == GetNetDevice()->GetAddress()))
//     {
//       //fprintf(stderr, "got the pkt I've sent\n");
//       p=0;
//       //drop(p, DROP_RTR_ROUTE_LOOP);
//       // it seems this neighbor couldn't find a greedy node
//       //m_nTab->updateRouteFlag(dbrh->prev_hop(), 0);
//       return false;
//     }
//   }

//   // NS_LOG_DEBUG("AquaSimDDBR::Recv2: owner:" << dbrh.GetOwner() << ", prev-hop:"
//   //   << dbrh.GetPrevHop() << ", cur:" << GetNetDevice()->GetAddress());

//   // it's time to forward the pkt now
//   p->AddHeader(ash);
//   p->AddHeader(dbrh);
//   //p->AddHeader(iph);
//   // ForwardPacket(p);
//   return true;
// }

/*
void
AquaSimDBR::Tap(const Ptr<Packet> p)
{
  // add function later
}
*/
