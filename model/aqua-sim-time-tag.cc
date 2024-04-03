/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 */

#include "aqua-sim-time-tag.h"

namespace ns3 {

TypeId
AquaSimTimeTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AquaSimTimeTag")
    .SetParent<Tag> ()
    // .AddConstructor<AquaSimTimeTag> ()
  ;
  return tid;
}

TypeId
AquaSimTimeTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
AquaSimTimeTag::GetSerializedSize (void) const
{
  return 8; // uint64_t
}

void
AquaSimTimeTag::Serialize (TagBuffer i) const
{
  i.WriteU64 (m_time);
}

void
AquaSimTimeTag::Deserialize (TagBuffer i)
{
  m_time = i.ReadU64 ();
}

void
AquaSimTimeTag::Print (std::ostream &os) const
{
  os << m_time;
}

AquaSimTimeTag::AquaSimTimeTag ()
{
}

void
AquaSimTimeTag::SetTime (Time time)
{
  m_time = time.GetNanoSeconds();
}

Time
AquaSimTimeTag::GetTime ()
{
  return NanoSeconds(m_time);
}
    
} // namespace ns3
