///... header


//#include ...
#include <string>
#include <vector>

#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"

#include "aqua-sim-phy.h"
#include "aqua-sim-packetstamp.h"
#include "aqua-sim-header.h"
#include "aqua-sim-energy-model.h"

//Aqua Sim Phy

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimPhy");

void
AquaSimIdleTimer::Expire(void)
{
  m_a->UpdateIdleEnergy();
  Simulator::Schedule(Time(1.0), &AquaSimIdleTimer::Expire, this);
}


NS_OBJECT_ENSURE_REGISTERED(AquaSimPhy);

AquaSimPhy::AquaSimPhy(void) : Object(), m_idleTimer(this)
{

  m_updateEnergyTime = Simulator::Now().GetSeconds();
  m_preamble = 1.5;
  m_trigger = 0.45;
  m_status = PHY_IDLE;
  m_modulationName = "";
  //m_ant = NULL;
  m_node = NULL;
  m_sC = NULL; 
  m_sinrChecker = NULL;
  m_eM = NULL;

  m_ptLevel = 0; 
  m_ptConsume = 0.660;
  m_prConsume = 0.395; 
  m_powerLevels[1] = 0.660;  /*0.660 indicates 1.6 W drained power for transmission*/
  m_PoweredOn = true;

  Simulator::Schedule(Time(1.0), &AquaSimPhy::Expire, this);	//start energy drain
}

AquaSimPhy::~AquaSimPhy(void)
{
  //delete m_sC;
}

TypeId
AquaSimPhy::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimPhy")
    .SetParent<Object>()
    //.AddConstructor<AquaSimPhy>()
    .AddAttribute("CPThresh", "Capture Threshold (db), default is 10.0 set as 10.",
      DoubleValue (10),
      MakeDoubleAccessor(&AquaSimPhy::m_CPThresh),
      MakeDoubleChecker<double> ())
    .AddAttribute("CSThresh", "Carrier sense threshold (W), default is 1.559e-11 set as 0.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimPhy::m_CSThresh),
      MakeDoubleChecker<double>())
    .AddAttribute("RXThresh", "Receive power threshold (W), default is 3.652e-10 set as 0.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimPhy::m_RXThresh),
      MakeDoubleChecker<double>())
    .AddAttribute("PT", "Transmitted signal power (W).",
      DoubleValue(0.2818),
      MakeDoubleAccessor(&AquaSimPhy::m_pT),
      MakeDoubleChecker<double>())
    .AddAttribute("Frequency", "The frequency, default is 25(khz).",
      DoubleValue(25),
      MakeDoubleAccessor(&AquaSimPhy::m_freq),
      MakeDoubleChecker<double>())
    .AddAttribute("L", "System loss default factor.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimPhy::m_L),
      MakeDoubleChecker<double>())
    .AddAttribute("K", "Energy spread factor, spherical spreading. Default is 2.0.",
      DoubleValue(2.0),
      MakeDoubleAccessor(&AquaSimPhy::m_K),
      MakeDoubleChecker<double>())
    .AddAttribute("TurnOnEnergy", "Energy consumption for turning on the modem (J).",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimPhy::m_EnergyTurnOn),
      MakeDoubleChecker<double>())
    .AddAttribute("TurnOffEnergy", "Energy consumption for turning off the modem (J).",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimPhy::m_EnergyTurnOff),
      MakeDoubleChecker<double>())
    .AddAttribute("Preamble", "Duration of preamble.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimPhy::m_preamble),
      MakeDoubleChecker<double>())
    .AddAttribute("Trigger", "Duration of trigger.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimPhy::m_trigger),
      MakeDoubleChecker<double>())
    .AddAttribute("PTLevel", "Level of transmitted signal power.",
      UintegerValue(0),
      MakeUintegerAccessor(&AquaSimPhy::m_ptLevel),
      MakeUintegerChecker<uint32_t> ())
    .AddAttribute("PTConsume", "Power consumption for transmission (W). Default is 0.660 (1.6W).",
      DoubleValue(0.660),
      MakeDoubleAccessor(&AquaSimPhy::m_ptConsume),
      MakeDoubleChecker<double>())
    .AddAttribute("PRConsume", "Power consumption for reception (W). Default is 0.395 (1.2W).",
      DoubleValue(0.395),
      MakeDoubleAccessor(&AquaSimPhy::m_prConsume),
      MakeDoubleChecker<double>())
    .AddAttribute("PIdle", "Idle power consumption (W). Default is 0.0 (0W).",
      DoubleValue(0.0),
      MakeDoubleAccessor(&AquaSimPhy::m_pIdle),
      MakeDoubleChecker<double>())
    ;
  return tid;
}

void
AquaSimPhy::SetTxPower(double ptConsume)
{
	m_ptConsume = ptConsume;
	//NS_ASSERT(EM() != NULL);
	m_eM->SetTxPower(m_ptConsume);	
}

