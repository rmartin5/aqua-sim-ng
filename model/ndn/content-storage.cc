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
#include "content-storage.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ContentStorage");
NS_OBJECT_ENSURE_REGISTERED (ContentStorage);

TypeId
ContentStorage::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ContentStorage")
    .SetParent<Object> ()
    ;
  return tid;
}

ContentStorage::ContentStorage() :
  m_cacheType(NO_CACHE)
{
  NS_LOG_FUNCTION(this);
}

CacheType
ContentStorage::GetCacheType()
{
  return m_cacheType;
}

void
ContentStorage::SetCacheType(CacheType type)
{
  NS_LOG_DEBUG(this << type);
  m_cacheType = type;
}

void
ContentStorage::SetCacheSize(size_t size)
{
  NS_LOG_DEBUG(this << size);
  m_cacheSize = size;
}
