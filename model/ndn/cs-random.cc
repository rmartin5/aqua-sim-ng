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
#include "cs-random.h"
#include "ns3/random-variable-stream.h"
#include <math.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CSRandom");
NS_OBJECT_ENSURE_REGISTERED (CSRandom);

TypeId
CSRandom::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CSRandom")
    .SetParent<ContentStorage> ()
    .AddConstructor<CSRandom>()
    ;
  return tid;
}

CSRandom::CSRandom()
{
  m_cache.clear();
}

void
CSRandom::AddEntry(uint8_t* key, uint8_t* data)
{
  NS_LOG_FUNCTION(this);

  if (CacheFull()) {
    if(!RemoveEntry()) {
      NS_LOG_WARN(this << "Something went wrong when removing entry, ignoring add of key:" << key);
      return;
    }
  }

  m_cache.insert(std::make_pair(key,data));
}

bool
CSRandom::RemoveEntry()
{
  NS_LOG_FUNCTION(this);

  if(m_cache.empty()) {
    NS_LOG_DEBUG("Trying to remove empty cache");
    return false;
  }

  unsigned bucket, bucket_size;
  Ptr<UniformRandomVariable> rnd = CreateObject<UniformRandomVariable> ();

  do
  {
    bucket = round(rnd->GetValue(0,m_cache.bucket_count()-1));
  } while ( (bucket_size = m_cache.bucket_size(bucket)) == 0 );

  auto element = std::next(m_cache.begin(bucket), round(rnd->GetValue(0,bucket_size)));
  m_cache.erase(m_cache.find(element->first));

  //m_cache.erase(element);
  return true;
}

bool
CSRandom::CacheFull()
{
  return ((m_cacheSize==0) ? false : (m_cache.size() >= m_cacheSize) );
}

uint8_t*
CSRandom::GetEntry(uint8_t* key)
{
  NS_LOG_FUNCTION(this);

  if(m_cache.empty()) {
    NS_LOG_DEBUG("Cache empty");
    return NULL;
  }

  randomCache::const_iterator it = m_cache.find(key);
  if (it == m_cache.end()) {
    NS_LOG_DEBUG(this << "Could not find entry for key:" << key);
    return NULL;
  }
  return it->second;
}
