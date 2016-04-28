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

#ifndef AQUA_SIM_NET_DEVICE_H
#define AQUA_SIM_NET_DEVICE_H

namespace ns3 {
  // early declare for aqua-sim-mac sake
  enum TransStatus {SLEEP, NIDLE, SEND, RECV, NSTATUS, DISABLE };
}

#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"
#include "ns3/callback.h"
#include "ns3/packet.h"
//#include "ns3/address.h"      //could be updated to support own unique address type for uwsn

#include "aqua-sim-address.h"
#include "aqua-sim-phy.h"
#include "aqua-sim-energy-model.h"
//#include "aqua-sim-channel.h"
#include "aqua-sim-routing.h"
#include "aqua-sim-mac.h"
#include "aqua-sim-synchronization.h"


namespace ns3 {


/**
 * \Underwater net device structure.
 *
 * A basic underwater net device structure. Ported from UWSN Lab's Aqua-Sim on NS2.
 */

class Channel;
class PromiscReceiveCallback;
class MobilityModel;

class AquaSimPhy;
class AquaSimRouting;
//class AquaSimMac;
class AquaSimChannel;
class AquaSimSync;

class AquaSimNetDevice : public NetDevice
{
public:
  AquaSimNetDevice ();
  ~AquaSimNetDevice ();
  static TypeId GetTypeId (void);

  //attach
  void ConnectLayers(void);
  void SetPhy (Ptr<AquaSimPhy> phy);
  void SetMac (Ptr<AquaSimMac> mac, Ptr<AquaSimSync> sync = NULL);
  void SetRouting (Ptr<AquaSimRouting> routing);
  void SetChannel (Ptr<AquaSimChannel> channel);
  //void SetApp (Ptr<AquaSimApp> app);
  void SetEnergyModel (Ptr<AquaSimEnergyModel> energyModel);
  double TransmitDistance();  //should be static
  void SetTransmitDistance(double range);

  Ptr<AquaSimPhy> GetPhy (void);
  Ptr<AquaSimMac> GetMac (void);
  Ptr<AquaSimRouting> GetRouting (void);
  //Ptr<AquaSimApp> GetApp (void);
        //Not currently implemented
  Ptr<AquaSimChannel> DoGetChannel(void) const;
  Ptr<AquaSimSync> GetMacSync(void);

  virtual void DoDispose (void);
  virtual void DoInitialize (void);

  void ForwardUp (Ptr<Packet> packet, Ptr<MobilityModel> src, Ptr<MobilityModel> dst);	//not used.

  //inherited functions from NetDevice class
  virtual void AddLinkChangeCallback (Callback<void> callback);
  virtual Address GetAddress (void) const;
  virtual Address GetBroadcast (void) const;
  virtual Ptr<Channel> GetChannel (void) const;
  virtual uint32_t GetIfIndex (void) const;
  virtual uint16_t GetMtu (void) const;
  virtual Address GetMulticast (Ipv4Address multicastGroup) const;
  virtual Address GetMulticast (Ipv6Address addr) const;
  virtual Ptr<Node> GetNode (void) const;
  virtual bool IsBridge (void) const;
  virtual bool IsBroadcast (void) const;
  virtual bool IsLinkUp (void) const;
  virtual bool IsMulticast (void) const;
  virtual bool IsPointToPoint (void) const;
  virtual bool NeedsArp (void) const;
  virtual bool Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
  virtual bool SendFrom (Ptr<Packet> packet, const Address &source,
                           const Address &dest, uint16_t protocolNumber);
  virtual void SetAddress (Address address);
  virtual void SetIfIndex (const uint32_t index);
  virtual bool SetMtu (const uint16_t mtu);
  virtual void SetNode (Ptr<Node> node);
  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb);
  virtual void SetReceiveCallback (ReceiveCallback cb);
  virtual bool SupportsSendFrom (void) const;

  /*
   * Taken from AquaSimNode during consolidation
   */
  //bool Move(void);	/*start the movement... should be handled within example*/
  //void Start(void);
  //void CheckPosition(void);
  inline Time &PositionUpdateTime(void) { return m_positionUpdateTime; }
  void SetPositionUpdateTime(Time posUpdateTime) { m_positionUpdateTime = posUpdateTime; }

  //sink related attributes
  int ClearSinkStatus(void);
  int SetSinkStatus(void);
  inline int GetSinkStatus(void) { return m_sinkStatus; }

  //VBF
  inline double &CX(void) { return m_cX; }
  inline double &CY(void) { return m_cY; }
  inline double &CZ(void) { return m_cZ; }

  inline bool FailureStatus(void) { return m_failureStatus; }
  inline double FailurePro(void) { return m_failurePro; }
  inline double FailureStatusPro(void) { return m_failureStatusPro; }

  void SetTransmissionStatus(TransStatus status);
  TransStatus GetTransmissionStatus(void);

  inline bool CarrierSense(void) { return m_carrierSense; }
  inline void ResetCarrierSense(void) { m_carrierSense = false; }
  inline void SetCarrierSense(bool f){
    m_carrierSense = f;
    m_carrierId = f;
  }
  inline bool CarrierId(void) { return m_carrierId; }
  inline void ResetCarrierId(void) { m_carrierId = false; }

  int m_nextHop;
  int m_setHopStatus;
  int m_sinkStatus;

  int GetHopStatus();
  int GetNextHop();

  //void UpdatePosition(void);  // UpdatePosition() out of date... should be using ns3's mobility module
  bool IsMoving(void);
  Ptr<AquaSimEnergyModel> EnergyModel(void) {return m_energyModel; }

  int TotalSentPkts() {return m_totalSentPkts;}
protected:

  void GenerateFailure(void);

private:

  void CompleteConfig (void);

  Ptr<AquaSimPhy> m_phy;
  Ptr<AquaSimMac> m_mac;
  Ptr<AquaSimRouting> m_routing;
  //Ptr<AquaSimApp> m_app;
  Ptr<AquaSimChannel> m_channel;
  Ptr<Node> m_node;
  Ptr<UniformRandomVariable> m_uniformRand;
  Ptr<AquaSimEnergyModel> m_energyModel;
  Ptr<AquaSimSync> m_macSync;

  NetDevice::ReceiveCallback m_forwardUp;
  bool m_configComplete;

  //m_clear for dispose?? to clear all layers from net-device side.

  /*
   * From AquaSimNode
   */
  TransStatus m_transStatus;
  double m_statusChangeTime;  //the time when changing m_preTransStatus to m_transStatus

  bool	m_failureStatus;// 1 if node fails, 0 otherwise
  double m_failurePro;
  double m_failureStatusPro;

  //the following attributes are added by Peng Xie for RMAC and VBF
  double m_cX;
  double m_cY;
  double m_cZ;

  bool m_carrierSense;
  bool m_carrierId;

  Time m_positionUpdateTime;

  uint32_t m_ifIndex;
  uint16_t m_mtu;

  double m_distCST;

  int m_totalSentPkts;
  //XXX remove counters
};  // class AquaSimNetDevice


}  // namespace ns3

#endif /* AQUA_SIM_NET_DEVICE_H */
