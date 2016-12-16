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
#include "pit.h"
#include <utility>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Pit");
NS_OBJECT_ENSURE_REGISTERED (Pit);

TypeId
Pit::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Pit")
    .SetParent<Object> ()
    .AddConstructor<Pit> ()
    .AddAttribute ("EntryTimeout", "Timeout for each entry in the PIT table (s)",
      TimeValue (Seconds (120)),
      MakeTimeAccessor (&Pit::m_timeout),
      MakeTimeChecker ())
    ;
  return tid;
}

Pit::Pit() : m_timeout(Seconds(120))
{
  NS_LOG_FUNCTION(this);
  ClearTable();
}

size_t
Pit::GetPitSize()
{
  return PitTable.size();
}

bool
Pit::RemoveEntry(uint8_t* name)
{
  NS_LOG_DEBUG(this << name);

  PitI entry;
  entry = PitTable.find(name);
  if (entry == PitTable.end())
  {
    NS_LOG_WARN("Can not remove " << name << " since it does not exist in PitTable");
    return false;
  }

  if (entry->second.timeout.IsRunning()) {
    entry->second.timeout.Cancel();
  }
  PitTable.erase(entry);
  return true;
}

bool
Pit::RemoveEntryByI(PitI entry)
{
  NS_LOG_DEBUG(this << entry->first);

  if (entry == PitTable.end())
  {
    NS_LOG_WARN("Can not remove " << entry->first << " since it does not exist in PitTable");
    return false;
  }

  if (entry->second.timeout.IsRunning()) {
    entry->second.timeout.Cancel();
  }
  PitTable.erase(entry);
  return true;
}

void
Pit::AddEntry(uint8_t* name, AquaSimAddress address)
{
  NS_LOG_DEBUG(this << name << address);

  PitI entry;
  entry = PitTable.find(name);
  if (entry == PitTable.end())
  {
    //create new entry
    PitEntry newEntry;
    newEntry.address.push_back(address);
    newEntry.timeout.SetArguments(name);
    newEntry.timeout.SetFunction(&Pit::RemoveEntry, this);
    newEntry.timeout.Schedule(m_timeout);
    PitTable.insert(std::make_pair(name,newEntry));
  }
  else
  {
    //add new address to PitEntry
    entry->second.address.push_back(address);
  }
}

void
Pit::SetTimeout(Time timeout)
{
  m_timeout = timeout;
}

void
Pit::ClearTable()
{
  PitTable.clear();
}
