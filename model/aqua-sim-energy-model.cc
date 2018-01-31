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

#include "ns3/energy-source.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/double.h"

#include "aqua-sim-energy-model.h"
#include "aqua-sim-net-device.h"
#include "aqua-sim-phy.h"

// Aqua Sim Energy Model

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimEnergyModel");
NS_OBJECT_ENSURE_REGISTERED(AquaSimEnergyModel);

TypeId
AquaSimEnergyModel::GetTypeId()
{
  static TypeId tid = TypeId ("ns3::AquaSimEnergyModel")
    .SetParent<DeviceEnergyModel> ()
    .AddConstructor<AquaSimEnergyModel> ()
    .AddAttribute ("NetDevice", "The Aqua Sim Net Device this model resides on.",
      PointerValue (),
      MakePointerAccessor (&AquaSimEnergyModel::m_device),
      MakePointerChecker<AquaSimNetDevice>())
    .AddAttribute ("RxPower", "Rx power",
      DoubleValue (0.75),
      MakeDoubleAccessor (&AquaSimEnergyModel::m_rxP),
      MakeDoubleChecker<double>())
    .AddAttribute ("TxPower", "Tx power",
      DoubleValue (2.0),
      MakeDoubleAccessor (&AquaSimEnergyModel::m_txP),
      MakeDoubleChecker<double>())
    .AddAttribute ("InitialEnergy", "Starting energy",
      DoubleValue (10000.0),
      MakeDoubleAccessor (&AquaSimEnergyModel::SetInitialEnergy),
      MakeDoubleChecker<double>())
    .AddAttribute ("IdlePower", "Idle power",
      DoubleValue (0.008),
      MakeDoubleAccessor (&AquaSimEnergyModel::m_idleP),
      MakeDoubleChecker<double>())
    ;
  return tid;
}

AquaSimEnergyModel::AquaSimEnergyModel() :
    m_energy(10000.0),
    m_initialEnergy(10000.0),
    m_rxP(0.75),
    m_txP(2.0),
    m_idleP(0.008),
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
  NS_LOG_FUNCTION(this << energy);

  m_energy = energy;
}
void
AquaSimEnergyModel::SetInitialEnergy(double initialEnergy)
{
  m_initialEnergy = initialEnergy;
  SetEnergy(m_initialEnergy);
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
double
AquaSimEnergyModel::GetInitialEnergy()
{
  return m_initialEnergy;
}

void
AquaSimEnergyModel::DecrIdleEnergy(double t, double idleP)
{
  NS_LOG_FUNCTION(this << m_energy);

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
AquaSimEnergyModel::DecrRcvEnergy(double t, double rcv)
{
  NS_LOG_FUNCTION(this);

  double dEng = t * rcv;
  if (m_energy <= dEng) {
	  m_energy = 0.0;
	  HandleEnergyDepletion();
  }
  else
	  m_energy -= dEng;

  m_totalEnergyConsumption += dEng;
}

void
AquaSimEnergyModel::DecrTxEnergy(double t, double pT)
{
  NS_LOG_FUNCTION(this);

  double dEng = t * pT;
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

void
AquaSimEnergyModel::DoDispose()
{
  NS_LOG_FUNCTION(this);
  m_device=0;
  m_source=0;
}
