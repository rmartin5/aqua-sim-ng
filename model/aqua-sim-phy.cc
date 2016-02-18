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
#include "aqua-sim-signal-cache.h"

namespace ns3
{

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

void
AquaSimPhy::AttachPhyToSignalCache(Ptr<AquaSimSignalCache> sC, Ptr<AquaSimPhy> phy)
{
  sC->AttachPhy(phy);
}

void
AquaSimPhy::DoDispose()
{
  NS_LOG_FUNCTION(this);
  Object::Dispose();
}
} //ns3 namespace
