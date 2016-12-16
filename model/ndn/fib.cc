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

#include "ns3/log.h"
#include "fib.h"
#include <iterator>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Fib");
NS_OBJECT_ENSURE_REGISTERED (Fib);

TypeId
Fib::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Fib")
    .SetParent<Object> ()
    .AddConstructor<Fib> ()
    ;
  return tid;
}

Fib::Fib() : m_strategy(MULTICAST)
{
  NS_LOG_FUNCTION(this);
  ClearTable();
}

std::list<AquaSimAddress>
Fib::InterestRecv(uint8_t* name)
{
  NS_LOG_DEBUG(this << name);

  std::list<AquaSimAddress> addressList;

  FibI iteratorEntry;
  iteratorEntry = FibTable.find(name);
  if (iteratorEntry == FibTable.end())
  {
    NS_LOG_DEBUG(this << "No entry found in FibTable for name:" << name);
    return addressList;
  }

  std::list<FibEntry> entry = iteratorEntry->second;
  switch(m_strategy) {
    case BEST_ROUTE:
      {
        FibEntry bestEntry = entry.front();
        for (std::list<FibEntry>::iterator it = entry.begin(); it != entry.end(); it++)
        {
          bestEntry = ((*it).second > bestEntry.second) ? *it : bestEntry;
        }
        addressList.push_back(bestEntry.first);
        break;
      }
    case MULTICAST:
      {
        for (std::list<FibEntry>::iterator it = entry.begin(); it != entry.end(); it++)
        {
          addressList.push_back((*it).first);
        }
        break;
      }
    default:
      NS_LOG_WARN("Fib::InterestRecv: incorrect forwarding strategy set.");
  }
  return addressList;
}

void
Fib::AddEntry (uint8_t* name, AquaSimAddress address, int routeCost)
{
  NS_LOG_DEBUG(this << name << address.GetAsInt() << routeCost);

  FibI entry;
  entry = FibTable.find(name);
  if (entry != FibTable.end())    //entry already exists
    entry->second.push_back(std::make_pair(address,routeCost));
  else {
    std::list<FibEntry> newEntry;
    newEntry.push_back(std::make_pair(address,routeCost));
    FibTable.insert(std::make_pair(name,newEntry));
  }
}

bool
Fib::RemoveEntry(uint8_t* name, AquaSimAddress address)
{
  NS_LOG_DEBUG(this << name << address.GetAsInt());

  FibI entry;
  entry = FibTable.find(name);
  if (entry == FibTable.end())
  {
    NS_LOG_WARN("Can not remove " << name << " since it does not exist in FibTable");
    return false;
  }

  if (entry->second.size() > 1) //more than one address for this name
  {
    for (std::list<FibEntry>::iterator it = entry->second.begin(); it != entry->second.end(); it++)
    {
      if ((*it).first == address)
      {
        entry->second.erase(it);
        return true;
      }
    }
    return false; //no matching address found within name entry
  }
  else
  {
    FibTable.erase(entry);
    return true;
  }
}

void
Fib::SetForwardStrategy(ForwardStrategy strategy)
{
  NS_LOG_FUNCTION(this);
  m_strategy = strategy;
}

void
Fib::ClearTable()
{
  FibTable.clear();
}
