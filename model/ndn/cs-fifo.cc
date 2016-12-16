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
#include "cs-fifo.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CSFifo");
NS_OBJECT_ENSURE_REGISTERED (CSFifo);

TypeId
CSFifo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CSFifo")
    .SetParent<ContentStorage> ()
    ;
  return tid;
}

CSFifo::CSFifo()
{
  m_cache.clear();
}

void
CSFifo::AddEntry(uint8_t* key, uint8_t* data)
{
  NS_LOG_FUNCTION(this);

  if (CacheFull()) {
    if(!RemoveEntry()) {
      NS_LOG_WARN(this << "Something went wrong when removing entry, ignoring add of key:" << key);
      return;
    }
  }

  m_cache.push_back(std::make_pair(key,data));
}

bool
CSFifo::RemoveEntry()
{
  NS_LOG_FUNCTION(this);

  if(m_cache.empty()) {
    NS_LOG_DEBUG("Trying to remove empty cache");
    return false;
  }

  m_cache.pop_front();
  return true;
}

bool
CSFifo::CacheFull()
{
  return (m_cache.size() >= m_cacheSize);
}

uint8_t*
CSFifo::GetEntry(uint8_t* key)
{
  NS_LOG_FUNCTION(this);

  if(m_cache.empty()) {
    NS_LOG_DEBUG("Cache empty");
    return NULL;
  }

  for (fifoCache::iterator it = m_cache.begin(); it < m_cache.end(); it++) {
    if ((*it).first == key) return (*it).second;
  }

  NS_LOG_DEBUG(this << "Could not find entry for key:" << key);
  return NULL;
}
