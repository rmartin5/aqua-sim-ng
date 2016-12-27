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

#ifndef AQUA_SIM_ENERGY_MODEL_H
#define AQUA_SIM_ENERGY_MODEL_H

#include "ns3/device-energy-model.h"
//#include "aqua-sim-net-device.h"

/*
Aqua Sim Energy model
Inherited from DeviceEnergyModel

This should be overloaded to match necessity.

Base case is very similar to UAN's AcousticModemEnergyModel.
*/

namespace ns3 {

class AquaSimNetDevice;

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Energy model class to assist in recording and keeping state of node's energy.
 */
class AquaSimEnergyModel : public DeviceEnergyModel
{
public:
  static TypeId GetTypeId(void);
  AquaSimEnergyModel();
  virtual ~AquaSimEnergyModel();

  virtual void SetDevice(Ptr<AquaSimNetDevice> device);
  virtual Ptr<AquaSimNetDevice> GetDevice(void) const;

  virtual void ChangeState(int newState);
  virtual double DoGetCurrentA(void) const; //maybe depending if current draw for its states are know or not
  virtual void HandleEnergyDepletion(void);
  virtual void HandleEnergyRecharged(void);

  //inherited
  double GetCurrentA(void) const;
  double GetTotalEnergyConsumption(void) const;
  void SetEnergySource(Ptr<EnergySource> source); //only called by DeviceEnergyModel helper...


  //include callback if energy is <= 0.0 during decreasing... to call energy depleted on Phy using device...
  ///Initial energy setters or for resetting
  void SetRxPower(double rxP);
  void SetTxPower(double txP);
  void SetIdlePower(double idleP);
  void SetEnergy(double energy);
  void SetInitialEnergy(double initialEnergy);
  ///Energy getters
  double GetRxPower(void);
  double GetTxPower(void);
  double GetIdlePower(void);
  double GetEnergy(void);
  double GetInitialEnergy(void);
  ///To be called after an event occurs
  void DecrIdleEnergy(double t, double idleP);
  void DecrRcvEnergy(double t, double rcv);
  void DecrTxEnergy(double t, double pT);
  void DecrEnergy(double t, double decrEnergy);  //allow user to specify energy decr value

  /* TODO : Future work needs to incorporate total times in each state */

protected:
  void DoDispose();

private:
  double m_energy;
  double m_initialEnergy;
  double m_rxP, m_txP, m_idleP;
  double m_totalEnergyConsumption;	//if energy recharging where incorporated

  Ptr<AquaSimNetDevice> m_device;
  Ptr<EnergySource> m_source;


};  //AquaSimEnergyModel class

}  //namespace ns3

#endif /* AQUA_SIM_ENERGY_MODEL_H */
