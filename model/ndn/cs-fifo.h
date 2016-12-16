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


#ifndef CS_FIFO_H
#define CS_FIFO_H

#include "ns3/object.h"
#include "content-storage.h"
#include <deque>
#include <utility>

namespace ns3 {

class CSFifo : public ContentStorage{
public:
  typedef std::deque<std::pair<uint8_t*,uint8_t*> > fifoCache;

  static TypeId GetTypeId (void);
  CSFifo();

  virtual void AddEntry(uint8_t* key, uint8_t* data);
  virtual bool RemoveEntry();
  virtual bool CacheFull();
  virtual uint8_t* GetEntry(uint8_t* key);
private:
   fifoCache m_cache;

}; // class CSFifo

} // namespace ns3

#endif /* CS_FIFO_H */
