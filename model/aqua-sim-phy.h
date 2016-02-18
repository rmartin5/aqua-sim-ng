/*
 * aqua-sim-phy.h
 *
 *  Created on: Nov 5, 2015
 *      Author: robert
 */

#ifndef AQUA_SIM_PHY_H
#define AQUA_SIM_PHY_H

#include "ns3/object.h"
//#include "ns3/packet.h"

#include <string>

//#include "aqua-sim-net-device.h"
//#include "aqua-sim-sinr-checker.h"
//#include "aqua-sim-signal-cache.h"
//#include "aqua-sim-modulation.h"

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
  class AquaSimChannel;
  class AquaSimMac;
  class AquaSimSinrChecker;
  class AquaSimSignalCache;
  class AquaSimModulation;
  class Packet;
  class Time;

  class AquaSimPhy : public Object
  {
  public:
    static TypeId GetTypeId();

    virtual void SetTxPower(double ptConsume) = 0;
    virtual void SetRxPower(double prConsume) = 0;
    virtual void SetIdlePower(double pIdle) = 0;

    virtual void SetNetDevice(Ptr<AquaSimNetDevice> device) = 0;
    virtual void SetChannel(Ptr<AquaSimChannel> channel) = 0;
    virtual void SetMac(Ptr<AquaSimMac> mac) = 0;
    virtual void SetSinrChecker(Ptr<AquaSimSinrChecker> sinrChecker) = 0;
    virtual void SetSignalCache(Ptr<AquaSimSignalCache> sC) = 0;
    virtual void AddModulation(Ptr<AquaSimModulation> modulation, std::string modulationName) = 0;
    virtual Ptr<AquaSimNetDevice> GetNetDevice () = 0;

    virtual void Dump() const = 0;
    virtual bool Decodable (double noise, double ps) = 0;
    virtual void SendPktUp(Ptr<Packet> p) = 0;
    virtual bool PktTransmit(Ptr<Packet> p) = 0;
    //virtual void PktTransmit(Ptr<Packet> p, Ptr<AquaSimPhy> src) = 0;

    virtual void UpdateIdleEnergy() = 0;
    /***************
     * could this just be handled by energy model instead??? AquaSimEnergyModel()
     **************/

    virtual void PowerOn() = 0;
    virtual void PowerOff() = 0;
    virtual void StatusShift(double x) = 0; //Necessary?????

    virtual Time CalcTxTime(uint32_t pktsize, std::string * modName = NULL) = 0;
    virtual double CalcPktSize(double txtime, std::string * modName = NULL) = 0;

    virtual void SignalCacheCallback(Ptr<Packet> p) = 0;
    virtual bool Recv(Ptr<Packet> p) = 0;

    virtual double Trigger() = 0;
    virtual double Preamble() = 0;
    virtual PhyStatus &Status() = 0;
    virtual void SetPhyStatus(PhyStatus status) = 0;

    virtual Ptr<AquaSimSignalCache> GetSignalCache() = 0;


    virtual double GetPt() = 0;
    virtual double GetRXThresh() = 0;
    virtual double GetCSThresh() = 0;

    virtual Ptr<AquaSimModulation> Modulation(std::string * modName) = 0;

    virtual double GetEnergySpread(void) = 0;
    virtual double GetFrequency() = 0;
    virtual bool MatchFreq(double freq) = 0;
    virtual double GetL() const = 0;
    virtual double GetLambda() = 0;

  protected:
    virtual Ptr<Packet> PrevalidateIncomingPkt(Ptr<Packet> p) = 0;
    virtual void UpdateTxEnergy(Time txTime, double pT, double pIdle) = 0;
    virtual void UpdateRxEnergy(Time txTime) = 0;
    virtual Ptr<Packet> StampTxInfo(Ptr<Packet> p) = 0;
    virtual void EnergyDeplete() = 0;

    void AttachPhyToSignalCache(Ptr<AquaSimSignalCache> sC, Ptr<AquaSimPhy> phy);


    virtual void DoDispose();
    virtual void Expire(void) = 0;

    friend class AquaSimEnergyModel;

  }; //AquaSimPhy class

} //ns3 namespace

#endif /* AQUA_SIM_PHY_H */
