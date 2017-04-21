/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Connecticut
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

#ifndef AQUA_SIM_TRAFFIC_GEN_H
#define AQUA_SIM_TRAFFIC_GEN_H

#include "ns3/application.h"
#include "ns3/address.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"

namespace ns3 {

  /**
   * \ingroup aqua-sim-ng
   *
   * \brief Specialized traffic generator for underwater simulation.
   *
   * TODO: this should be expanded to better incorporate application layer packet integration
   */

class AquaSimTrafficGen : public Application
{
public:
  static TypeId GetTypeId();
  AquaSimTrafficGen ();
  ~AquaSimTrafficGen ();
  void SetDelay (double delay);
  void SetSize (uint32_t size);
protected:
  virtual void DoDipsose();
private:
  virtual void StartApplication();
  virtual void StopApplication();
  void DoGenerate();
  void CancelEvents();
  void SendPacket();

  double m_delayInt;
  uint32_t m_pktSize;
  Ptr<Socket> m_socket;
  Address m_peer;
  EventId m_sendEvent;
  TypeId m_tid;
};

} // namespace ns3

#endif /* AQUA_SIM_TRAFFIC_GEN_H */
