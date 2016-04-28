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


#include "aqua-sim-synchronization.h"

#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-mac.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimSync");
NS_OBJECT_ENSURE_REGISTERED(AquaSimSync);

AquaSimSync::AquaSimSync() :
m_clockSkew(Seconds(0)),
m_beaconRecvCount(0)
{
}

TypeId
AquaSimSync::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimSync")
  .SetParent<Object>()
  //.AddConstructor<AquaSimSync>()
  .AddAttribute ("NumBeacons", "Set the number of beacons to send per phase.",
    IntegerValue (60),
    MakeIntegerAccessor (&AquaSimSync::m_numBeacons),
    MakeIntegerChecker<int> ())
  .AddAttribute ("BeaconSendInterval", "Interval at which beacons are sent per phase.",
    TimeValue (Seconds(10)),
    MakeTimeAccessor (&AquaSimSync::m_beaconSendInterval),
    MakeTimeChecker ())
  .AddAttribute ("PeriodicSyncInterval", "Synchronization interval.",
    TimeValue (Minutes(10)),
    MakeTimeAccessor (&AquaSimSync::m_periodicSyncInterval),
    MakeTimeChecker ())
  .AddAttribute ("ClockSkewInterval", "Clock skew re-estimation interval.",
    TimeValue (Hours(5)),
    MakeTimeAccessor (&AquaSimSync::m_clockSkewInterval),
    MakeTimeChecker ())
  ;
  return tid;
}

void
AquaSimSync::RecvSync(Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this << p);
  AquaSimHeader ash;
  p->PeekHeader(ash);

  m_localClock = ash.GetTimeStamp() + m_clockSkew;
  //NOTE can and SHOULD be overloaded
}

void
AquaSimSync::RecvSyncBeacon(Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this << p);
  AquaSimHeader ash;
  p->PeekHeader(ash);

  m_beaconRecvCount++;
  m_beaconClockSkew += Simulator::Now() - ash.GetTimeStamp();

  if (m_beaconRecvCount >= m_numBeacons){
    m_clockSkew = m_beaconClockSkew / m_beaconRecvCount;
    m_beaconRecvCount = 0;
    m_beaconClockSkew = Seconds(0);
  }
  //NOTE can and SHOULD be overloaded
}

void
AquaSimSync::SendBeacons()
{
  Time beaconSendDelay = Seconds(0);
  for(int i=0; i < m_numBeacons; i++)
  {
    Simulator::Schedule(beaconSendDelay, &AquaSimSync::SyncSend, this, true);
    beaconSendDelay += m_beaconSendInterval;
  }

  Simulator::Schedule(m_clockSkewInterval, &AquaSimSync::SendBeacons, this);
}

void
AquaSimSync::SyncSend(bool isBeacon)
{
  if(!m_device->GetMac()->SendDown( CreateSyncPacket(isBeacon) ))
  {
    NS_LOG_DEBUG("Sync/Beacon failed to send. Is device busy/sleeping?");
  }
}

void
AquaSimSync::SendSync()
{
  Simulator::ScheduleNow(&AquaSimSync::SyncSend, this, false);
  Simulator::Schedule(m_periodicSyncInterval, &AquaSimSync::SendSync, this);
  //NOTE can and SHOULD be overloaded for sync protocol purposes
}

void
AquaSimSync::SetDevice(Ptr<AquaSimNetDevice> device)
{
  m_device = device;
}

Ptr<Packet>
AquaSimSync::CreateSyncPacket(bool isBeacon)
{
  Ptr<Packet> p = Create<Packet>();
  AquaSimHeader ash;
  MacHeader mach;

  ash.SetDirection(AquaSimHeader::DOWN);
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
  ash.SetDAddr(AquaSimAddress::GetBroadcast());
  ash.SetErrorFlag(false);
  ash.SetUId(p->GetUid());
  ash.SetTimeStamp(Simulator::Now());

  if(isBeacon){
    mach.SetDemuxPType(MacHeader::UWPTYPE_SYNC_BEACON);
  }
  else{
    mach.SetDemuxPType(MacHeader::UWPTYPE_SYNC);
  }

  p->AddHeader(mach); //strictly to set demux packet type
  p->AddHeader(ash);
  return p;
}
