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

//#include ...
#include "aqua-sim-signal-cache.h"
#include "aqua-sim-header.h"

#include <queue>

#include "aqua-sim-phy.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

#include <complex.h>
#include <complex>
//Aqua Sim Signal Cache

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimSignalCache");
NS_OBJECT_ENSURE_REGISTERED(PktSubmissionTimer);


PktSubmissionTimer::PktSubmissionTimer(Ptr<AquaSimSignalCache> sC)
{
  m_sC = sC;
}

PktSubmissionTimer::~PktSubmissionTimer()
{
  NS_LOG_FUNCTION(this);
  m_sC=0;
}

TypeId
PktSubmissionTimer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PktSubmissionTimer")
    .SetParent<Object>()
  ;
  return tid;
}

void
PktSubmissionTimer::Expire(Ptr<IncomingPacket> inPkt)
{
  /*Ptr<IncomingPacket> inPkt = m_waitingList.top().inPkt;
  m_waitingList.pop();
  if(!m_waitingList.empty())
  {
    Simulator::Schedule(m_waitingList.top().endT, &PktSubmissionTimer::Expire, this);
  }
  */

  NS_LOG_DEBUG("Expire. time:" << Simulator::Now().ToDouble(Time::S) << " inPkt:" << inPkt);
  m_sC->SubmitPkt(inPkt);
}

void
PktSubmissionTimer::AddNewSubmission(Ptr<IncomingPacket> inPkt) {
  AquaSimHeader asHeader;
  (inPkt->packet)->PeekHeader(asHeader);
  /*Time transmissionDelay = Seconds(inPkt->packet->GetSize() * 8 *      //Byte conversion/
                           m_sC->m_phy->GetMac()->GetEncodingEff() /
                           m_sC->m_phy->GetMac()->GetBitRate() ); */

  /* Need to calcuate modulation here, aka how long until entire packet is received */
  Time transmissionDelay = m_sC->m_phy->CalcTxTime(asHeader.GetSize());

  NS_LOG_FUNCTION(this << "incomingPkt:" << inPkt << "txtime:" <<
                    asHeader.GetTxTime() << " transmissionDelay:" <<
                    transmissionDelay.ToDouble(Time::S));

  Simulator::Schedule(transmissionDelay,&PktSubmissionTimer::Expire, this, inPkt);

  /*if (m_waitingList.empty() || m_waitingList.top().endT > transmissionDelay)
  {
      Simulator::Schedule(transmissionDelay, &PktSubmissionTimer::Expire, this);
  }
  m_waitingList.push(PktSubmissionUnit(inPkt, transmissionDelay));
  */
}


PktSubmissionUnit::PktSubmissionUnit(Ptr<IncomingPacket> inPkt_, Time endT_)
	: inPkt(inPkt_), endT(endT_)
{
}

NS_OBJECT_ENSURE_REGISTERED(AquaSimSignalCache);

AquaSimSignalCache::AquaSimSignalCache() :
m_pktNum(0), m_totalPS(0.0), m_pktSubTimer(NULL)
{
  NS_LOG_FUNCTION(this);

  m_head = CreateObject<IncomingPacket>(AquaSimPacketStamp::INVALID);
  m_pktSubTimer = new PktSubmissionTimer(this);
  status = AquaSimPacketStamp::INVALID;
}

AquaSimSignalCache::~AquaSimSignalCache()
{
}

TypeId
AquaSimSignalCache::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AquaSimSignalCache")
    .SetParent<Object> ();
  ;
  return tid;
}

void
AquaSimSignalCache::AddNewPacket(Ptr<Packet> p){
  /**
  * any packet error marked before this step means
  * this packet is invalid and will be considered
  * as noise to other packets only.
  */
  // TODO is packet collision even really tested or dealt with in this class???
  AquaSimHeader asHeader;
  p->PeekHeader(asHeader);

  Ptr<IncomingPacket> inPkt = CreateObject<IncomingPacket>(p,
		  asHeader.GetErrorFlag() ? AquaSimPacketStamp::INVALID : AquaSimPacketStamp::RECEPTION);

  NS_LOG_DEBUG("AddNewPacket:" << p << " w/ Error flag:" << asHeader.GetErrorFlag() << " and incomingpkt:" << inPkt);


  m_pktSubTimer->AddNewSubmission(inPkt);

  inPkt->next = m_head->next;
  m_head->next = inPkt;

  m_pktNum++;
  m_totalPS += m_phy->EM()->GetRxPower();
  UpdatePacketStatus();
}


bool
AquaSimSignalCache::DeleteIncomingPacket(Ptr<Packet> p){
  NS_LOG_FUNCTION(this);
  Ptr<IncomingPacket> pre = m_head;
  Ptr<IncomingPacket> ptr = m_head->next;

  while (ptr != nullptr && ptr->packet != p) {
    ptr = ptr->next;
    pre = pre->next;
  }

  if (ptr != nullptr && ptr->packet == p) {
    m_pktNum--;
    m_totalPS -= m_phy->EM()->GetRxPower();
    pre->next = ptr->next;
    ptr = 0;
    return true;
  }
  NS_LOG_DEBUG("DeleteIncomingPacket: ptr:" << ptr << "ptr(packet) == p?" << (ptr->packet != p));
  return false;
}


