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

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(AquaSimPtTag);

AquaSimPtTag::AquaSimPtTag ()
{
}

void
AquaSimPtTag::SetPacketType(uint8_t pt)
{
  m_packetType = pt;
}
uint8_t
AquaSimPtTag::GetPacketType() const
{
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
  return 1;
}
void
AquaSimPtTag::Serialize (TagBuffer i) const
{
  i.WriteU8(m_packetType);
}
void
AquaSimPtTag::Deserialize (TagBuffer i)
{
  m_packetType = i.ReadU8();
}
void
AquaSimPtTag::Print (std::ostream &os) const
{
  os << "Aqua Sim packetType=" << (uint32_t)m_packetType;
}

} // namespace ns3
