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

#include "ns3/nstime.h"
#include "ns3/log.h"

#include "aqua-sim-phy.h"
#include "aqua-sim-net-device.h"
#include "aqua-sim-signal-cache.h"
#include "aqua-sim-mac.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimPhy");
NS_OBJECT_ENSURE_REGISTERED(AquaSimPhy);

TypeId
AquaSimPhy::GetTypeId ()
{
  static TypeId tid = TypeId("ns3::AquaSimPhy")
    .SetParent<Object>()
  ;
  return tid;
}

AquaSimPhy::AquaSimPhy()
{
}

void
AquaSimPhy::AttachPhyToSignalCache(Ptr<AquaSimSignalCache> sC, Ptr<AquaSimPhy> phy)
{
  sC->AttachPhy(phy);
}

void
AquaSimPhy::DoDispose()
{
  NS_LOG_FUNCTION(this);
  m_device=0;
  for (std::vector<Ptr<AquaSimChannel> >::iterator it = m_channel.begin(); it != m_channel.end(); ++it)
    *it=0;
}

Ptr<AquaSimNetDevice>
AquaSimPhy::GetNetDevice()
{
  return m_device;
}

Ptr<AquaSimMac>
AquaSimPhy::GetMac()
{
  return m_device->GetMac();
}

Ptr<AquaSimEnergyModel>
AquaSimPhy::EM()
{
  return m_device->EnergyModel();
}


void
AquaSimPhy::SetNetDevice(Ptr<AquaSimNetDevice> device)
{
  NS_LOG_FUNCTION(this);
  m_device = device;
}

void
AquaSimPhy::SetChannel(std::vector<Ptr<AquaSimChannel> > channel)
{
  NS_LOG_FUNCTION(this);
  m_channel = channel;
}

void
AquaSimPhy::NotifyTx(Ptr<Packet> packet)
{
  m_phyTxTrace(packet);
}

void
AquaSimPhy::NotifyRx(Ptr<Packet> packet)
{
  m_phyRxTrace(packet);
}
