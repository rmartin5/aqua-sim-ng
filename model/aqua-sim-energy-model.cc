/// ... header

#include "ns3/energy-source.h"
#include "ns3/log.h"
#include "ns3/pointer.h"

#include "aqua-sim-energy-model.h"
#include "aqua-sim-net-device.h"
#include "aqua-sim-phy.h"

// Aqua Sim Energy Model

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimEnergyModel");
NS_OBJECT_ENSURE_REGISTERED(AquaSimEnergyModel);

TypeId 
AquaSimEnergyModel::GetTypeId()
{
  static TypeId tid = TypeId ("ns3::AquaSimEnergyModel")
    .SetParent<DeviceEnergyModel> ()
    .AddAttribute ("NetDevice", "The Aqua Sim Net Device this model resides on.",
      PointerValue (),
      MakePointerAccessor (&AquaSimEnergyModel::m_device),
      MakePointerChecker<AquaSimNetDevice>())
    ;
  return tid;
}

AquaSimEnergyModel::AquaSimEnergyModel() :
    m_energy(0.0),
    m_initialEnergy(0.0),
    m_rxP(0),
    m_txP(0),
    m_idleP(0),
    m_totalEnergyConsumption(0.0)
{
  //m_source = 0;
}

AquaSimEnergyModel::~AquaSimEnergyModel()
{
}

void 
AquaSimEnergyModel::SetDevice(Ptr<AquaSimNetDevice> device)
{
  m_device = device;
}

Ptr<AquaSimNetDevice>
AquaSimEnergyModel::GetDevice() const
{
  return m_device;
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
  return GetCurrentA();
}

void 
AquaSimEnergyModel::HandleEnergyDepletion(void)
{
  NS_LOG_FUNCTION(this);
  NS_LOG_DEBUG(this << "Energy is depleted on device " << m_device
	  << ", calling AquaSimPhy::EnergyDeplete");

  m_device->GetPhy()->EnergyDeplete();
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
  return 0;
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
  return m_idleP;
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
