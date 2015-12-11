//...

#include "aqua-sim-mac.h"
#include "aqua-sim-header.h"

#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/address.h"
#include "ns3/ptr.h"

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
  .AddAttribute ("SetNetDevice", "A pointer to connect to the net device.",
    PointerValue (),
    MakePointerAccessor (&AquaSimMac::m_device),
    MakePointerChecker<AquaSimMac> ())
  .AddAttribute ("SetPhy", "A pointer to set the phy layer.",
    PointerValue (),
    MakePointerAccessor (&AquaSimMac::m_phy),
    MakePointerChecker<AquaSimMac> ())
  .AddAttribute ("SetRouting", "A pointer to set the routing layer.",
    PointerValue (),
    MakePointerAccessor (&AquaSimMac::m_rout),
    MakePointerChecker<AquaSimMac> ())
  ;
  return tid;
}

AquaSimMac::AquaSimMac()
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

void
AquaSimMac::SetPhy(Ptr<AquaSimPhy> phy){
  NS_LOG_FUNCTION(this << phy);
  m_phy = phy;
}

void
AquaSimMac::SetRouting(Ptr<AquaSimRouting> rout){
  NS_LOG_FUNCTION(this << rout);
  m_rout = rout;
}

void
AquaSimMac::SetAddress(Address addr)
{
  NS_LOG_FUNCTION(this << addr);
  m_address = addr;
}

void
AquaSimMac::RecvProcess(Ptr<Packet> p){
  NS_LOG_FUNCTION (this << " a dummy version.");
}

void
AquaSimMac::TxProcess(Ptr<Packet> p){
  NS_LOG_FUNCTION(this << " a dummy version.");
}

void
AquaSimMac::SetForwardUpCallback(Callback<void, Ptr<Packet>, Address> upCallback)
{
  //not currently used.
  m_callback = upCallback;
}

void
AquaSimMac::SendUp(Ptr<Packet> p)
{
  NS_ASSERT(m_device && m_phy && m_rout);

  m_rout->Recv(p);
}

void
AquaSimMac::SendDown(Ptr<Packet> p)
{
  NS_ASSERT(m_device && m_phy && m_rout);

  m_phy->Recv(p);
}

void
AquaSimMac::HandleIncomingPkt(Ptr<Packet> p) { 	
  NS_LOG_FUNCTION(this);

  //m_recvChannel->AddNewPacket(p);
  AquaSimHeader asHeader;
  p->PeekHeader(asHeader);

  double txTime = asHeader.GetTxTime();
  if (Phy()->Status() != PHY_SEND) {
      m_device->SetCarrierSense(true);
  }
  p->AddHeader(asHeader);

  Simulator::Schedule(Seconds(txTime), &AquaSimMac::Recv, this, p);
  return;
}

void
AquaSimMac::HandleOutgoingPkt(Ptr<Packet> p) {
  NS_LOG_FUNCTION(this);
  //m_callback = h;
  /*
  *  TODO Handle busy terminal problem before trying to tx packet
  */
  TxProcess(p);
}

void
AquaSimMac::Recv(Ptr<Packet> p) {
  //assert(initialized());
  NS_ASSERT(m_device && m_phy && m_rout);
  AquaSimHeader asHeader;
  p->PeekHeader(asHeader);

  if (asHeader.GetDirection() == AquaSimHeader::DOWN){
	  // Handle outgoing packets.
	  HandleOutgoingPkt(p);
  }
  else {
	  // Handle incoming packets.
	  HandleIncomingPkt(p);
  }
}

void
AquaSimMac::PowerOn()
{
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

int
AquaSimMac::GetSizeByTxTime(double txTime, std::string * modName) {
  return Phy()->CalcPktSize(txTime, modName);
}

void AquaSimMac::InterruptRecv(double txTime){
  //assert(initialized());
  NS_ASSERT(m_device && m_phy && m_rout);

  if (PHY_RECV == Phy()->Status()){
	  Phy()->StatusShift(txTime);
  }
}

} // namespace ns3