void
AquaSimSignalCache::SubmitPkt(Ptr<IncomingPacket> inPkt) {
  NS_LOG_FUNCTION(this << inPkt << inPkt->status);

  status = inPkt->status;
  Ptr<Packet> p = inPkt->packet;
  DeleteIncomingPacket(p); //object pointed by inPkt is deleted here
  /**
  * modem has no idea about invalid packets, so release
  * them here
  */
  if (status == AquaSimPacketStamp::INVALID)
  {
    NS_LOG_DEBUG("Packet(" << p << ") dropped");
    p = 0;
  }
  else
    m_phy->SignalCacheCallback(p);
}


Ptr<IncomingPacket>
AquaSimSignalCache::Lookup(Ptr<Packet> p){
  NS_LOG_FUNCTION(this);
  Ptr<IncomingPacket> ptr = m_head->next;

  while (ptr != nullptr && ptr->packet != PeekPointer(p)) {
    ptr = ptr->next;
  }
  return ptr;
}


void
AquaSimSignalCache::InvalidateIncomingPacket(){
  NS_LOG_FUNCTION(this);
  Ptr<IncomingPacket> ptr = m_head->next;

  while (ptr != nullptr) {
    ptr->status = AquaSimPacketStamp::INVALID;
    ptr = ptr->next;
  }
}


AquaSimPacketStamp::PacketStatus
AquaSimSignalCache::Status(Ptr<Packet> p){
  NS_LOG_FUNCTION(this);
  Ptr<IncomingPacket> ptr = Lookup(p);

  return ptr == nullptr ? AquaSimPacketStamp::INVALID : ptr->status;
}


void
AquaSimSignalCache::UpdatePacketStatus(){
  NS_LOG_FUNCTION(this);

  double noise = 0,		//total noise
	 ps = 0;		//power strength
	//,SINR = 0; 		//currently not used

  for (Ptr<IncomingPacket> ptr = m_head->next; ptr != nullptr; ptr = ptr->next) {
    ps = m_phy->EM()->GetRxPower();
    noise = m_totalPS - ps;

    if (ptr->status != AquaSimPacketStamp::RECEPTION)
      continue;

    ptr->status = m_phy->Decodable(noise + m_noise->Noise(), ps) ? AquaSimPacketStamp::RECEPTION : AquaSimPacketStamp::INVALID;
  }
}

void
AquaSimSignalCache::SetNoiseGen(Ptr<AquaSimNoiseGen> noise)
{
  NS_LOG_FUNCTION(this);
  m_noise = noise;
}

double AquaSimSignalCache::GetNoise()
{
  return m_totalPS + m_noise->Noise();
}

void AquaSimSignalCache::DoDispose()
{
  NS_LOG_FUNCTION(this);
//  Ptr<IncomingPacket> m_head;
//  PktSubmissionTimer* m_pktSubTimer;

  Ptr<IncomingPacket> pos = m_head;
  while (m_head != nullptr) {
    m_head = m_head->next;
    pos->packet = 0;
    pos = 0;
    pos = m_head;
  }

  delete m_pktSubTimer;
  m_pktSubTimer = 0;
  m_phy=0;
  m_noise=0;
  Object::DoDispose();
}

/****
 * AquaSimMultiPathSignalCache class
 ****/

AquaSimMultiPathSignalCache::AquaSimMultiPathSignalCache()
{
  NS_LOG_FUNCTION(this);
}

AquaSimMultiPathSignalCache::~AquaSimMultiPathSignalCache()
{
}

TypeId
AquaSimMultiPathSignalCache::GetTypeId()
{
  static TypeId tid = TypeId ("ns3::AquaSimMultiPathSignalCache")
    .SetParent<AquaSimSignalCache> ();
  ;
  return tid;
}

/*
 * Used to gather the multi paths produced between the transmitter and receiver.
 * Multipath produced are restricted by stop_thres based on attentuation.
 * Model was adapted from Acoustic Channel Simulator, P. Qarabaqi and M. Stojanovic, Northeastern University:
 *  http://millitsa.coe.neu.edu/?q=projects
 *
 * NOTE: can assume bottom acoustic speed can range between 1200 m/s (soft sediment) and 1800 m/s (hard floor)
 *
 * INPUT:
 * h: depth of water, h_t: depth of transmitter, h_r: depth of receiver
 * dist: distance between modems, s: acoustic speed (m/s), s_bottom: acoustic speed at sea bottom (m/s)
 * k: spreading factor, freq: frequency (kHz)
 * stop_thres: stop once path arrival strength falls below direct arrival divided by threshold
 *
 * OUTPUT:
 *  vector of various paths associated with MultiPathInfo structure.
 */
