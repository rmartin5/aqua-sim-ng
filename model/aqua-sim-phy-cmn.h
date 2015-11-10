//... header

//FIXME... this needs to be updated to match common case... including source file
#ifndef AQUA_SIM_PHY_CMN_H
#define AQUA_SIM_PHY_CMN_H


#include <string>
#include <list>
#include <map>
#include <vector>

#include "ns3/nstime.h"
#include "ns3/timer.h"
#include "ns3/event-id.h"
#include "ns3/packet.h"
#include "ns3/object.h"

#include "aqua-sim-signal-cache.h"
#include "aqua-sim-modulation.h"
#include "aqua-sim-energy-model.h"
#include "aqua-sim-node.h"
#include "aqua-sim-sinr-checker.h"
#include "aqua-sim-channel.h"
#include "aqua-sim-mac.h"

//Aqua Sim Phy

namespace ns3 {

class AquaSimPhy;
class AquaSimSinrChecker;
class AquaSimSignalCache;
class AquaSimModulation;
class AquaSimNode;
class AquaSimChannel;
class AquaSimMac;
class Packet;
class AquaSimEnergyModel;

class AquaSimIdleTimer : public Timer
{
public:
  AquaSimIdleTimer(AquaSimPhy* a) : Timer() { m_a = a; }
  void Expire(void);
	  //this is calling energy reduction function using phy variable
	  // should probably be a callback instead...
private:
  AquaSimPhy* m_a;
  //Ptr<EventImpl> m_e;  //would probably also need to use PeekEventImpl()
};  //  class AquaSimIdleTimer

class AquaSimPhyCmn : public AquaSimPhy
{
public:
  AquaSimPhyCmn(void);
  virtual ~AquaSimPhyCmn(void);
  static TypeId GetTypeId(void);

  virtual void SetTxPower(double ptConsume);
  virtual void SetRxPower(double prConsume);
  virtual void SetIdlePower(double pIdle);

  virtual void SetNode(Ptr<AquaSimNode> node);
  virtual void SetSinrChecker(Ptr<AquaSimSinrChecker> sinrChecker);
  virtual void SetSignalCache(Ptr<AquaSimSignalCache> sC);
  virtual void AddModulation(Ptr<AquaSimModulation> modulation, std::string modulationName);

  virtual void Dump(void) const;
  virtual bool Decodable(double noise, double ps);
  void SignalCacheCallback(Ptr<Packet> p);

  void SendPktDown(Ptr<Packet> p); //should be PktTransmit instead
  void SendPktUp(Ptr<Packet> p);

  /*
  inline int Initialized(void) {
	  return (Trigger() <= Preamble() && Node()
			  && m_mac != NULL && m_channel != NULL
			  && m_modulations.size()>0
			  && m_sC != NULL && m_sinrChecker != NULL
			  );
  }
  */

  void UpdateIdleEnergy(void);


  inline Ptr<AquaSimNode> Node(void) const { return m_node; }	//redundant
  //inline Ptr<AquaSimEnergyModel> EM(void) { return Node()->EnergyModel(); }
  /*
   * will not have to place some of the basic functions, such as above, within child class.
   */

  void PowerOn();
  void PowerOff();
  void StatusShift(double);

  inline double GetPt() { return m_pT; }
  inline double GetRXThresh() { return m_RXThresh; }
  inline double GetCSThresh() { return m_CSThresh; }

  Ptr<AquaSimModulation> Modulation(Ptr<std::string> modName = NULL); //TODO modulation should be updated.

  virtual void Recv(Ptr<Packet> p);

  inline double Trigger(void) { return m_trigger; }
  inline double Preamble(void) { return m_preamble; }

  inline double GetEnergySpread(void){ return m_K; }
  inline double GetFrequency(){ return m_freq; }
  inline bool MatchFreq(double freq);
  inline double GetL() const { return m_L; }
  inline double GetLambda() { return m_lambda; }

  inline PhyStatus &Status() {return m_status;}

protected:
  virtual Ptr<Packet> PrevalidateIncomingPkt(Ptr<Packet> p);
  virtual void UpdateTxEnergy(Time txTime, double pT, double pIdle);
  virtual void UpdateRxEnergy(Time txTime);
  virtual Ptr<Packet> StampTxInfo(Ptr<Packet> p);
  void EnergyDeplete(void);

  //TODO energy model could substitute this and better define it all.
  double m_pT;		// transmitted signal power (W)
  double m_ptConsume;	// power consumption for transmission (W)
  double m_prConsume;	// power consumption for reception (W)
  double m_pIdle;         // idle power consumption (W)
  double m_updateEnergyTime;	// the last time we update energy.

  double m_RXThresh;	// receive power threshold (W)
  double m_CSThresh;	// carrier sense threshold (W)
  double m_CPThresh;	// capture threshold (db)

  double m_K;	// energy spread factor
  double m_freq;  // frequency
  double m_L;	// system loss default factor
  double m_lambda;  // wavelength (m), we don't use it anymore

  PhyStatus m_status;	// status of modem

  // preamble and trigger are fixed for a given modem (can be updated in future)
  double m_preamble;
  double m_trigger;

  /**
  * MUST make sure Pt and tx_range are consistent at the physical layer!!!
  * so set transmission range according to PowerLevels_
  */
  //level of transmission power in an increasing order
  std::vector<double> m_powerLevels;
  int m_ptLevel;

  /**
  * Modulation Schemes. a modem can support multiple modulation schemes
  * map modulation's name to the object
  */
  std::map<const std::string, Ptr<AquaSimModulation> > m_modulations;
  std::string m_modulationName;	//the name of current modulation

  /**
  * cache the incoming signal from channel. it calculates SINR
  * and check collisions.
  */
  Ptr<AquaSimSignalCache> m_sC;
  Ptr<AquaSimSinrChecker> m_sinrChecker;

  AquaSimIdleTimer m_idleTimer;

  double m_EnergyTurnOn;	//energy consumption for turning on the modem (J)
  double m_EnergyTurnOff; //energy consumption for turning off the modem (J)

  //Ptr<AquaSimAntenna> m_ant; // we don't use it anymore, however we need it as an arguments
  /**
  * points to the same object as node_ in class MobileNode
  * but n_ is of type UnderwaterSensorNode * instead of MobileNode *
  */
  Ptr<AquaSimNode> m_node;
  Ptr<AquaSimEnergyModel> m_eM;

  bool m_PoweredOn;  //true: power on false:power off

  friend class AquaSimIdleTimer;

private:
  Ptr<AquaSimChannel> m_channel;
  Ptr<AquaSimMac> m_mac;

  void Expire(void);

}; //AquaSimPhyCmn

} //namespace ns3

#endif /* AQUA_SIM_PHY_CMN_H */
