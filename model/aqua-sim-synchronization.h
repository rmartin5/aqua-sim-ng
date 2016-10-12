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

#ifndef AQUA_SIM_SYNCHRONIZATION_H
#define AQUA_SIM_SYNCHRONIZATION_H

#include "ns3/object.h"
#include "ns3/nstime.h"
#include "aqua-sim-net-device.h"

namespace ns3 {

class Packet;

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Syncronization class for underwater.
 *
 * Currently is strictly applying synchronization techniques without simulating the need for them (i.e. no internal clocks
 *  are handled since we rely on the simulator clock).
 */
class AquaSimSync : public Object {
public:
  AquaSimSync();
  static TypeId GetTypeId(void);
  void SetDevice(Ptr<AquaSimNetDevice>);

  //Should be overloaded for protocol needs
  virtual void RecvSync(Ptr<Packet>);
  virtual void RecvSyncBeacon(Ptr<Packet>);

protected:
  void SendBeacons();
  virtual void SendSync();
  void SyncSend(bool isBeacon);

  Ptr<Packet> CreateSyncPacket(bool isBeacon);

  int m_numBeacons;
  Time m_clockSkew;  //local clock skew
  Time m_localClock;  //currently not implemented.

  Time m_beaconSendInterval;
  Time m_periodicSyncInterval;
  Time m_clockSkewInterval;   // re-estimation of clock skew
  Ptr<AquaSimNetDevice> m_device;

  //clockskew values:
  int m_beaconRecvCount;
  Time m_beaconClockSkew;

}; //class AquaSimSync

} // namespace ns3

#endif /* AQUA_SIM_SYNCHRONIZATION_H */