void
AquaSimPhy::SetRxPower(double prConsume)
{
	m_prConsume = prConsume;
	//NS_ASSERT(EM() != NULL);
	m_eM->SetRxPower(m_prConsume);
}

void
AquaSimPhy::SetIdlePower(double pIdle)
{
	m_pIdle = pIdle;
	//NS_ASSERT(EM() != NULL);
	m_eM->SetIdlePower(m_pIdle);
}

/*
void
AquaSimPhy::SetAntenna(Ptr<AquaSimAntenna> ant)
{
	m_ant = ant;
}
*/

void
AquaSimPhy::SetNode(Ptr<AquaSimNode> node)
{
	m_node = node;
}

void
AquaSimPhy::SetSinrChecker(Ptr<AquaSimSinrChecker> sinrChecker)
{
	m_sinrChecker = sinrChecker;
}

void
AquaSimPhy::SetSignalCache(Ptr<AquaSimSignalCache> sC)
{
	m_sC = sC;
	m_sC->AttachPhy(this);	//Need to implement signal cache module for this...
}

void
AquaSimPhy::AddModulation(Ptr<AquaSimModulation> modulation, std::string modulationName)
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

/**
* update energy for transmitting for duration of P_t
*/
void
AquaSimPhy::UpdateTxEnergy(Time txTime, double pT, double pIdle) {
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
AquaSimPhy::UpdateRxEnergy(Time txTime) {
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
		/* In this case, this node is receiving some other packet*/
		if (endTime > m_updateEnergyTime) {		 //TODO check for pkt errors
			//EM()->DecrRcvEnergy(endTime - m_updateEnergyTime);
			m_updateEnergyTime = endTime;
		}
	}


	/*if (EM()->Energy() <= 0) {
		EM()->SetEnergy(-1);
		Node()->LogEnergy(0);
	}
	*/
}

void
AquaSimPhy::UpdateIdleEnergy() {
	if (!m_PoweredOn /*|| EM() == NULL*/ )
		return;

	if (Simulator::Now().GetSeconds() > m_updateEnergyTime && m_PoweredOn) {
		//EM()->DecrIdleEnergy(Simulator::Now().GetSeconds() - m_updateEnergyTime);
		m_updateEnergyTime = Simulator::Now().GetSeconds();
	}

	// log node energy
	/*if (EM()->Energy() > 0) {
		Node()->LogEnergy(1);
	}
	else {
		Node()->LogEnergy(0);
	}
	*/

	Simulator::Schedule(Time(1.0), &AquaSimPhy::UpdateIdleEnergy, this);	//m_idleTimer.resched(1.0);
}

