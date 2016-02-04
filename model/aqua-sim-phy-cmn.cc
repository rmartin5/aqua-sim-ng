///... header


//#include ...
#include <string>
#include <vector>

#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"

#include "aqua-sim-header.h"
#include "aqua-sim-energy-model.h"
#include "aqua-sim-phy-cmn.h"
#include "aqua-sim-phy.h"

//Aqua Sim Phy Cmn

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimPhyCmn");
NS_OBJECT_ENSURE_REGISTERED(AquaSimPhyCmn);

AquaSimPhyCmn::AquaSimPhyCmn(void) :
    m_powerLevels(1, 0.660),	/*0.660 indicates 1.6 W drained power for transmission*/
    m_sinrChecker(NULL)//, m_idleTimer(this)
{
  NS_LOG_FUNCTION(this);

  m_updateEnergyTime = Simulator::Now().GetSeconds();
  m_preamble = 1.5;
  m_trigger = 0.45;
  m_status = PHY_IDLE;
  //m_ant = NULL;
  m_eM = NULL;
  m_channel = NULL;
  m_mac = NULL;

  m_ptLevel = 0; 
  m_ptConsume = 0.660;
  m_prConsume = 0.395; 
  m_PoweredOn = true;

  m_RXThresh = 0;
  m_CSThresh = 0;
  m_CPThresh = 10;
  m_pT = 0.2818;
  m_EnergyTurnOn = 0;
  m_EnergyTurnOff = 0;
  m_lambda = 0.0;
  m_pIdle = 0.0;
  m_L = 0;
  m_K = 2.0;
  m_freq = 25;

  m_modulationName = "default";
  Ptr<AquaSimModulation> mod = CreateObject<AquaSimModulation>();
  AddModulation(mod, "default");
  m_sC = CreateObject<AquaSimSignalCache>();
  AttachPhyToSignalCache(m_sC, this);

  incPktCounter = 0;	//debugging purposes only
  outPktCounter = 0;

  Simulator::Schedule(Time(1.0), &AquaSimPhyCmn::Expire, this);	//start energy drain
}

AquaSimPhyCmn::~AquaSimPhyCmn(void)
{
  NS_LOG_FUNCTION(this);
  DoDispose();
}

