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

#include "aqua-sim-mac.h"
#include "aqua-sim-header.h"

#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/double.h"
#include "ns3/aqua-sim-address.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"

//...

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimMac");
NS_OBJECT_ENSURE_REGISTERED(AquaSimMac);

/*
* Base class for Aqua Sim MAC
* Once child created this needs to be updated ****
*/

TypeId
AquaSimMac::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimMac")
  .SetParent<Object>()
  //.AddConstructor<AquaSimMac>()
  .AddAttribute ("SetNetDevice", "A pointer to connect to the net device.",
    PointerValue (),
    MakePointerAccessor (&AquaSimMac::m_device),
    MakePointerChecker<AquaSimMac> ())
  .AddAttribute ("BitRate", "Bit rate of MAC layer.",
    DoubleValue(1e4),
    MakeDoubleAccessor(&AquaSimMac::m_bitRate),
    MakeDoubleChecker<double>())
  .AddAttribute ("EncodingEfficiency", "Ratio of encoding",
    DoubleValue(1),
    MakeDoubleAccessor(&AquaSimMac::m_encodingEfficiency),
    MakeDoubleChecker<double>())
  /*.AddAttribute ("SetPhy", "A pointer to set the phy layer.",
    PointerValue (),
    MakePointerAccessor (&AquaSimMac::m_phy),
    MakePointerChecker<AquaSimMac> ())
  .AddAttribute ("SetRouting", "A pointer to set the routing layer.",
    PointerValue (),
    MakePointerAccessor (&AquaSimMac::m_rout),
    MakePointerChecker<AquaSimMac> ())*/
  .AddTraceSource ("RoutingTx",
    "Trace source indicating a packet has started transmitting.",
    MakeTraceSourceAccessor (&AquaSimMac::m_macTxTrace),
    "ns3::AquaSimMac::TxCallback")
  .AddTraceSource ("RoutingRx",
    "Trace source indicating a packet has been received.",
    MakeTraceSourceAccessor (&AquaSimMac::m_macRxTrace),
    "ns3::AquaSimMac::RxCallback")
  ;
  return tid;
}

AquaSimMac::AquaSimMac() :
  m_bitRate(1e4)/*10kbps*/, m_encodingEfficiency(1)
{
}

AquaSimMac::~AquaSimMac()
{
}

void
AquaSimMac::SetDevice(Ptr<AquaSimNetDevice> device)
{
  NS_LOG_FUNCTION(this);
  m_device = device;
}
/*
void
AquaSimMac::SetPhy(Ptr<AquaSimPhy> phy){
  NS_LOG_FUNCTION(this << phy);
  m_phy = phy;
}

void
AquaSimMac::SetRouting(Ptr<AquaSimRouting> rout){
  NS_LOG_FUNCTION(this << rout);
  m_rout = rout;
}*/

void
AquaSimMac::SetAddress(AquaSimAddress addr)
{
  NS_LOG_FUNCTION(this << addr);
  m_address = addr;
}


void
AquaSimMac::SetForwardUpCallback(Callback<void, const AquaSimAddress&> upCallback)
{
  //not currently used.
  m_callback = upCallback;
}

bool
AquaSimMac::SendUp(Ptr<Packet> p)
{
  NS_ASSERT(m_device);
  AquaSimHeader ash;
  p->PeekHeader(ash);

  if (Routing()) {
    return Routing()->Recv(p,ash.GetDAddr(),0);
  }

  if (ash.GetDAddr() == AquaSimAddress::ConvertFrom(m_device->GetAddress())) {
    //I am sink, no pass up implemented.
    NS_LOG_INFO("Mac:SendUp : packet at destination node:" << m_device->GetAddress() <<
      ", with end-to-end delay of " << (Simulator::Now()-ash.GetTimeStamp()).ToDouble(Time::S));
    return true;
  }

  /* if no routing layer is currently implemented */

  //Change to DOWN and send packet (mac layer wise).
  p->RemoveHeader(ash);
  ash.SetDirection(AquaSimHeader::DOWN);
  p->AddHeader(ash);

  if (!TxProcess(p)) {
    NS_LOG_DEBUG(this << "Mac recv error");
  }
  return false;
}

bool
AquaSimMac::SendDown(Ptr<Packet> p, TransStatus afterTrans)
{
  NS_ASSERT(m_device);// && m_phy && m_rout);

  /*  For debugging:
  std::cout << "\nMac @SendDown check:\n";
  p->Print(std::cout);
  std::cout << "\n";
  */

  if (m_device->GetTransmissionStatus() == SLEEP) {
    NS_LOG_DEBUG("SendDown::Sleeping, drop pkt");
      return false;
   }

  if (m_device->GetTransmissionStatus() == RECV) {
      NS_LOG_DEBUG("SendDown::Recv, queuing pkt");
      m_sendQueue.push(std::make_pair(p,afterTrans));
      return true;
  }
  else {
      m_device->SetTransmissionStatus(SEND);
      AquaSimHeader ash;
      p->PeekHeader(ash);
      if (ash.GetTxTime().IsNegative()) ash.SetTxTime(GetTxTime(p));
      NS_LOG_DEBUG("Sending packet to Phy : " << ash.GetSize() << " bytes ; " << ash.GetTxTime().GetSeconds() << " sec. ; Dest: " << ash.GetDAddr().GetAsInt() << " ; Src: " << ash.GetSAddr().GetAsInt() << " ; Next H.: " << ash.GetNextHop().GetAsInt());
      Simulator::Schedule(ash.GetTxTime(), &AquaSimNetDevice::SetTransmissionStatus,m_device,afterTrans);
      //slightly awkard but for phy header Buffer
      AquaSimPacketStamp pstamp;
      p->AddHeader(pstamp);
      return Phy()->Recv(p);
  }
}

