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


#ifndef PIT_H
#define PIT_H

#include "ns3/object.h"
#include "ns3/timer.h"
#include "ns3/nstime.h"
#include "ns3/aqua-sim-address.h"
#include <map>

namespace ns3 {

class Pit : public Object {
public:
  struct PitEntry {
    std::list<AquaSimAddress> address;
    Timer timeout;
  };

  typedef std::map<uint8_t*,PitEntry>::iterator PitI;

  static TypeId GetTypeId (void);
  Pit();

  size_t GetPitSize();
  bool RemoveEntry(uint8_t* name);
  bool RemoveEntryByI(PitI);
  bool AddEntry(uint8_t* name, AquaSimAddress address);
  void SetTimeout(Time timeout);
  std::list<AquaSimAddress> GetEntry(uint8_t* name);

private:
  void ClearTable();

  std::map<uint8_t*,PitEntry> PitTable;
  Time m_timeout;

}; // class Pit

} // namespace ns3

#endif /* PIT_H */
