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


#ifndef CS_LRU_H
#define CS_LRU_H

#include "ns3/object.h"
#include "content-storage.h"
#include <list>
#include <unordered_map>
#include <utility>

namespace ns3 {

class CSLru : public ContentStorage{
public:
  static TypeId GetTypeId (void);
  CSLru();

  virtual void AddEntry(uint8_t* key, uint8_t* data);
  virtual bool RemoveEntry();
  virtual bool CacheFull();
  virtual uint8_t* GetEntry(uint8_t* key);
  bool EntryExist(uint8_t* key);
private:
  void Clean();
  std::list< std::pair<uint8_t*,uint8_t*> > item_list;
  std::unordered_map<uint8_t*, decltype(item_list.begin()) > item_map;
}; // class CSLru

} // namespace ns3

#endif /* CS_LRU_H */
