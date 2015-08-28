/// ... header

#include "aqua-sim-energy-model.h"
#include "aqua-sim-node.h"
#include "ns3/energy-source.h"

// Aqua Sim Energy Model

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimEnergyModel");
NS_OBJECT_ENSURE_REGISTERED(AquaSimEnergyModel);


TypeId 
AquaSimEnergyModel::GetTypeId(void);  //TODO

AquaSimEnergyModel::AquaSimEnergyModel() : m_energy(0.0),
m_initialEnergy(0.0), rxP(0), txP(0), idleP(0), 
m_node(NULL), m_source(0)
{
}

AquaSimEnergyModel::~AquaSimEnergyModel()
{
}

void 
AquaSimEnergyModel::SetNode(Ptr<AquaSimNode> node)
{
	m_node = node;
}

Ptr<AquaSimNode>
AquaSimEnergyModel::GetNode() const
{
	return m_node;
}

void 
AquaSimEnergyModel::ChangeState(int newState)
{
	NS_LOG_FUNCTION(this << newState);
}

double
AquaSimEnergyModel::DoGetCurrentA(void) const
{
	NS_LOG_FUNCTION(this);
	GetCurrentA();
}

void 
AquaSimEnergyModel::HandleEnergyDepletion(void)
{
	NS_LOG_FUNCTION(this);
	NS_LOG_DEBUG(this << "Energy is depleted on node " << m_node
		<< ", calling AquaSimPhy::EnergyDeplete");
	
	Ptr<AquaSimNetDevice> dev = m_node->GetDevice(0)->GetObject<AquaSimNetDevice>();
	dev->GetPhy()->EnergyDeplete();
}

void 
AquaSimEnergyModel::HandleEnergyRecharged(void)
{
	//Not currently used.
}


double 
AquaSimEnergyModel::GetCurrentA(void) const
{
	//Not currently used.
	NS_LOG_FUNCTION(this);
}

double 
AquaSimEnergyModel::GetTotalEnergyConsumption(void) const
{
	return m_totalEnergyConsumption;
}

//only called by DeviceEnergyModel helper...
void 
AquaSimEnergyModel::SetEnergySource(Ptr<EnergySource> source)
{
	NS_LOG_FUNCTION(this);
	NS_ASSERT(source != 0);

	m_source = source;
}

void
AquaSimEnergyModel::SetRxPower(double rxP)
{
	m_rxP = rxP;
}
void
AquaSimEnergyModel::SetTxPower(double txP)
{
	m_txP = txP;
}
void
AquaSimEnergyModel::SetIdlePower(double idleP)
{
	m_idleP = idleP;
}
void
AquaSimEnergyModel::SetEnergy(double energy)
{
	m_energy = energy;
}
void
AquaSimEnergyModel::SetInitialEnergy(double initialEnergy)
{
	m_initialEnergy = initialEnergy;
}

double
AquaSimEnergyModel::GetRxPower()
{
	return m_rxP;
}
double
AquaSimEnergyModel::GetTxPower()
{
	return m_txP;
}
double
AquaSimEnergyModel::GetIdlePower()
{
	return idleP;
}
double
AquaSimEnergyModel::GetEnergy()
{
	return m_energy;
}


void
AquaSimEnergyModel::DecrIdleEnergy(double t)
{
	NS_LOG_FUNCTION(this);

	double dEng = t * m_idleP;
	if (m_energy <= dEng) {
		m_energy = 0.0;
		HandleEnergyDepletion();
	}
	else
		m_energy -= dEng;
	
	m_totalEnergyConsumption += dEng;
}

void
AquaSimEnergyModel::DecrRcvEnergy(double t)
{
	NS_LOG_FUNCTION(this);

	double dEng = t * m_rxP;
	if (m_energy <= dEng) {
		m_energy = 0.0;
		HandleEnergyDepletion();
	}
	else
		m_energy -= dEng;

	m_totalEnergyConsumption += dEng;
}

void
AquaSimEnergyModel::DecrTxEnergy(double t)
{
	NS_LOG_FUNCTION(this);

	double dEng = t * m_txP;
	if (m_energy <= dEng) {
		m_energy = 0.0;
		HandleEnergyDepletion();
	}
	else
		m_energy -= dEng;

	m_totalEnergyConsumption += dEng;
}

void
AquaSimEnergyModel::DecrEnergy(double t, double decrEnergy)
{
	NS_LOG_FUNCTION(this);

	double dEng = t * decrEnergy;
	if (m_energy <= dEng) {
		m_energy = 0.0;
		HandleEnergyDepletion();
	}
	else
		m_energy -= dEng;

	m_totalEnergyConsumption += dEng;
}

} // namespace ns3