void
AquaSimMac::HandleIncomingPkt(Ptr<Packet> p) {
  NS_LOG_FUNCTION(this);

  //m_recvChannel->AddNewPacket(p);
  AquaSimHeader asHeader;
  p->RemoveHeader(asHeader);

  Time txTime = asHeader.GetTxTime();
  if (Device()->GetTransmissionStatus() != SEND) {
      m_device->SetCarrierSense(true);
  }
  p->AddHeader(asHeader);

  Simulator::Schedule(txTime, &AquaSimMac::SendUp, this, p);
}

void
AquaSimMac::HandleOutgoingPkt(Ptr<Packet> p) {
  NS_LOG_FUNCTION(this);
  //m_callback = h;
  /*
  *  TODO Handle busy terminal problem before trying to tx packet
  *     NOTE this is done in SendDown()...
  */
  //m_phy->SetPhyStatus(PHY_SEND);

  TxProcess(p);
}

void
AquaSimMac::Recv(Ptr<Packet> p) {
  //assert(initialized());
  NS_LOG_FUNCTION(this);
  NS_ASSERT(m_device);// && m_phy && m_rout);
  AquaSimHeader asHeader;
  p->PeekHeader(asHeader);

  switch (asHeader.GetDirection())
  {
    case (AquaSimHeader::DOWN):
      // Handle outgoing packets.
      HandleOutgoingPkt(p);
      return;
    case (AquaSimHeader::NONE):
      NS_LOG_WARN(this << "No direction set for packet(" << p << "), dropping");
      return;
    case (AquaSimHeader::UP):
      // Handle incoming packets.
      HandleIncomingPkt(p);
      return;
  }
  NS_LOG_DEBUG("Something went very wrong in mac");
}

void
AquaSimMac::PowerOn()
{
  NS_LOG_FUNCTION(this);
  Phy()->PowerOn();
}

double
AquaSimMac::GetPreamble()
{
  return Phy()->Preamble();
}

void
AquaSimMac::PowerOff()
{
  NS_LOG_FUNCTION(this);
  Phy()->PowerOff();
}

/**
* @param pkt_len length of packet, in byte
* @param mod_name	modulation name
*
* @return txtime of a packet of size pkt_len using the modulation scheme
* 		specified by mod_name
*/

Time
AquaSimMac::GetTxTime(int pktLen, std::string * modName)
{
  return Phy()->CalcTxTime(pktLen, modName);
}

Time
AquaSimMac::GetTxTime(Ptr<Packet> pkt, std::string * modName) {
  return GetTxTime(pkt->GetSize(), modName);
}

double
AquaSimMac::GetSizeByTxTime(double txTime, std::string * modName) {
  return Phy()->CalcPktSize(txTime, modName);
}

void AquaSimMac::InterruptRecv(double txTime){
  //assert(initialized());
  NS_ASSERT(m_device);// && m_phy && m_rout);

  //FIXME Isn't this violating the busy terminal problem???
  //transmission is already being overheard and therefore can not stop receiving in order to send
  if (RECV == Device()->GetTransmissionStatus()){
	  Phy()->StatusShift(txTime);
  }
}

void
AquaSimMac::NotifyRx (std::string context, Ptr<Packet> p)
{
  SendUp(p);
  NS_LOG_UNCOND(context << " RX " << p->ToString());
  //m_macRxTrace(p);
}

void
AquaSimMac::NotifyTx (std::string context, Ptr<Packet> p)
{
  SendDown(p);
  NS_LOG_UNCOND(context << " TX " << p->ToString());
  //m_macTxTrace(p);
}

bool
AquaSimMac::SendQueueEmpty()
{
  return m_sendQueue.empty();
}

std::pair<Ptr<Packet>,TransStatus>
AquaSimMac::SendQueuePop()
{
  std::pair<Ptr<Packet>,TransStatus> element = m_sendQueue.front();
  m_sendQueue.front().first=0;
  m_sendQueue.pop();
  return element;
}

Ptr<AquaSimNetDevice>
AquaSimMac::Device()
{
  return m_device;
}

Ptr<AquaSimPhy>
AquaSimMac::Phy()
{
  return m_device->GetPhy();
}

Ptr<AquaSimRouting>
AquaSimMac::Routing()
{
  return m_device->GetRouting();
}
double
AquaSimMac::GetBitRate()
{
  return m_bitRate;
}

double
AquaSimMac::GetEncodingEff()
{
  return m_encodingEfficiency;
}

void
AquaSimMac::SetBitRate(double bitRate)
{
  m_bitRate = bitRate;
}

void
AquaSimMac::SetEncodingEff(double encodingEff)
{
  m_encodingEfficiency = encodingEff;
}

void AquaSimMac::DoDispose()
{
  NS_LOG_FUNCTION(this);
  m_device=0;
  while(!m_sendQueue.empty()) {
    m_sendQueue.front().first=0;
    m_sendQueue.pop();
  }
  Object::DoDispose();
}

} // namespace ns3
