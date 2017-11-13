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
#ifndef AQUA_SIM_PHY_H
#define AQUA_SIM_PHY_H

#include "ns3/object.h"
//#include "ns3/packet.h"

#include <string>

#include "aqua-sim-net-device.h"
#include "aqua-sim-channel.h"
#include "ns3/traced-callback.h"
//#include "aqua-sim-sinr-checker.h"
//#include "aqua-sim-signal-cache.h"
//#include "aqua-sim-modulation.h"

/*
 * Baseclass for Aqua-Sim Phy
 *
 */
namespace ns3 {

   //Using net device as device status singleton
  /*enum PhyStatus {
  	PHY_RECV,
  	PHY_SEND,
  	PHY_IDLE,
  	PHY_SLEEP,
  	PHY_TRIG,
  	PHY_NONE,
  	PHY_DISABLE
  	};*/

  class AquaSimNetDevice;
  class AquaSimChannel;
  class AquaSimMac;
  class AquaSimEnergyModel;
  class AquaSimSinrChecker;
  class AquaSimSignalCache;
  class AquaSimModulation;
  class Packet;
  class Time;

  /**
   * \ingroup aqua-sim-ng
   *
   * \brief Base class for Phy layer.
   *
   * Overall modem status can be found under AquaSimNetDevice
   */
  class AquaSimPhy : public Object
  {
  public:
    AquaSimPhy();
    static TypeId GetTypeId();

    virtual void SetTxPower(double ptConsume) = 0;
    virtual void SetRxPower(double prConsume) = 0;
    virtual void SetIdlePower(double pIdle) = 0;

    void SetNetDevice(Ptr<AquaSimNetDevice> device);
    void SetChannel(std::vector<Ptr<AquaSimChannel> > channel);
    virtual void SetSinrChecker(Ptr<AquaSimSinrChecker> sinrChecker) = 0;
    virtual void SetSignalCache(Ptr<AquaSimSignalCache> sC) = 0;
    virtual void AddModulation(Ptr<AquaSimModulation> modulation, std::string modulationName) = 0;
    Ptr<AquaSimNetDevice> GetNetDevice ();
    Ptr<AquaSimMac> GetMac();
    Ptr<AquaSimEnergyModel> EM();

    virtual void Dump() const = 0;
    virtual bool Decodable (double noise, double ps) = 0;
    virtual void SendPktUp(Ptr<Packet> p) = 0;
    virtual bool PktTransmit(Ptr<Packet> p, int channelId) = 0;
    //virtual void PktTransmit(Ptr<Packet> p, Ptr<AquaSimPhy> src) = 0;

    virtual void UpdateIdleEnergy() = 0;
    /***************
    * could this just be handled by energy model instead??? AquaSimEnergyModel()
    **************/

    virtual void PowerOn() = 0;
    virtual void PowerOff() = 0;
    virtual void StatusShift(double x) = 0; //Necessary?????
    virtual bool IsPoweredOn()=0;

    virtual Time CalcTxTime(uint32_t pktsize, std::string * modName = NULL) = 0;
    virtual double CalcPktSize(double txtime, std::string * modName = NULL) = 0;

    virtual void SignalCacheCallback(Ptr<Packet> p) = 0;
    virtual bool Recv(Ptr<Packet> p) = 0;

    virtual double Trigger() = 0;
    virtual double Preamble() = 0;

    //inline PhyStatus & Status() {return m_status;}
    //void SetPhyStatus(PhyStatus status);

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

    virtual int PktRecvCount() = 0; //debugging

    /*
    * Used for some mac/routing protocols and for restricting packet range within range-propagation for channel module.
    */
    virtual void SetTransRange(double range)=0;
    virtual double GetTransRange()=0;

    virtual int64_t AssignStreams (int64_t stream) = 0;
    typedef void (* TracedCallback) (Ptr<Packet> pkt, double noise);
    typedef void (* RxErrorCallback) (std::string path, Ptr<const Packet> pkt);
    void NotifyTx(Ptr<Packet> packet);
    void NotifyRx(Ptr<Packet> packet);

  protected:
    virtual Ptr<Packet> PrevalidateIncomingPkt(Ptr<Packet> p) = 0;
    virtual void UpdateTxEnergy(Time txTime, double pT, double pIdle) = 0;
    virtual void UpdateRxEnergy(Time txTime, bool errorFlag) = 0;
    virtual Ptr<Packet> StampTxInfo(Ptr<Packet> p) = 0;
    virtual void EnergyDeplete() = 0;

    void AttachPhyToSignalCache(Ptr<AquaSimSignalCache> sC, Ptr<AquaSimPhy> phy);

    virtual void DoDispose();

    std::vector<Ptr<AquaSimChannel> > m_channel;  //for multi-channel support
    Ptr<AquaSimNetDevice> m_device;

    friend class AquaSimEnergyModel;
    friend class AquaSimNetDevice;  //slightly dangerous but currrently used to remove reference cycle on disposal.

    ns3::TracedCallback<Ptr<const Packet> > m_phyRxErrorTrace;

    //PhyStatus m_status;	// status of modem
  private:
      ns3::TracedCallback<Ptr<Packet> > m_phyTxTrace;
      ns3::TracedCallback<Ptr<Packet> > m_phyRxTrace;

  }; //AquaSimPhy class

} //ns3 namespace

#endif /* AQUA_SIM_PHY_H */
