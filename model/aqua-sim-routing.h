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

#ifndef AQUA_SIM_ROUTING_H
#define AQUA_SIM_ROUTING_H

#include "ns3/object.h"
#include "ns3/address.h"
#include "ns3/nstime.h"
#include "ns3/traced-value.h"
#include "ns3/packet.h"
//#include "ns3/ipv4.h"
//#include "ns3/ipv4-routing-protocol.h"
//#include "ns3/ipv4-static-routing.h"

#include "aqua-sim-address.h"
//#include "aqua-sim-mac.h"
#include "aqua-sim-net-device.h"

namespace ns3 {

//class Packet;
//class AquaSimNetDevice;
class AquaSimMac;

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Base class for underwater routing. Consists of basic routing components, setters/getters, and send up/down to other layers.
 */
class AquaSimRouting : public Object
{
public:
/*define common commands for underwater routing protocols*/
  static TypeId GetTypeId(void);
  AquaSimRouting(void);
  virtual ~AquaSimRouting(void);

  virtual void SetNetDevice(Ptr<AquaSimNetDevice> device);
  virtual void SetMac(Ptr<AquaSimMac> mac);

  /*avoid instantiation since UnderwaterRouting's behavior is not defined*/
  virtual bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)=0;	//handler not implemented
  /*send packet p to next_hop after delay*/
  virtual bool SendDown(Ptr<Packet> p, AquaSimAddress nextHop, Time delay);

  int SendUpPktCount() {return m_sendUpPktCount;}
  int TrafficInPkts() {return trafficPktsTrace.Get();}
  int TrafficInBytes() {return trafficBytesTrace.Get();}
  //int TrafficInBytes(bool trafficBytesTrace);

  virtual void AssignInternalData(std::vector<std::string> collection);
  virtual void AssignInternalDataPath(std::vector<std::string> collection);

  virtual int64_t AssignStreams (int64_t stream) = 0;

protected:
  /*send packet up to port-demux*/
  virtual bool SendUp(Ptr<Packet> p);			//demux not implemented yet.
  /*check if if a dead loop results in the incoming packet*/
  virtual bool IsDeadLoop(Ptr<Packet> p);
  /*check if this node is the next hop*/
  virtual bool AmINextHop(const Ptr<Packet> p);
  /*check if this node is the destination.*/
  virtual bool AmIDst(const Ptr<Packet> p);
  /*check if this node is the source node,
          * i.e., whose app layer generates this packet.*/
  virtual bool AmISrc(const Ptr<Packet> p);
  virtual void SendPacket(Ptr<Packet> p);

  virtual Ptr<AquaSimNetDevice> GetNetDevice();
  virtual Ptr<AquaSimMac> GetMac();

  typedef void (* RxCallback)(std::string path, Ptr<Packet> p);
  typedef void (* TxCallback)(std::string path, Ptr<Packet> p, AquaSimAddress nextHop, Time delay);

  typedef void (* PacketReceivedCallback)(std::string path, Ptr<Packet> p);
  typedef void (* PacketTransmittingCallback)(std::string path, Ptr<Packet> p, AquaSimAddress nextHop, AquaSimAddress dest);


  void NotifyRx(std::string path, Ptr<Packet> p);
  void NotifyTx(std::string path, Ptr<Packet> p, AquaSimAddress nextHop, Time delay);

  virtual void DoDispose();
protected:
  //AquaSimAddress m_myAddr;  //the ip address of this node
  Ptr<AquaSimNetDevice> m_device;
  //NsObject *ll;			//pointer to link layer object
  //NsObject *port_dmux;

  //DDoS Routing usage:
  std::vector<std::string> m_data;
  std::vector<std::string> m_knownDataPath;   //low key FIB, known path for certain data name.

  TracedValue<uint32_t> trafficPktsTrace;
  TracedValue<uint32_t> trafficBytesTrace;

private:
  Ptr<AquaSimMac> m_mac;

  TracedValue<Ptr<const Packet> > m_routingRxTrace;
  TracedValue<Ptr<const Packet> > m_routingTxTrace;

  TracedCallback<Ptr<const Packet> > m_routingRxCbTrace;
  TracedCallback<Ptr<const Packet>, AquaSimAddress, AquaSimAddress > m_routingTxCbTrace;

  int m_sendUpPktCount;

};  //AquaSimRouting class

}  //namespace ns3

#endif /* AQUA_SIM_ROUTING_H */
