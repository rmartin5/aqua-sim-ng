/*
* aqua-sim-phy.cc
*
*  Created on: Nov 5, 2015
*      Author: Robert Martin
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
