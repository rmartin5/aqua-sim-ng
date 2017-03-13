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


#include "aqua-sim-localization.h"

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-mac.h"

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/integer.h"

#include "math.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimLocalization");
NS_OBJECT_ENSURE_REGISTERED(AquaSimLocalization);

AquaSimLocalization::AquaSimLocalization()
{
  NS_LOG_FUNCTION(this);
}

TypeId
AquaSimLocalization::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimLocalization")
  .SetParent<Object>()
  //.AddConstructor<AquaSimLocalization>()
  .AddAttribute ("RefreshRate", "How often localization is implemented.",
    TimeValue (Hours(5)),
    MakeTimeAccessor (&AquaSimLocalization::m_localizationRefreshRate),
    MakeTimeChecker ())
  ;
  return tid;
}

void
AquaSimLocalization::SetDevice(Ptr<AquaSimNetDevice> device)
{
  m_device = device;
}

void
AquaSimLocalization::SetPosition(Vector nodePosition)
{
  m_nodePosition = nodePosition;
}

void
AquaSimLocalization::SetPr(double pr)
{
  m_pr = pr;
}

std::list<LocalizationStructure>
AquaSimLocalization::GetLocalizationList()
{
  return m_localizationList;
}

void
AquaSimLocalization::ClearLocalizationList()
{
  m_localizationList.clear();
}

double
AquaSimLocalization::EuclideanDistance2D(Vector2D s, Vector2D r)
{
  return sqrt( pow((s.x-r.x),2) + pow((s.y-r.y),2) );
}

double
AquaSimLocalization::EuclideanDistance3D(Vector s, Vector r)
{
  return sqrt( pow((s.x-r.x),2) + pow((s.y-r.y),2) + pow((s.z-r.z),2) );
}

double
AquaSimLocalization::LocationError(Vector s, Vector r, double estRange)
{
  return std::abs (EuclideanDistance3D(s,r) - pow(estRange,2));
}

void
AquaSimLocalization::DoDispose()
{
  NS_LOG_FUNCTION(this);
  m_device=0;
}


/*
 *  Range-based Localization
 */

NS_OBJECT_ENSURE_REGISTERED(AquaSimRBLocalization);

AquaSimRBLocalization::AquaSimRBLocalization() :
  m_localizationThreshold(4)
{
}

TypeId
AquaSimRBLocalization::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimRBLocalization")
  .SetParent<AquaSimLocalization>()
  .AddConstructor<AquaSimRBLocalization>()
  .AddAttribute ("RefNode", "Set as a reference node for localization",
    BooleanValue (0),
    MakeBooleanAccessor (&AquaSimRBLocalization::m_referenceNode),
    MakeBooleanChecker ())
  .AddAttribute ("ConfidenceThreshold", "Threshold to determine if node can be a location reference node",
    DoubleValue (0.8),
    MakeDoubleAccessor (&AquaSimRBLocalization::m_confidenceThreshold),
    MakeDoubleChecker<double> ())
  .AddAttribute ("LocThreshold", "Threshold to determine if we should try to localize node",
    IntegerValue (4),
    MakeIntegerAccessor (&AquaSimRBLocalization::m_localizationThreshold),
    MakeIntegerChecker<int> ())
  ;
  return tid;
}

void
AquaSimRBLocalization::Recv(Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this << p);

  Vector aoa = GetAngleOfArrival(p);

  AquaSimHeader ash;
  MacHeader mach;
  LocalizationHeader loch;
  p->RemoveHeader(ash);
  p->RemoveHeader(mach);
  p->PeekHeader(loch);

  LocalizationStructure ls;

  ls.m_RSSI = m_pr;
  ls.m_AoA = aoa;
  ls.m_TDoA = ash.GetTimeStamp();
  ls.m_ToA = Simulator::Now();
  ls.m_knownLocation = loch.GetNodePosition();
  ls.m_nodeID = ash.GetSAddr().GetAsInt();
  ls.m_nodeConfidence = loch.GetConfidence();

  m_localizationList.push_back(ls);

  if(m_localizationList.size() >= (unsigned)m_localizationThreshold) {
    Lateration();
  }
}

void
AquaSimRBLocalization::SendLoc()
{
  Ptr<Packet> p = Create<Packet>();
  AquaSimHeader ash;
  MacHeader mach;
  LocalizationHeader loch;

  ash.SetDirection(AquaSimHeader::DOWN);
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
  ash.SetDAddr(AquaSimAddress::GetBroadcast());
  ash.SetErrorFlag(false);
  ash.SetUId(p->GetUid());
  ash.SetTimeStamp(Simulator::Now());

  mach.SetDemuxPType(MacHeader::UWPTYPE_LOC);
  loch.SetNodePosition(m_nodePosition);
  loch.SetConfidence(m_confidence);

  p->AddHeader(loch);
  p->AddHeader(mach); //strictly to set demux packet type
  p->AddHeader(ash);

  if( !m_device->GetMac()->SendDown(p) )
  {
    NS_LOG_DEBUG("Localization failed to send. Is device busy/sleeping?");
  }

  Simulator::Schedule(m_localizationRefreshRate, &AquaSimRBLocalization::SendLoc, this);
}

void
AquaSimRBLocalization::SetReferenceNode(bool ref)
{
  m_referenceNode = ref;
}
void
AquaSimRBLocalization::SetConfidenceThreshold(double confidence)
{
  m_confidence = confidence;
}
void
AquaSimRBLocalization::SetLocalizationThreshold(double locThreshold)
{
  m_localizationThreshold = locThreshold;
}

void
AquaSimRBLocalization::Lateration()
{
  NS_LOG_FUNCTION(this);
  if (m_referenceNode) return;

  double errorTotal=0;
  double estRange=0;
  double locationTotal=0;

  std::list<LocalizationStructure>::iterator it=m_localizationList.begin();
  for (; it != m_localizationList.end(); ++it)
  {
    estRange = ((*it).m_ToA.GetMilliSeconds()  - (*it).m_TDoA.GetMilliSeconds()) * 1500;
    errorTotal += LocationError((*it).m_knownLocation, m_nodePosition, estRange);
    locationTotal += EuclideanDistance3D((*it).m_knownLocation, m_nodePosition);
    estRange=0;
  }

  m_confidence = 1 - errorTotal / locationTotal;
  if (m_confidence > m_confidenceThreshold)
  {
    m_referenceNode = 1;
  }

  ClearLocalizationList();
}

Vector
AquaSimRBLocalization::GetAngleOfArrival(Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this << p << "Dummy angle of arrival computation.");

  return Vector(0,0,0);
}