TypeId
AquaSimPhyCmn::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimPhyCmn")
    .SetParent<AquaSimPhy> ()
    .AddConstructor<AquaSimPhyCmn> ()
    .AddAttribute("CPThresh", "Capture Threshold (db), default is 10.0 set as 10.",
      DoubleValue (10),
      MakeDoubleAccessor(&AquaSimPhyCmn::m_CPThresh),
      MakeDoubleChecker<double> ())
    .AddAttribute("CSThresh", "Carrier sense threshold (W), default is 1.559e-11 set as 0.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimPhyCmn::m_CSThresh),
      MakeDoubleChecker<double>())
    .AddAttribute("RXThresh", "Receive power threshold (W), default is 3.652e-10 set as 0.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimPhyCmn::m_RXThresh),
      MakeDoubleChecker<double>())
    .AddAttribute("PT", "Transmitted signal power (W).",
      DoubleValue(0.2818),
      MakeDoubleAccessor(&AquaSimPhyCmn::m_pT),
      MakeDoubleChecker<double>())
    .AddAttribute("Frequency", "The frequency, default is 25(khz).",
      DoubleValue(25),
      MakeDoubleAccessor(&AquaSimPhyCmn::m_freq),
      MakeDoubleChecker<double>())
    .AddAttribute("L", "System loss default factor.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimPhyCmn::m_L),
      MakeDoubleChecker<double>())
    .AddAttribute("K", "Energy spread factor, spherical spreading. Default is 2.0.",
      DoubleValue(2.0),
      MakeDoubleAccessor(&AquaSimPhyCmn::m_K),
      MakeDoubleChecker<double>())
    .AddAttribute("TurnOnEnergy", "Energy consumption for turning on the modem (J).",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimPhyCmn::m_EnergyTurnOn),
      MakeDoubleChecker<double>())
    .AddAttribute("TurnOffEnergy", "Energy consumption for turning off the modem (J).",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimPhyCmn::m_EnergyTurnOff),
      MakeDoubleChecker<double>())
    .AddAttribute("Preamble", "Duration of preamble.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimPhyCmn::m_preamble),
      MakeDoubleChecker<double>())
    .AddAttribute("Trigger", "Duration of trigger.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimPhyCmn::m_trigger),
      MakeDoubleChecker<double>())
    .AddAttribute("PTLevel", "Level of transmitted signal power.",
      UintegerValue(0),
      MakeUintegerAccessor(&AquaSimPhyCmn::m_ptLevel),
      MakeUintegerChecker<uint32_t> ())
    .AddAttribute("PTConsume", "Power consumption for transmission (W). Default is 0.660 (1.6W).",
      DoubleValue(0.660),
      MakeDoubleAccessor(&AquaSimPhyCmn::m_ptConsume),
      MakeDoubleChecker<double>())
    .AddAttribute("PRConsume", "Power consumption for reception (W). Default is 0.395 (1.2W).",
      DoubleValue(0.395),
      MakeDoubleAccessor(&AquaSimPhyCmn::m_prConsume),
      MakeDoubleChecker<double>())
    .AddAttribute("PIdle", "Idle power consumption (W). Default is 0.0 (0W).",
      DoubleValue(0.0),
      MakeDoubleAccessor(&AquaSimPhyCmn::m_pIdle),
      MakeDoubleChecker<double>())
    .AddAttribute("SignalCache", "Signal cache attached to this node.",
      PointerValue(),
      MakePointerAccessor (&AquaSimPhyCmn::m_sC),
      MakePointerChecker<AquaSimSignalCache>())
    ;
  return tid;
}

void
AquaSimPhyCmn::SetTxPower(double ptConsume)
{
  m_ptConsume = ptConsume;
  //NS_ASSERT(EM() != NULL);
  m_eM->SetTxPower(m_ptConsume);
}

void
AquaSimPhyCmn::SetRxPower(double prConsume)
{
  m_prConsume = prConsume;
  //NS_ASSERT(EM() != NULL);
  m_eM->SetRxPower(m_prConsume);
}

void
AquaSimPhyCmn::SetIdlePower(double pIdle)
{
  m_pIdle = pIdle;
  //NS_ASSERT(EM() != NULL);
  m_eM->SetIdlePower(m_pIdle);
}

/*
void
AquaSimPhyCmn::SetAntenna(Ptr<AquaSimAntenna> ant)
{
  m_ant = ant;
}
*/

void
AquaSimPhyCmn::SetNetDevice(Ptr<AquaSimNetDevice> device)
{
  NS_LOG_FUNCTION(this);
  m_device = device;
}

void
AquaSimPhyCmn::SetChannel(Ptr<AquaSimChannel> channel)
{
  NS_LOG_FUNCTION(this);
  m_channel = channel;
}

void
AquaSimPhyCmn::SetMac(Ptr<AquaSimMac> mac)
{
  NS_LOG_FUNCTION(this);
  m_mac = mac;
}

void
AquaSimPhyCmn::SetSinrChecker(Ptr<AquaSimSinrChecker> sinrChecker)
{
  m_sinrChecker = sinrChecker;
}

void
AquaSimPhyCmn::SetSignalCache(Ptr<AquaSimSignalCache> sC)
{
  m_sC = sC;
  AttachPhyToSignalCache(m_sC,this);
}

void
AquaSimPhyCmn::SetPhyStatus(PhyStatus status)
{
  NS_LOG_FUNCTION(this);
  m_status = status;
}

Ptr<AquaSimSignalCache>
AquaSimPhyCmn::GetSignalCache()
{
  return m_sC;
}