std::vector<MultiPathInfo>
AquaSimMultiPathSignalCache::GetPaths(double h, double h_t, double h_r,
                                      double dist, double s, double s_bottom,
                                      int k, double freq, double stop_thres)
{
  NS_LOG_FUNCTION(this);
  std::vector<MultiPathInfo> paths;

  double a = pow(10,Absorption(freq/1000)/10);
  a=pow(a,0.001);

  int nr=0; //direct path
  double originalG, G, A, heff;
  MultiPathInfo path;
  path.theta = atan((h_t-h_r)/dist);
  path.length = sqrt(pow((h_t-h_r),2)+pow(dist,2));
  path.del=path.length/s;
  A=(pow(path.length,k)*pow(a,path.length));
  originalG=G=1/sqrt(A);

  paths.push_back(path);

  std::vector<int> reflections;
  reflections.push_back(0); //start with surface reflection;

  while (abs(G) >= originalG/stop_thres) {
    nr++;

    MultiPathInfo p1,p2;
    int last, first;
    first=reflections.front();
    last=reflections.back();
    p1.b_ref=ReflSum(reflections);
    p1.s_ref=nr-p1.b_ref;
    heff=(1-first)*h_t + first*(h-h_t) + (nr-1)*h + (1-last)*h_r + last*(h-h_r);
    p1.length=sqrt(pow(heff,2)+pow(dist,2));
    p1.theta=atan(heff/dist);
    if (first==1) p1.theta*=-1;
    p1.del=p1.length/s;
    p1.delay=p1.del-paths.front().del;
    A=pow(p1.length,k)*pow(a,p1.length);
    p1.gamma = pow( ReflCoeff(abs(p1.theta),s,s_bottom),p1.b_ref) * pow(-1,p1.s_ref);
    G=std::min(p1.gamma/sqrt(A),G);
    p1.hp=p1.gamma/sqrt( pow((p1.length / paths.front().length),k) * pow(a,(p1.length-paths.front().length)) );
    paths.push_back(p1);

    //flip reflection markers
    for (unsigned int i=0; i<reflections.size();i++) {
      reflections[i] = !reflections[i];
    }

    first=reflections.front();
    last=reflections.back();
    p2.b_ref=ReflSum(reflections);
    p2.s_ref=nr-p2.b_ref;
    heff=(1-first)*h_t + first*(h-h_t) + (nr-1)*h + (1-last)*h_r + last*(h-h_r);
    p2.length=sqrt(pow(heff,2)+pow(dist,2));
    p2.theta=atan(heff/dist);
    if (first==1) p2.theta*=-1;
    p2.del=p2.length/s;
    p2.delay=p2.del-paths.front().del;
    A=pow(p2.length,k)*pow(a,p2.length);
    p2.gamma = pow( ReflCoeff(abs(p2.theta),s,s_bottom),p2.b_ref) * pow(-1,p2.s_ref);
    G=std::min(p2.gamma/sqrt(A),G);
    p2.hp=p2.gamma/sqrt( pow((p2.length / paths.front().length),k) * pow(a,(p2.length-paths.front().length)) );
    paths.push_back(p2);

    reflections.push_back(!reflections.back());
  }

  return paths;
}

int
AquaSimMultiPathSignalCache::ReflSum(std::vector<int> reflections)
{
  int sum=0;
  for(unsigned int i=0;i<reflections.size();i++) {
    sum+=reflections[i];
  }
  return sum;
}


/*
 *  Find the reflection coefficients using theta, acoustic speed (s), and acoustic speed
 *    at bottom (s_bottom).
 */
double
AquaSimMultiPathSignalCache::ReflCoeff(double theta, double s, double s_bottom)
{
  int rho1,rho2,x1,x2,thetac;
  rho1=1000;  // in kg/m3
  rho2=1800;  // in kg/m3

  thetac=(s>s_bottom)?std::real(acos(s/s_bottom)):acos(s/s_bottom);

  if (theta<thetac) {
    if (thetac==0) return -1;
    else {
      double pi = 4 * atan(1.0);
      return std::real(std::exp(sqrt(-1) * pi * (1-theta/thetac)));
    }
  }
  //theta>=thetac
  x1=rho2/s*sin(theta);
  x2=rho1/s_bottom*sqrt(1-pow((s_bottom/s),2)*pow(cos(theta),2));
  return (x1-x2)/(x1+x2);
}

double
AquaSimMultiPathSignalCache::Absorption(double f)
{
  //redundant alas throp equation is hidden within channel model
  return (0.11 * pow(f,2) / (1 + pow(f,2) )
      + 44 * pow(f,2) / (4100 + pow(f,2) )
      + 0.000275 * pow(f,2) + 0.0003 );
}

void
AquaSimMultiPathSignalCache::DoDispose()
{
  NS_LOG_FUNCTION(this);
  AquaSimSignalCache::DoDispose();
}

};  // namespace ns3
