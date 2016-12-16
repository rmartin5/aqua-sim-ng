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


#ifndef FIB_H
#define FIB_H

#include "ns3/object.h"
#include "ns3/aqua-sim-address.h"
#include <utility>
#include <list>
#include <map>

namespace ns3 {

class Fib : public Object {
public:
  enum ForwardStrategy {BEST_ROUTE, MULTICAST};
  //int for Best route strategy

  typedef std::pair<AquaSimAddress,int > FibEntry;
  typedef std::map<uint8_t*,std::list<FibEntry> >::iterator FibI;

  static TypeId GetTypeId (void);
  Fib();

  std::list<AquaSimAddress> InterestRecv(uint8_t* name);
  void AddEntry (uint8_t* name, AquaSimAddress address, int routeCost=0);
  bool RemoveEntry(uint8_t* name, AquaSimAddress address);
  void SetForwardStrategy(ForwardStrategy strategy);

private:
  void ClearTable();

  std::map<uint8_t*,std::list<FibEntry> > FibTable;
  ForwardStrategy m_strategy;

}; // class Fib

} // namespace ns3

#endif /* FIB_H */
