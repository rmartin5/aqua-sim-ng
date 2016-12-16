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

#ifndef NAMED_DATA_HEADER_H
#define NAMED_DATA_HEADER_H

#include <iostream>
#include "ns3/header.h"

namespace ns3 {

class NamedDataHeader : public Header
{
public:
  enum pType {NDN_INTEREST, NDN_DATA, NDN_DISCOVERY};

  NamedDataHeader();
  static TypeId GetTypeId(void);

  uint8_t GetPType();
  void SetPType(uint8_t type);

  //inherited methods
  virtual uint32_t GetSerializedSize(void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId(void) const;

private:
  uint8_t m_type;

};  // class NamedDataHeader

}  // namespace ns3

#endif /* NAMED_DATA_HEADER_H */
