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
//#include "ns3/ipv4.h"
//#include "ns3/ipv4-routing-protocol.h"
//#include "ns3/ipv4-static-routing.h"

#include "aqua-sim-address.h"
#include "aqua-sim-mac.h"
#include "aqua-sim-net-device.h"

namespace ns3 {

class Packet;

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
  virtual bool Recv(Ptr<Packet> p)=0;	//handler not implemented
  /*send packet p to next_hop after delay*/
  virtual bool SendDown(Ptr<Packet> p, AquaSimAddress nextHop, Time delay);
  virtual void SetMyAddr(AquaSimAddress myAddr);
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
protected:
  AquaSimAddress m_myAddr;  //the ip address of this node
  Ptr<AquaSimNetDevice> m_device;
  //Ptr<Trace> m_traceTarget;       // Trace Target	TODO need to initiate tracing
  //NsObject *ll;			//pointer to link layer object
  //NsObject *port_dmux;

private:
  Ptr<AquaSimMac> m_mac;

};  //AquaSimRouting class

}  //namespace ns3

#endif /* AQUA_SIM_ROUTING_H */
