//....

//#include ...
#include "aqua-sim-signal-cache.h"
#include "aqua-sim-header.h"

#include <queue>

#include "aqua-sim-phy.h"
#include "ns3/simulator.h"
#include "ns3/log.h"


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
}

TypeId
PktSubmissionTimer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PktSubmissionTimer")
  ;
  return tid;
}

void
PktSubmissionTimer::Expire(void) {
  IncomingPacket* inPkt = m_waitingList.top().inPkt;
  m_waitingList.pop();

  if (!m_waitingList.empty()) {
    Simulator::Schedule( m_waitingList.top().endT - Simulator::Now(), &PktSubmissionTimer::Expire, this);
  }
  m_sC->SubmitPkt(inPkt);
}

void
PktSubmissionTimer::AddNewSubmission(IncomingPacket* inPkt) {
  AquaSimHeader asHeader;
  (inPkt->packet)->PeekHeader(asHeader);
  NS_LOG_DEBUG ("txtime=" << asHeader.GetTxTime());
  Time endT_ = Time(Simulator::Now().GetSeconds() + Seconds(asHeader.GetTxTime()));
  if (m_waitingList.empty() || m_waitingList.top().endT > endT_) {
    Simulator::Schedule( endT_ - Simulator::Now(), &PktSubmissionTimer::AddNewSubmission, this, inPkt );
  }
  m_waitingList.push(PktSubmissionUnit(inPkt, endT_));
}


PktSubmissionUnit::PktSubmissionUnit(IncomingPacket* inPkt_, Time endT_) 
	: inPkt(inPkt_), endT(endT_) 
{
}

NS_OBJECT_ENSURE_REGISTERED(AquaSimSignalCache);

AquaSimSignalCache::AquaSimSignalCache() :
m_pktNum(0), m_totalPS(0.0), m_phy(NULL), m_pktSubTimer(NULL)
{
  m_head = new IncomingPacket(NULL, INVALID);
  m_pktSubTimer = new PktSubmissionTimer(this);
  status = INVALID;
}

AquaSimSignalCache::~AquaSimSignalCache() {
  IncomingPacket* pos = m_head;
  while (m_head != NULL) {
    m_head = m_head->next;
    pos->packet = 0;
    pos = 0;		//this is probably memory leakage.
    pos = m_head;
  }

  m_pktSubTimer = 0;	//Should probably use unref instead...
}

TypeId
AquaSimSignalCache::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AquaSimSignalCache")
	  //.SetParent<Object> ();
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

  AquaSimHeader asHeader;
  p->PeekHeader(asHeader);
  NS_LOG_DEBUG ("errorFlag=" << asHeader.GetErrorFlag());

  IncomingPacket* inPkt = new IncomingPacket(PeekPointer(p),
		  asHeader.GetErrorFlag() ? INVALID : RECEPTION);

  m_pktSubTimer->AddNewSubmission(inPkt);

  inPkt->next = m_head->next;
  m_head->next = inPkt;

  m_pktNum++;
  m_totalPS += m_em->GetRxPower();
  UpdatePacketStatus();
}


bool
AquaSimSignalCache::DeleteIncomingPacket(Ptr<Packet> p){
  IncomingPacket* pre = m_head;
  IncomingPacket* ptr = m_head->next;

  while (ptr != NULL && ptr->packet != p) {
    ptr = ptr->next;
    pre = pre->next;
  }

  if (ptr != NULL && ptr->packet == p) {
    m_pktNum--;
    m_totalPS -= m_em->GetRxPower();
    pre->next = ptr->next;
    ptr = 0;
    return true;
  }
  return false;
}


void
AquaSimSignalCache::SubmitPkt(IncomingPacket* inPkt) {
  status = inPkt->status;
  Ptr<Packet> p = inPkt->packet;
  DeleteIncomingPacket(p); //object pointed by inPkt is deleted here
  /**
  * modem has no idea about invalid packets, so release
  * them here
  */
  if (status == INVALID)
    p = 0;
  else
    m_phy->SignalCacheCallback(p);
}


IncomingPacket*
AquaSimSignalCache::Lookup(Ptr<Packet> p){
  IncomingPacket* ptr = m_head->next;

  while (ptr != NULL && ptr->packet != PeekPointer(p)) {
    ptr = ptr->next;
  }
  return ptr;
}


void
AquaSimSignalCache::InvalidateIncomingPacket(){
  IncomingPacket* ptr = m_head->next;

  while (ptr != NULL) {
    ptr->status = INVALID;
    ptr = ptr->next;
  }
}


PacketStatus 
AquaSimSignalCache::Status(Ptr<Packet> p){
  IncomingPacket* ptr = Lookup(p);

  return ptr == NULL ? INVALID : ptr->status;
}


void
AquaSimSignalCache::UpdatePacketStatus(){
  double noise = 0,		//total noise
	 ps = 0;		//power strength
	//,SINR = 0; 		//currently not used

  for (IncomingPacket* ptr = m_head->next; ptr != NULL; ptr = ptr->next) {
    ps = m_em->GetRxPower();
    noise = m_totalPS - ps;

    if (ptr->status != RECEPTION)
      continue;

    ptr->status = m_phy->Decodable(noise + m_noise->Noise(), ps) ? RECEPTION : INVALID;
  }
}


};  // namespace ns3
