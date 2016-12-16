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
#include "cs-lru.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CSLru");
NS_OBJECT_ENSURE_REGISTERED (CSLru);

TypeId
CSLru::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CSLru")
    .SetParent<ContentStorage> ()
    ;
  return tid;
}

CSLru::CSLru()
{
}

void
CSLru::AddEntry(uint8_t* key, uint8_t* data)
{
  NS_LOG_FUNCTION(this);

  auto it = item_map.find(key);
  if(it != item_map.end()) {
    item_list.erase(it->second);
    item_map.erase(it);
  }
  item_list.push_front(std::make_pair(key,data));
  item_map.insert(std::make_pair(key, item_list.begin()));
  Clean();
}

bool
CSLru::RemoveEntry()
{
  NS_LOG_WARN("Dummy RemoveEntry function. Taken care of internally if cache exceeds size.");
  return true;
}

bool
CSLru::CacheFull()
{
  return (item_map.size() > m_cacheSize);
}

uint8_t*
CSLru::GetEntry(uint8_t* key)
{
  NS_LOG_FUNCTION(this);

  if (!EntryExist(key)) {
    NS_LOG_DEBUG(this << "Could not find entry for key:" << key);
    return NULL;
  }
  auto it = item_map.find(key);
  item_list.splice(item_list.begin(), item_list, it->second);
  return it->second->second;
}

bool
CSLru::EntryExist(uint8_t* key)
{
  return (item_map.count(key)>0);
}

void
CSLru::Clean()
{
  while(CacheFull()) {
    auto last_it = item_list.end(); last_it--;
    item_map.erase(last_it->first);
    item_list.pop_back();
  }
}
