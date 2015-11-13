/*
* aqua-sim-phy.cc
*
*  Created on: Nov 5, 2015
*      Author: Robert Martin
*/

#include "aqua-sim-phy.h"

#include "ns3/nstime.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED (AquaSimPhy);

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
  Object::Dispose();
}

} //ns3 namespace
