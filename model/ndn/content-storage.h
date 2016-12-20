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


#ifndef CONTENT_STORAGE_H
#define CONTENT_STORAGE_H

#include "ns3/object.h"

namespace ns3 {
enum CacheType {NO_CACHE, LRU, FIFO, RANDOM};

class ContentStorage : public Object{
public:

  static TypeId GetTypeId (void);
  ContentStorage();
  CacheType GetCacheType();
  void SetCacheType(CacheType type);
  void SetCacheSize(size_t size);

  virtual void AddEntry(uint8_t* key, uint8_t* data)=0;
  virtual bool RemoveEntry()=0;
  virtual bool CacheFull()=0;
  virtual uint8_t* GetEntry(uint8_t* key)=0;
protected:
  size_t m_cacheSize; //default(0) is unlimited
  CacheType m_cacheType;

}; // class ContentStorage

} // namespace ns3

#endif /* CONTENT_STORAGE_H */
