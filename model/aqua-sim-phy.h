/*
 * aqua-sim-phy.h
 *
 *  Created on: Nov 5, 2015
 *      Author: robert
 */

#ifndef AQUA_SIM_PHY_H_
#define AQUA_SIM_PHY_H_

#include "ns3/object.h"

// TODO create a base class for this...

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

  class AquaSimPhy : public Object
  {
    AquaSimPhy();
    virtual ~AquaSimPhy();
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
     * could this just be handled by energy model instead???
     **************/

    inline double GetEnergySpread(void){ return m_K; }
    inline double GetFrequency(){ return m_freq; }
    inline bool MatchFreq(double freq);
    inline double GetL() const { return m_L; }
    inline double GetLambda() { return m_lambda; }

    inline PhyStatus &Status() {return m_status;}
    virtual void PowerOn() = 0;
    virtual void PowerOff() = 0;
    virtual void StatusShift(double x) = 0; //Necessary?????

  }; //AquaSimPhy class

} //ns3


#endif /* SRC_AQUA_SIM_NG_MODEL_AQUA_SIM_PHY_H_ */
