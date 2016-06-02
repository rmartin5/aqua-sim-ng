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

#ifndef AQUA_SIM_MAC_BROADCAST_H
#define AQUA_SIM_MAC_BROADCAST_H

#include "aqua-sim-mac.h"

namespace ns3 {

#define BC_BACKOFF  0.5 //default is 0.1 the maximum time period for backoff
#define BC_MAXIMUMCOUNTER 10 //default is 4 the maximum number of backoff
#define BC_CALLBACK_DELAY 0.0001 // the interval between two consecutive sendings


class AquaSimBroadcastMac : public AquaSimMac
{
public:
  AquaSimBroadcastMac();

  static TypeId GetTypeId(void);
  int m_packetHeaderSize; //# of bytes in the header
  int m_packetSize;  //to test the optimized length of packet

  // to process the incoming packet
  virtual bool RecvProcess (Ptr<Packet>);
  void CallbackProcess ();
  void DropPacket (Ptr<Packet>);

  // to process the outgoing packet
  virtual bool TxProcess (Ptr<Packet>);
protected:
  void BackoffHandler(Ptr<Packet>);
private:
  int m_backoffCounter;
};  // class AquaSimBroadcastMac

} // namespace ns3

#endif /* AQUA_SIM_MAC_BROADCAST_H */
