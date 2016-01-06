
//....

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

  void SetRxPower(double rxP);
  void SetTxPower(double txP);
  void SetIdlePower(double idleP);
  void SetEnergy(double energy);
  void SetInitialEnergy(double initialEnergy);

  double GetRxPower(void);
  double GetTxPower(void);
  double GetIdlePower(void);
  double GetEnergy(void);
  void DecrIdleEnergy(double t);
  void DecrRcvEnergy(double t);
  void DecrTxEnergy(double t);
  void DecrEnergy(double t, double decrEnergy);  //allow user to specify energy decr value

  /* TODO : Future work needs to incorporate total times in each state */

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