void
AquaSimPhyCmn::AddModulation(Ptr<AquaSimModulation> modulation, std::string modulationName)
{
  /**
  * format:  phyobj add-modulation modulation_name modulation_obj
  * the first added modulation scheme will be the default one
  */
  if (modulation == NULL || modulationName.empty())
    NS_LOG_ERROR("AddModulation NULL value for modulation " << modulation << " or name " << modulationName);
  else if (m_modulations.count(modulationName) > 0)
    NS_LOG_WARN("Duplicate modulations");
  else {
    if (m_modulations.size() == 0)
      m_modulationName = modulationName;

    m_modulations[modulationName] = modulation;
  }
}

Ptr<AquaSimNetDevice>
AquaSimPhyCmn::GetNetDevice()
{
  return m_device;
}

/**
* update energy for transmitting for duration of P_t
*/
void
AquaSimPhyCmn::UpdateTxEnergy(Time txTime, double pT, double pIdle) {
  NS_LOG_FUNCTION(this << "Currently not implemented");
  /*double startTime = Simulator::Now().GetSeconds(), endTime = Simulator::Now().GetSeconds() + txTime.GetSeconds();

  if (NULL != EM()) {
    if (startTime >= m_updateEnergyTime) {
      EM()->DecrIdleEnergy(startTime - m_updateEnergyTime);
      m_updateEnergyTime = startTime;
    }
    EM()->DecrTxEnergy(txTime.GetSeconds());
    m_updateEnergyTime = endTime;
  }
  else
    NS_LOG_FUNCTION(this << " No EnergyModel set.");
  */
}


void
AquaSimPhyCmn::UpdateRxEnergy(Time txTime) {
  NS_LOG_FUNCTION(txTime);

  double startTime = Simulator::Now().GetSeconds();
  double endTime = startTime + txTime.GetSeconds();

  /*if (EM() == NULL) {
    NS_LOG_FUNCTION(this << " No EnergyModel set.");
    return;
  }
  */

  if (startTime > m_updateEnergyTime) {
    //EM()->DecrIdleEnergy(startTime - m_updateEnergyTime);
    //EM()->DecrRcvEnergy(txTime.GetSeconds());
    m_updateEnergyTime = endTime;
  }
  else{
    /* In this case, this device is receiving some other packet*/
    if (endTime > m_updateEnergyTime) {		 //TODO check for errors
      //EM()->DecrRcvEnergy(endTime - m_updateEnergyTime);
      m_updateEnergyTime = endTime;
    }
  }


  /*if (EM()->Energy() <= 0) {
    EM()->SetEnergy(-1);
    m_device->LogEnergy(0);
  }
  */
}

void
AquaSimPhyCmn::UpdateIdleEnergy() {
  if (!m_PoweredOn /*|| EM() == NULL*/ )
    return;

  if (Simulator::Now().GetSeconds() > m_updateEnergyTime && m_PoweredOn) {
    //EM()->DecrIdleEnergy(Simulator::Now().GetSeconds() - m_updateEnergyTime);
    m_updateEnergyTime = Simulator::Now().GetSeconds();
  }

  // log device energy
  /*if (EM()->Energy() > 0) {
    m_device->LogEnergy(1);
  }
  else {
    m_device->LogEnergy(0);
  }
  */

  Simulator::Schedule(Time(1.0), &AquaSimPhyCmn::UpdateIdleEnergy, this);	//m_idleTimer.resched(1.0);
}

bool
AquaSimPhyCmn::Decodable(double noise, double ps) {
  double epsilon = 1e-6;	//accuracy for float comparison

  if (ps < m_RXThresh) //signal is too weak
    return false;

  if (fabs(noise) <  epsilon) {
    //avoid being divided by 0 when calculating SINR
    return true;
  }

  return m_sinrChecker->Decodable(ps / noise);
}

