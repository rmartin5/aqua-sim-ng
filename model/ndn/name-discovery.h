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
* 96Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
* Author: Robert Martin <robert.martin@engr.uconn.edu>
*/


#ifndef NAME_DISCOVERY_H
#define NAME_DISCOVERY_H

#include "ns3/object.h"
#include <utility>
#include <string>
#include "ns3/aqua-sim-address.h"

namespace ns3 {

class NameDiscovery {
public:
  static TypeId GetTypeId (void);
  NameDiscovery();

  //return both the interest (payload) and neighbor node address
  std::pair<uint8_t*,AquaSimAddress> ProcessNameDiscovery(Ptr<Packet> packet);

  //Used for shortening local name path stored
  void ShortenNamePrefix(uint8_t* name, char delim);

}; // class NameDiscovery

} // namespace ns3

#endif /* NAME_DISCOVERY_H */
