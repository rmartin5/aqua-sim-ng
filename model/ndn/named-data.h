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


#ifndef NAMED_DATA_H
#define NAMED_DATA_H

#include "ns3/object.h"
#include "ns3/packet.h"
#include "fib.h"
#include "pit.h"
#include "content-storage.h"
#include "ns3/aqua-sim-net-device.h"

namespace ns3 {

//For data packets, separating name prefix from data within payload.
#define DELIMITER "|||"

class NamedData : public Object {
public:
  static TypeId GetTypeId (void);
  NamedData();

  void SetFib(Ptr<Fib> fib);
  void SetPit(Ptr<Pit> pit);
  void SetContentStorage(Ptr<ContentStorage> cs);
  void SetNetDevice(Ptr<AquaSimNetDevice> device);

  void Recv(Ptr<Packet> packet);
  Ptr<Packet> CreateInterest(uint8_t* name, uint32_t nameSize);
  Ptr<Packet> CreateData(uint8_t* name, uint8_t* data, uint32_t nameSize, uint32_t dataSize);
  Ptr<Packet> CreateNameDiscovery(uint8_t* name, uint32_t nameSize);
  void SendPkt(Ptr<Packet> packet);
    //NOTE: check for busy terminal problem.

private:
  uint8_t* GetDataStr(Ptr<Packet> dataPkt);
  uint8_t* GetInterestStr(Ptr<Packet> dataPkt);

  Ptr<Fib> m_fib;
  Ptr<Pit> m_pit;
  Ptr<ContentStorage> m_cs;
  Ptr<AquaSimNetDevice> m_device;
  bool m_hasCache;

}; // class NamedData

} // namespace ns3

#endif /* NAMED_DATA_H */