/**
* stamp the packet with information required by channel
* different channel model may require different information
* overload this method if needed
*/
Ptr<Packet> 
AquaSimPhyCmn::StampTxInfo(Ptr<Packet> p) {
NS_LOG_FUNCTION(this << "not currently supported.");
return p;
}

/**
* we will cache the incoming packet in phy layer
* and send it to MAC layer after receiving the entire one
*/
bool
AquaSimPhyCmn::Recv(Ptr<Packet> p) {  // Handler* h
  NS_LOG_FUNCTION(this << p);

  AquaSimHeader asHeader;
  p->PeekHeader(asHeader);
  //NS_LOG_DEBUG ("direction=" << asHeader.GetDirection());

  if (asHeader.GetDirection() == AquaSimHeader::DOWN) {
    NS_LOG_DEBUG("Phy_Recv DOWN. Pkt counter(" << outPktCounter++ << ") on node(" <<
		 m_device->GetNode() << ")");
    PktTransmit(p);
  }
  else {
    if (asHeader.GetDirection() != AquaSimHeader::UP) {
      NS_LOG_WARN("Direction for pkt-flow not specified, "
	      "sending pkt up the stack on default.");
    }

    NS_LOG_DEBUG("Phy_Recv UP. Pkt counter(" << incPktCounter++ << ") on node(" <<
		 m_device->GetNode() << ")");
    p = PrevalidateIncomingPkt(p);

    if (p != NULL) {
      //put the packet into the incoming queue
      m_sC->AddNewPacket(p);
    }
  }
  return true; //TODO fix this.
}

bool AquaSimPhyCmn::MatchFreq(double freq) {
  double epsilon = 1e-6;	//accuracy for float comparison

  return std::fabs(freq - m_freq) < epsilon;
}

/**
* This function pre-validate a incoming packet,
* it checks if this reception fails because of a hardware issue
* meanwhile, we update the energy consumption on this node.
*
* @param p the received packet
* @return  NULL if this packet cannot be received even if it cannot be decoded
* 			p would be freed and set to NULL in this case
* 			otherwise, return p
*/
Ptr<Packet>
AquaSimPhyCmn::PrevalidateIncomingPkt(Ptr<Packet> p) {
  NS_LOG_FUNCTION(this << p);

  AquaSimHeader asHeader;
  p->RemoveHeader(asHeader);
  NS_LOG_DEBUG ("TxTime=" << asHeader.GetTxTime());
  Time txTime = asHeader.GetTxTime();

  if (m_device->FailureStatus()) {
    NS_LOG_WARN("AquaSimPhyCmn: nodeId=" << m_device->GetNode()->GetId() << " fails!\n");
    p = 0;
    return NULL;
  }

  if (!MatchFreq(asHeader.GetFreq())) {
    NS_LOG_WARN("AquaSimPhyCmn: Cannot match freq(" << asHeader.GetFreq() << ") on node(" <<
		m_device->GetNode() << ")");
    p = 0;
    return NULL;
  }

  /**
  * any packet error set here result from that a packet
  * cannot be detected by the modem, so modem's status doesn't receive
  */
  if (/*(EM() && EM()->Energy() <= 0) ||*/ Status() == PHY_SLEEP
				      || Status() == PHY_SEND
				      || asHeader.GetPr() < m_RXThresh
				      || Status() == PHY_DISABLE) {

    /**
    * p still can pass since its signal may affect other packets
    * when this node wake up or start to receive other packets
    */
    NS_LOG_DEBUG(this << "packet error");
    asHeader.SetErrorFlag(true);
  }

  else {
    Status() = PHY_RECV;
  }

  UpdateRxEnergy(txTime);

  p->AddHeader(asHeader);

  return p;
}

