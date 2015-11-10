/*
 * aqua-sim-phy.h
 *
 *  Created on: Nov 5, 2015
 *      Author: robert
 */

#ifndef AQUA_SIM_PHY_H_
#define AQUA_SIM_PHY_H_

#include "ns3/object.h"
#include "ns3/packet.h"

#include <string>

#include "aqua-sim-net-device.h"
#include "aqua-sim-sinr-checker.h"
#include "aqua-sim-signal-cache.h"
#include "aqua-sim-modulation.h"

/*
 * Baseclass for Aqua-Sim Phy
 *
 */
namespace ns3 {

  enum PhyStatus {
  	PHY_RECV,
  	PHY_SEND,
  	PHY_IDLE,
  	PHY_SLEEP,
  	PHY_TRIG,
  	PHY_NONE,
  	PHY_DISABLE
  	};

  class AquaSimNetDevice;
  class AquaSimSinrChecker;
  class AquaSimSignalCache;
  class AquaSimModulation;
  class Packet;

  class AquaSimPhy : public Object
  {
  public:
    static TypeId GetTypeId();

    virtual void SetTxPower(double ptConsume) = 0;
    virtual void SetRxPower(double prConsume) = 0;
    virtual void SetIdlePower(double pIdle) = 0;
    virtual void SetASNetDevice(Ptr<AquaSimNetDevice> device) = 0; //node can get set/get from netdevice
    virtual void SetSinrChecker(Ptr<AquaSimSinrChecker> sinrChecker) = 0;
    virtual void SetSignalCache(Ptr<AquaSimSignalCache> sC) = 0;
    virtual void AddModulation(Ptr<AquaSimModulation> modulation, std::string modulationName) = 0;

    virtual bool Decodable (double noise, double ps) = 0;
    virtual void SendPktUp(Ptr<Packet> p) = 0;
    virtual void PktTransmit(Ptr<Packet> p, Ptr<AquaSimPhy> src) = 0;

    virtual void UpdateIdleEnergy() = 0;
    /***************
     * could this just be handled by energy model instead??? AquaSimEnergyModel()
     **************/

    virtual void PowerOn() = 0;
    virtual void PowerOff() = 0;
    virtual void StatusShift(double x) = 0; //Necessary?????

    inline Time CalcTxTime(int pktsize, Ptr<std::string> modName = NULL);
    inline int CalcPktSize(double txtime, Ptr<std::string> modName = NULL);

    virtual Ptr<AquaSimNode> Node() const = 0;

  protected:
    virtual Ptr<Packet> PrevalidateIncomingPkt(Ptr<Packet> p) = 0;
    virtual void UpdateTxEnergy(Time txTime, double pT, double pIdle) = 0;
    virtual void UpdateRxEnergy(Time txTime) = 0;
    virtual Ptr<Packet> StampTxInfo(Ptr<Packet> p) = 0;
    virtual void EnergyDeplete() = 0;

  }; //AquaSimPhy class

} //ns3 namespace

#endif /* AQUA_SIM_PHY_H_ */
