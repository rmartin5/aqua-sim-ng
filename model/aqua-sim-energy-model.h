
//....

#ifndef AQUA_SIM_ENERGY_MODEL_H
#define AQUA_SIM_ENERGY_MODEL_H

#include "ns3/device-energy-model.h"
#include "ns3/nstime.h"
#include "aqua-sim-node.h"

/*
Aqua Sim Energy model
Inherited from DeviceEnergyModel

This should be overloaded to match necessity.

Base case is very similar to UAN's AcousticModemEnergyModel.
*/

namespace ns3 {

class AquaSimNode;

class AquaSimEnergyModel : public DeviceEnergyModel
{
public:
  static TypeId GetTypeId(void);
  AquaSimEnergyModel();
  virtual ~AquaSimEnergyModel();

  virtual void SetNode(Ptr<AquaSimNode> node);
  virtual Ptr<AquaSimNode> GetNode(void) const;

  virtual void ChangeState(int newState);
  virtual double DoGetCurrentA(void) const; //maybe depending if current draw for its states are know or not
  virtual void HandleEnergyDepletion(void);
  virtual void HandleEnergyRecharged(void);

  //inherited
  double GetCurrentA(void) const;
  double GetTotalEnergyConsumption(void) const;
  void SetEnergySource(Ptr<EnergySource> source); //only called by DeviceEnergyModel helper...


  //include callback if energy is <= 0.0 during decreasing... to call energy depleted on Phy using m_node...

  void SetRxPower(double);
  void SetTxPower(double);
  void SetIdlePower(double);
  void SetEnergy(double);
  void SetInitialEnergy(double);

  double GetRxPower(void);
  double GetTxPower(void);
  double GetIdlePower(void);
  double GetEnergy(void);
  void DecrIdleEnergy(double);
  void DecrRcvEnergy(double);
  void DecrTxEnergy(double);
  void DecrEnergy(double, double decrEnergy);  //allow user to specify energy decr value

  /* TODO : Future work needs to incorporate total times in each state */

private:
  double m_energy;
  double m_initialEnergy;
  double m_rxP, m_txP, m_idleP;
  double m_totalEnergyConsumption;	//if energy recharging where incorporated

  Ptr<AquaSimNode> m_node;
  Ptr<EnergySource> m_source;


};  //AquaSimEnergyModel class

}  //namespace ns3

#endif /* AQUA_SIM_ENERGY_MODEL_H */