bool
AquaSimPhy::Decodable(double noise, double ps) {
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
AquaSimPhy::StampTxInfo(Ptr<Packet> p) {
	//if (!m_ant)
	//	NS_LOG_WARN("No antenna!\n");

	Ptr<AquaSimPacketStamp> pStamp;
	pStamp->Stamp(GetPointer(p), m_pT, m_lambda);

				//TODO this is not correct...
					// this does not translate to anything
	/*			
	Ptr<AquaSimPacketStamp> const &txInfo = pStamp;
	txInfo->Freq() = Freq();			
	txInfo->Pt() = m_powerLevels[m_ptLevel];
	txInfo->ModName() = m_modulationName;
	*/
				
	return p;
}

/**
* we will cache the incomming packet in phy layer
* and send it to MAC layer after receiving the entire one
*/
void
AquaSimPhy::Recv(Ptr<Packet> p) {  // Handler* h
	NS_ASSERT(Initialized());
	AquaSimHeader asHeader;	
	p->PeekHeader(asHeader);
	NS_LOG_DEBUG ("direction=" << asHeader.GetDirection());

	if (asHeader.GetDirection() == AquaSimHeader::DOWN) {
		SendPktDown(p);
	}
	else {
		if (asHeader.GetDirection() != AquaSimHeader::UP) {
			NS_LOG_WARN("Direction for pkt-flow not specified, "
				"sending pkt up the stack on default.\n");
		}
		
		p = PrevalidateIncomingPkt(p);

		if (p != NULL) {
			//put the packet into the incoming queue
			m_sC->AddNewPacket(p);
		}
	}
}

bool AquaSimPhy::MatchFreq(double freq) {
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
AquaSimPhy::PrevalidateIncomingPkt(Ptr<Packet> p) {
	Ptr<AquaSimPacketStamp> pStamp;
	
	AquaSimHeader asHeader;	
	p->PeekHeader(asHeader);
	NS_LOG_DEBUG ("TxTime=" << asHeader.GetTxTime());
	Time txTime = Time::FromInteger(asHeader.GetTxTime(),Time::S); 		

	if (Node()->FailureStatus()) {
		NS_LOG_WARN("AquaSimPhy: nodeId=" << Node()->GetId() << " fails!\n");
		p = 0;
		return NULL;
	}

	if (!MatchFreq(pStamp->Freq())) {
		p = 0;
		return NULL;
	}

	/**
	* any packet error set here result from that a packet
	* cannot be detected by the modem, so modem's status doesn't receive
	*/
	if (/*(EM() && EM()->Energy() <= 0) ||*/ Status() == PHY_SLEEP
		|| Status() == PHY_SEND
		|| pStamp->Pr() < m_RXThresh
		|| Status() == PHY_DISABLE) {

		/**
		* p still can pass since its signal may affect other packets
		* when this node wake up or start to receive other packets
		*/
		NS_LOG_DEBUG(this << " packet error");
		asHeader.SetErrorFlag(true);
		p->AddHeader(asHeader);
	}

	else {
		Status() = PHY_RECV;
	}

	UpdateRxEnergy(txTime);

	return p;
}

/**
* pass packet p to channel
*/
void
AquaSimPhy::SendPktDown(Ptr<Packet> p) {
	AquaSimHeader asHeader;	
	p->PeekHeader(asHeader);
	NS_LOG_DEBUG ("TxTime=" << asHeader.GetTxTime());

	if (Node()->FailureStatus()) {
		NS_LOG_WARN("AquaSimPhy nodeId=" << Node()->GetId() << " fails!\n");
		p = 0;
		return;
	}

	if (Status() == PHY_SLEEP || /*(NULL != EM() && EM()->Energy() <= 0) ||*/ Status() == PHY_DISABLE) {
		p = 0;
		return;
	}

	switch (Status()){
	case PHY_SEND:
		UpdateTxEnergy(Time::FromInteger(asHeader.GetTxTime(),Time::S), m_ptConsume, m_pIdle);
		//Should be reset to PHY_IDLE once txtime has ended...
		// Does this occur somewhere else??
		break;
	case PHY_IDLE:
		NS_LOG_WARN("AquaSimPhy node(" << Node()->GetId() << "):mac forgot to change"
			<< "the status at time " << Simulator::Now() << "\n");
		break;
	case PHY_SLEEP:
		NS_LOG_WARN("AquaSimPhy node(" << Node()->GetId() << ") is sleeping!\n");
		break;
	default:
		NS_LOG_WARN("AquaSimPhy: wrong status\n");
	}

	/*
	*  Stamp the packet with the interface arguments
	*/
	StampTxInfo(p);

	/**
	* here we simulate multi-channel (different frequencies),
	* not multiple tranceiver, so we pass the packet to channel_ directly
	* p' uw_txinfo_ carries channel frequency information
	*/
	m_channel->Recv(p, this);
}

/**
* send packet to upper layer, supposed to be MAC layer,
* but actually go to trace module first
*/
void
AquaSimPhy::SendPktUp(Ptr<Packet> p)	//TODO this should probably be a Callback
{
	m_mac->Recv(p);
}

/**
* process packet when signal
*/
void
AquaSimPhy::SignalCacheCallback(Ptr<Packet> p) {
	SendPktUp(p);
}

void
AquaSimPhy::PowerOn() {
	if (Status() == PHY_DISABLE)
		NS_LOG_FUNCTION(this << " Node " << m_node << " is disabled.");
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
AquaSimPhy::PowerOff() {
	if (Status() == PHY_DISABLE)
		NS_LOG_FUNCTION(this << " Node " << m_node << " is disabled.");
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
AquaSimPhy::Dump(void) const
{
	NS_LOG_DEBUG("AquaSimPhy Dump: Channel(" << m_channel << ") " <<
		"Pt(" << m_pT << ") " <<
		//"Gt(" << m_ant->GetTxGain(0, 0, 0, m_lambda) << ") " <<
		"lambda(" << m_lambda << ") " <<
		"L(" << m_L << ")\n");
}

/**
* calculate transmission time of a packet of size pktsize
* we consider the preamble
*/
Time
AquaSimPhy::CalcTxTime(int pktSize, Ptr<std::string> modName) {
	return Time::FromDouble(Modulation(modName)->TxTime(pktSize), Time::S) + 
			Time::FromInteger(Preamble(),Time::S);
}

int
AquaSimPhy::CalcPktSize(double txTime, Ptr<std::string> modName) {
	return Modulation(modName)->PktSize(txTime - Preamble());
}

void
AquaSimPhy::StatusShift(double txTime) {
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
		//EM()->DecrEnergy(overlapTime,	m_ptConsume - m_prConsume);
	}
}

/**
* @para ModName the name of selected modulation scheme
*	@return     NULL if ModName cannot be found in m_modulations
*	@return		a pointer to the corresponding AquaSimModulation obj
*/
Ptr<AquaSimModulation>
AquaSimPhy::Modulation(Ptr<std::string> modName) {
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
}

void
AquaSimPhy::EnergyDeplete() {
	NS_LOG_FUNCTION(this);
	NS_LOG_DEBUG("Energy is depleted on node " << m_node);

	m_status = PHY_DISABLE;
}

void
AquaSimPhy::Expire(void) {
	AquaSimIdleTimer::Expire();
}

};  // namespace ns3
