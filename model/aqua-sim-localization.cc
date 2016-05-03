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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimLocalization");
NS_OBJECT_ENSURE_REGISTERED(AquaSimLocalization);

AquaSimLocalization::AquaSimLocalization()
{
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

void
AquaSimLocalization::Recv(Ptr<Packet> p)
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

  m_localizationList.push_back(ls);

  if(m_localizationList.size() >= 4) {
    Lateration();
  }
}

void
AquaSimLocalization::SendLoc()
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

  p->AddHeader(loch);
  p->AddHeader(mach); //strictly to set demux packet type
  p->AddHeader(ash);

  if( !m_device->GetMac()->SendDown(p) )
  {
    NS_LOG_DEBUG("Localization failed to send. Is device busy/sleeping?");
  }

  Simulator::Schedule(m_localizationRefreshRate, &AquaSimLocalization::SendLoc, this);
}

void
AquaSimLocalization::Lateration()
{
  NS_LOG_FUNCTION(this << "Dummy localization.");
  //compute localization using stored local/neighbor node info;

  ClearLocalizationList();
}

Vector
AquaSimLocalization::GetAngleOfArrival(Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this << p << "Dummy angle of arrival computation.");

  return Vector(0,0,0);
}
