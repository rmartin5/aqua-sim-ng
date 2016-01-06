/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 UWSN Lab at the University of Connecticut
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

#include "ns3/vector.h"
#include "ns3/log.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/integer.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"

#include "aqua-sim-net-device.h"
#include "aqua-sim-phy.h"
#include "aqua-sim-mac.h"
#include "aqua-sim-routing.h"
#include "aqua-sim-channel.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimNetDevice");
NS_OBJECT_ENSURE_REGISTERED (AquaSimNetDevice);

AquaSimNetDevice::AquaSimNetDevice ()
  : NetDevice(),
    m_nextHop(-10),
    m_setHopStatus(0),
    m_sinkStatus(0),
    m_transStatus(NIDLE),
    m_statusChangeTime(0.0),
    m_failureStatus(false), //added by peng xie
    m_failurePro(0.0), //added by peng xie
    m_failureStatusPro(0.0), //added by peng xie and zheng
    m_cX(0.0),
    m_cY(0.0),
    m_cZ(0.0),
    m_carrierSense(false),
    m_carrierId(false),
    m_ifIndex(0),
    m_mtu(0)
{
  m_configComplete = false;
}

AquaSimNetDevice::~AquaSimNetDevice ()
{
}

TypeId
AquaSimNetDevice::GetTypeId ()
{
  static TypeId tid = TypeId("ns3::AquaSimNetDevice")
    .SetParent<NetDevice>()
    .AddAttribute ("Phy", "The PHY layer attached to this device.",
      PointerValue (),
      MakePointerAccessor (&AquaSimNetDevice::m_phy),
      MakePointerChecker<AquaSimPhy>())
    .AddAttribute ("Mac", "The MAC layer attached to this device.",
      PointerValue (),
      MakePointerAccessor (&AquaSimNetDevice::m_mac),
      MakePointerChecker<AquaSimMac>())
    .AddAttribute ("Routing", "The Routing layer attached to this device.",
      PointerValue (),
      MakePointerAccessor (&AquaSimNetDevice::m_routing),
      MakePointerChecker<AquaSimRouting>())
    .AddAttribute ("Channel", "The Channel layer attached to this device.",
      PointerValue (),
      MakePointerAccessor (&AquaSimNetDevice::m_channel),
      MakePointerChecker<AquaSimChannel>())
  //.AddAttribute ("App", "The App layer attached to this device.",
    //PointerValue (&AquaSimNetDevie::m_app),
    //MakePointerAccessor (&AquaSimNetDevice::GetApp, &AquaSimNetDevice::SetApp),
    //MakePointerChecker<AquaSimApp>())
    .AddAttribute ("Mobility", "The Mobility model attached to this device.",
      PointerValue (),
      MakePointerAccessor (&AquaSimNetDevice::m_mobility),
      MakePointerChecker<MobilityModel>())
  //3 following commands are for VBF related protocols only
    .AddAttribute("SetCx", "Set x for VBF related protocols.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimNetDevice::m_cX),
      MakeDoubleChecker<double>())
   .AddAttribute("SetCy", "Set y for VBF related protocols.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimNetDevice::m_cY),
      MakeDoubleChecker<double>())
   .AddAttribute("SetCz", "Set z for VBF related protocols.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimNetDevice::m_cZ),
      MakeDoubleChecker<double>())
   .AddAttribute("SetFailureStatus", "Set node failure status. Default false.",
     BooleanValue(0),
     MakeBooleanAccessor(&AquaSimNetDevice::m_failureStatus),
     MakeBooleanChecker())
   .AddAttribute("SetFailureStatusPro", "Set node failure status pro.",
     DoubleValue(0),
     MakeDoubleAccessor(&AquaSimNetDevice::m_failureStatusPro),
     MakeDoubleChecker<double>())
   .AddAttribute("SetFailurePro", "Set node failure pro.",
     DoubleValue(0),
     MakeDoubleAccessor(&AquaSimNetDevice::m_failurePro),
     MakeDoubleChecker<double>())
   .AddAttribute("NextHop", "Set next hop. Default is 1.",
     IntegerValue(1),
     MakeIntegerAccessor(&AquaSimNetDevice::m_nextHop),
     MakeIntegerChecker<int>())
   .AddAttribute("SinkStatus", "Set the sink's status, int value.",
     IntegerValue(0),
     MakeIntegerAccessor(&AquaSimNetDevice::m_sinkStatus),
     MakeIntegerChecker<int> ())
  ;
  return tid;
}  

void
AquaSimNetDevice::DoDispose (void)
{
  //TODO clear all variables and link to NetDevice to do the same...
}

void
AquaSimNetDevice::DoInitialize (void)
{
  //m_phy->Initialize ();
  //m_mac->Initialize ();
  //m_app->Initialize ();
  //channel?
  NetDevice::DoInitialize ();
}

void
AquaSimNetDevice::CompleteConfig (void)
{
  if (m_mac == 0 || m_phy == 0 || /*m_app == 0 ||*/ m_node == 0 || m_phy || m_configComplete)
    {
      return;
    }
  //exec
  //TODO set app, ++
  m_configComplete = true;
}

void
AquaSimNetDevice::SetPhy (Ptr<AquaSimPhy> phy)
{
  NS_LOG_FUNCTION(this);
  m_phy = phy;
  CompleteConfig ();
}

