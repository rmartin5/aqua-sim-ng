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

#include "ns3/log.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"

#include "aqua-sim-net-device.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (AquaSimNetDevice);

AquaSimNetDevice::AquaSimNetDevice ()
  : NetDevice()
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
    .AddAttribute ("Channel", "The Channel layer attached to this device.",
       PointerValue (),
       MakePointerAccessor (&AquaSimNetDevice::m_channel),
       MakePointerChecker<AquaSimChannel>())
  //.AddAttribute ("App", "The App layer attached to this device.",
    // PointerValue (&AquaSimNetDevie::m_app),
    // MakePointerAccessor (&AquaSimNetDevice::GetApp, &AquaSimNetDevice::SetApp),
    // MakePointerChecker<AquaSimApp>())
  ;     //Added mobility model? routing layer?
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
  m_phy->Initialize ();
  m_mac->Initialize ();
  //m_app->Initialize ();
  //channel?
  NetDevice::DoInitialize ();
}

void
AquaSimNetDevice::CompleteConfig (void)
{
  if (m_mac == 0 || m_phy == 0 || /*m_app == 0 ||*/ m_node == 0 || m_configComplete)
    {
      return;
    }
  //exec
  //TODO set app, phy, ++
  m_configComplete = true;
}

void
AquaSimNetDevice::SetPhy (Ptr<AquaSimPhy> phy)
{
  m_phy = phy;
  CompleteConfig ();
}

void
AquaSimNetDevice::SetMac (Ptr<AquaSimMac> mac)
{
  m_mac = mac;
  CompleteConfig ();
}

void
AquaSimNetDevice::SetChannel (Ptr<AquaSimChannel> channel)
{
  m_channel = channel;
  CompleteConfig ();
}
/*
void
AquaSimNetDevice::SetApp (Ptr<AquaSimApp> app)
{
  m_app = app;
  CompleteConfig ();
}
*/
void
AquaSimNetDevice::SetNode (Ptr<AquaSimNode> node)
{
  m_node = node;
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

Ptr<AquaSimChannel>
AquaSimNetDevice::GetChannel (void)
{
  return m_channel;
}
 
Ptr<AquaSimNode>
AquaSimNetDevice::GetNode (void)
{
  return m_node;
}

/*
AquaSimNetDevice::GetApp (void)
{
  return m_app;
}
*/


}  // namespace ns3
