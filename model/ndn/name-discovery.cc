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

#include "ns3/log.h"
#include "ns3/packet.h"
#include "name-discovery.h"
#include "ns3/aqua-sim-header.h"
#include "ns3/aqua-sim-header-mac.h"
#include "named-data-header.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NameDiscovery");
NS_OBJECT_ENSURE_REGISTERED (NameDiscovery);

TypeId
NameDiscovery::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NameDiscovery")
    ;
  return tid;
}

NameDiscovery::NameDiscovery()
{
}

std::pair<uint8_t*,AquaSimAddress>
NameDiscovery::ProcessNameDiscovery(Ptr<Packet> packet)
{
  AquaSimHeader ash; MacHeader mach; NamedDataHeader ndh;
  packet->RemoveAtStart(ndh.GetSerializedSize() + mach.GetSerializedSize() + ash.GetSerializedSize());
  uint32_t size = packet->GetSize ();
  uint8_t *data = new uint8_t[size];
  packet->CopyData (data, size);
  packet->AddHeader(ndh); packet->AddHeader(mach); packet->AddHeader(ash);

  return std::make_pair(data, ash.GetSAddr());
}

void
NameDiscovery::ShortenNamePrefix(uint8_t* name, char delim)
{
  char *lastPos = strrchr(reinterpret_cast<char*>(name),delim);
  if(lastPos != NULL) {
    *lastPos = '\0';
    //return lasPos+1; will return removed part of string
    return;
  }
  NS_LOG_WARN(this << "Delim:" << delim << " was not found within name:" << name);
}