void
AquaSimNetDevice::SetMac (Ptr<AquaSimMac> mac)
{
  NS_LOG_FUNCTION(this);
  m_mac = mac;
  CompleteConfig ();
}

void
AquaSimNetDevice::SetRouting(Ptr<AquaSimRouting> routing)
{
  NS_LOG_FUNCTION(this);
  m_routing = routing;
  CompleteConfig ();
}

void
AquaSimNetDevice::SetChannel (Ptr<AquaSimChannel> channel)
{
  NS_LOG_FUNCTION(this);
  m_channel = channel;
  CompleteConfig ();
}
/*
void
AquaSimNetDevice::SetApp (Ptr<AquaSimApp> app)
{
  NS_LOG_FUNCTION(this);
  m_app = app;
  CompleteConfig ();
}
*/
void
AquaSimNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION(this);
  m_node = node;
}

void
AquaSimNetDevice::SetMobility(Ptr<MobilityModel> mobility)
{
  NS_LOG_FUNCTION(this);
  m_mobility = mobility;
}

Ptr<AquaSimPhy>
AquaSimNetDevice::GetPhy (void)
{
  return m_phy;
}

Ptr<AquaSimMac>
AquaSimNetDevice::GetMac (void)
{
  return m_mac;
}

Ptr<AquaSimRouting>
AquaSimNetDevice::GetRouting (void)
{
  return m_routing;
}

Ptr<Channel>
AquaSimNetDevice::GetChannel (void) const
{
  return m_channel;
}
 
Ptr<Node>
AquaSimNetDevice::GetNode (void) const
{
  return m_node;
}

Ptr<MobilityModel>
AquaSimNetDevice::GetMobility (void)
{
  return m_mobility;
}

/*
AquaSimNetDevice::GetApp (void)
{
  return m_app;
}
*/

bool
AquaSimNetDevice::IsMoving(void)
{
  NS_LOG_FUNCTION(this);

  if (m_mobility == NULL){
      return false;
  }

  Vector3D vel = m_mobility->GetVelocity();
  if (vel.x==0 && vel.y==0 && vel.z==0) {
      return false;
  }

  return true;
}

int
AquaSimNetDevice::SetSinkStatus()
{
  m_sinkStatus = 1;
  return 0;
}


int
AquaSimNetDevice::ClearSinkStatus()
{
  m_sinkStatus = 0;
  return 0;
}

void
AquaSimNetDevice::GenerateFailure()
{
  double error_pro = m_uniformRand->GetValue();
  if (error_pro < m_failureStatusPro)
    m_failureStatus = true;
}

void
AquaSimNetDevice::AddLinkChangeCallback (Callback< void > callback)
{
  NS_LOG_WARN("Not implemented");
}

Address
AquaSimNetDevice::GetAddress (void) const
{
  NS_LOG_WARN("Not implemented on mac yet");
  return m_mac->GetAddress();
}

Address
AquaSimNetDevice::GetBroadcast (void) const
{
  NS_LOG_WARN("Not implemented since UW is median is always broadcast (i.e. 255).");
  return Address();
}

uint32_t
AquaSimNetDevice::GetIfIndex (void) const
{
  return m_ifIndex;
}

uint16_t
AquaSimNetDevice::GetMtu (void) const
{
  return m_mtu;
}

Address
AquaSimNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_WARN("Not implemented");
  return Address();
}

Address
AquaSimNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_WARN("Not implemented");
  return Address();
}

bool
AquaSimNetDevice::IsBridge (void) const
{
  return false;
}

bool
AquaSimNetDevice::IsBroadcast (void) const
{
  return true;
}

bool
AquaSimNetDevice::IsLinkUp (void) const
{
  return (m_phy != 0);
}

bool
AquaSimNetDevice::IsMulticast (void) const
{
  return false;
}

bool
AquaSimNetDevice::IsPointToPoint (void) const
{
  return false;
}

bool
AquaSimNetDevice::NeedsArp (void) const
{
  return false;
}

bool
AquaSimNetDevice::Send (Ptr< Packet > packet, const Address &dest, uint16_t protocolNumber)
{
  if(m_routing)
    {
      m_routing->SendDown(packet, dest, Simulator::Now());
      return true;
    }
  if (m_mac)
    {
      m_mac->SendDown(packet);
      return true;
    }
  if (m_phy)
    {
      m_phy->PktTransmit(packet);
      return true;
    }
  NS_LOG_WARN("No layers are attached to this device. Can not send.");
  return false;
}

bool
AquaSimNetDevice::SendFrom (Ptr< Packet > packet, const Address &source, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_WARN("SendFrom not supported");
  return false;
}

void
AquaSimNetDevice::SetAddress (Address address)
{
  m_mac->SetAddress(address);
}

void
AquaSimNetDevice::SetIfIndex (const uint32_t index)
{
  m_ifIndex = index;
}

bool
AquaSimNetDevice::SetMtu (uint16_t mtu)
{
  m_mtu = mtu;
  return true;
}

void
AquaSimNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG_WARN("PromiscRecvCB Not supported");
}

void
AquaSimNetDevice::SetReceiveCallback (ReceiveCallback cb)
{
  NS_LOG_WARN("RecvCallback not implemented");
  m_forwardUp = cb;
}

bool
AquaSimNetDevice::SupportsSendFrom (void) const
{
  return false;
}

}  // namespace ns3
