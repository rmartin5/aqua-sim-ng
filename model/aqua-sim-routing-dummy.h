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


#ifndef AQUA_SIM_ROUTING_DUMMY_H
#define AQUA_SIM_ROUTING_DUMMY_H

#include "aqua-sim-routing.h"

namespace ns3 {


/**
 * \ingroup aqua-sim-ng
 *
 * \brief Dummy routing class.
 */
class AquaSimRoutingDummy : public AquaSimRouting {
 public:
  AquaSimRoutingDummy();
  static TypeId GetTypeId(void);
  int64_t AssignStreams (int64_t stream);

  virtual bool Recv(Ptr< Packet > packet, const Address &dest, uint16_t protocolNumber);

 protected:
  void DataForSink(Ptr<Packet> pkt);
  void MACsend(Ptr<Packet> pkt, Time delay=Seconds(0));
  virtual void DoDispose();

}; // class AquaSimRoutingDummy

} // namespace ns3

#endif /* AQUA_SIM_ROUTING_DUMMY_H */
