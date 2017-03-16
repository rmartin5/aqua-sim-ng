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

#include "aqua-sim-pt-tag.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimPtTag");
NS_OBJECT_ENSURE_REGISTERED(AquaSimPtTag);

AquaSimPtTag::AquaSimPtTag ()
{
}

void
AquaSimPtTag::SetPacketType(uint16_t pt)
{
  NS_LOG_DEBUG("PtTag:SetPacketType:" << pt);
  m_packetType = pt;
}
uint16_t
AquaSimPtTag::GetPacketType() const
{
  NS_LOG_FUNCTION(this);
  return m_packetType;
}

TypeId
AquaSimPtTag::GetTypeId()
{
  static TypeId tid = TypeId ("ns3::AquaSimPtTag")
    .SetParent<Tag>()
    .AddConstructor<AquaSimPtTag>()
  ;
  return tid;
}
TypeId
AquaSimPtTag::GetInstanceTypeId () const
{
  return GetTypeId();
}
uint32_t
AquaSimPtTag::GetSerializedSize () const
{
  return 2;
}
void
AquaSimPtTag::Serialize (TagBuffer i) const
{
  i.WriteU16(m_packetType);
}
void
AquaSimPtTag::Deserialize (TagBuffer i)
{
  m_packetType = i.ReadU16();
}
void
AquaSimPtTag::Print (std::ostream &os) const
{
  os << "Aqua Sim packetType=" << (uint16_t)m_packetType;
}

} // namespace ns3