/**
* pass packet p to channel
*/
bool
AquaSimPhyCmn::PktTransmit(Ptr<Packet> p) {
  NS_LOG_FUNCTION(this);

  AquaSimHeader asHeader;
  p->RemoveHeader(asHeader);

  if (m_device->FailureStatus()) {
    NS_LOG_WARN("AquaSimPhyCmn nodeId=" << m_device->GetNode()->GetId() << " fails!\n");
    p = 0;
    return false;
  }

  if (Status() == PHY_SLEEP || /*(NULL != EM() && EM()->Energy() <= 0) ||*/ Status() == PHY_DISABLE)
  {
    NS_LOG_DEBUG("Unable to reach phy layer (sleep/disable)");
    p = 0;
    return false;
  }

  switch (Status()){
  case PHY_SEND:
    UpdateTxEnergy(asHeader.GetTxTime(), m_ptConsume, m_pIdle);
    break;
  case PHY_IDLE:
    /*
     * Something went wrong here...
     */
    NS_LOG_WARN("AquaSimPhyCmn node(" << m_device->GetNode() << "," <<  m_device->GetNode()->GetId()
		<< "):mac forgot to change the status at time " << Simulator::Now());
    return false;
    break;
  case PHY_SLEEP:
    NS_LOG_WARN("AquaSimPhyCmn node(" << m_device->GetNode()->GetId() << ") is sleeping! (dropping pkt)");
    return false;
    break;
  default:
    NS_LOG_WARN("AquaSimPhyCmn: wrong status (dropping pkt)");
    return false;
  }

  /*
  *  Stamp the packet with the interface arguments
  */
  asHeader.Stamp(GetPointer(p), m_pT, m_lambda);

  asHeader.SetFreq(m_freq);
  asHeader.SetPt(m_powerLevels[m_ptLevel]);
  asHeader.SetModName(m_modulationName);
  asHeader.SetErrorFlag(false);

  Time txSendDelay = this->CalcTxTime(p->GetSize(), &m_modulationName );
  Simulator::Schedule(txSendDelay, &AquaSimPhyCmn::SetPhyStatus, this, PHY_IDLE);

  /**
  * here we simulate multi-channel (different frequencies),
  * not multiple tranceiver, so we pass the packet to channel_ directly
  * p' uw_txinfo_ carries channel frequency information
  */

  p->AddHeader(asHeader);

  asHeader.Print(std::cout);

  return m_channel->Recv(p, this);
}

/**
* send packet to upper layer, supposed to be MAC layer,
* but actually go to trace module first
*/
void
AquaSimPhyCmn::SendPktUp(Ptr<Packet> p) {	//TODO this should probably be a Callback
  NS_LOG_FUNCTION(this);

  if (!m_mac->Recv(p))
    NS_LOG_DEBUG(this << "Mac Recv error");
}

/**
* process packet when signal
*/
void
AquaSimPhyCmn::SignalCacheCallback(Ptr<Packet> p) {
  NS_LOG_FUNCTION(this << p);

  SendPktUp(p);
}

void
AquaSimPhyCmn::PowerOn() {
  NS_LOG_FUNCTION(this);

  if (Status() == PHY_DISABLE)
    NS_LOG_FUNCTION(this << " Node " << m_device->GetNode() << " is disabled.");
  else
  {
    m_PoweredOn = true;
    m_status = PHY_IDLE;
    /*if (EM() != NULL) {
	    //minus the energy consumed by power on
	    EM()->SetEnergy(std::max(0, EM->Energy() - m_EnergyTurnOn));
	    m_updateEnergyTime = std::max(Simulator::Now().GetSeconds, m_updateEnergyTime);
    }
    */
  }
}

