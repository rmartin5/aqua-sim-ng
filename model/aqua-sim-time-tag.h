/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 */

#ifndef AQUA_SIM_TIME_TAG
#define AQUA_SIM_TIME_TAG

#include <ns3/tag.h>
#include "ns3/nstime.h"

namespace ns3 {

/**
 * \ingroup Keep timestamp of the packet arrival from network layer
 *
 * Tag the packet with the arrival timestamp
 * 
 */
class AquaSimTimeTag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const;

  /**
   * Constructor
   */
  AquaSimTimeTag ();

  // Set chunk type
  void SetTime (Time time);
  // Return chunk type
  Time GetTime ();

  // From class Tag
  uint32_t GetSerializedSize (void) const;
  void Serialize (TagBuffer i) const;
  void Deserialize (TagBuffer i);
  void Print (std::ostream &os) const;


private:
  uint64_t m_time; // in nanoseconds
};

} // namespace ns3

#endif /* AQUA_SIM_TIME_TAG */