void
AquaSimPhyCmn::PowerOff() {
  NS_LOG_FUNCTION(this);

  if (Status() == PHY_DISABLE)
    NS_LOG_FUNCTION(this << " Node " << m_device->GetNode() << " is disabled.");
  else
  {
    m_PoweredOn = false;
    m_status = PHY_SLEEP;
    /*if (EM() == NULL)
	    return;
    */

    //minus the energy consumed by power off
    //EM()->SetEnergy(std::max(0, EM->Energy() - m_EnergyTurnOff));

    if (Simulator::Now().GetSeconds() > m_updateEnergyTime) {
      //EM()->DecrIdleEnergy(Simulator::Now().GetSeconds() - m_updateEnergyTime);
      m_updateEnergyTime = Simulator::Now().GetSeconds();
    }
  }
}

void 
AquaSimPhyCmn::Dump(void) const
{
  NS_LOG_DEBUG("AquaSimPhyCmn Dump: Channel(" << m_channel << ") " <<
	       "Pt(" << m_pT << ") " <<
	       //"Gt(" << m_ant->GetTxGain(0, 0, 0, m_lambda) << ") " <<
	       "lambda(" << m_lambda << ") " <<
	       "L(" << m_L << ")\n");
}

void
AquaSimPhyCmn::StatusShift(double txTime) {
  double endTime = Simulator::Now().GetSeconds() + txTime;
  /*  The receiver is receiving a packet when the
  transmitter begins to transmit a data.
  We assume the half-duplex mode, the transmitter
  stops the receiving process and begins the sending
  process.
  */
  if (m_updateEnergyTime < endTime)
  {
    //double overlapTime = m_updateEnergyTime - Simulator::Now().GetSeconds();
    //double actualTxTime = endTime - m_updateEnergyTime;
    //EM()->DecrEnergy(overlapTime, m_ptConsume - m_prConsume);
    //EM()->DecrTxEnergy(actualTxTime);
    m_updateEnergyTime = endTime;
  }
  else {
    //double overlapTime = txTime;
    //EM()->DecrEnergy(overlapTime, m_ptConsume - m_prConsume);
  }
}

/**
* @para ModName the name of selected modulation scheme
*	@return     NULL if ModName cannot be found in m_modulations
*	@return		a pointer to the corresponding AquaSimModulation obj
*/
Ptr<AquaSimModulation>
AquaSimPhyCmn::Modulation(std::string * modName) {
  std::map<std::string, Ptr<AquaSimModulation> >::iterator pos;
  if (m_modulations.size() == 0) {
    NS_LOG_WARN("No modulations\n");
    return NULL;
  }
  else if (modName == NULL) {
    modName = &m_modulationName;
    pos = m_modulations.find(*modName);
    if (m_modulations.end() != pos) {
      return pos->second;
    }
    else {
      NS_LOG_WARN("Failed to locate modulation " << modName->c_str() << "\n");
      return NULL;
    }
  }
  return NULL;
}

void
AquaSimPhyCmn::EnergyDeplete() {
  NS_LOG_FUNCTION(this);
  NS_LOG_DEBUG("Energy is depleted on node " << m_device->GetNode());

  m_status = PHY_DISABLE;
}

void
AquaSimPhyCmn::Expire(void) {
  NS_LOG_INFO("Expire not currently implemented.");
  //m_idleTimer.Expire();
}

/**
 * calculate transmission time of a packet of size pktsize
 * we consider the preamble
 */
Time
AquaSimPhyCmn::CalcTxTime (uint32_t pktSize, std::string * modName)
{
  //TODO fix the variables for this to actually make sense...
    //also an Assert for the given name being NULL

  return Time::FromDouble(m_modulations.find(m_modulationName)->second->TxTime(pktSize), Time::S)
      + Time::FromInteger(Preamble(), Time::S);
}

int
AquaSimPhyCmn::CalcPktSize (double txTime, std::string * modName)
{
  return Modulation(modName)->PktSize (txTime - Preamble());
}

/*
void
AquaSimIdleTimer::Expire(void)
{
  m_a->UpdateIdleEnergy();
  Simulator::Schedule(Time(1.0), &AquaSimIdleTimer::Expire, this);
}
*/

};  // namespace ns3
