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

#include "named-data-header.h"

#include "ns3/log.h"
#include "ns3/buffer.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NamedDataHeader");
NS_OBJECT_ENSURE_REGISTERED(NamedDataHeader);

NamedDataHeader::NamedDataHeader() :
  m_type(NDN_INTEREST)
{
}

TypeId
NamedDataHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::NamedDataHeader")
    .SetParent<Header>()
    .AddConstructor<NamedDataHeader>()
  ;
  return tid;
}

uint32_t
NamedDataHeader::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_type = i.ReadU8();

  return GetSerializedSize();
}

uint32_t
NamedDataHeader::GetSerializedSize(void) const
{
  //reserved bytes for header
  return (1);
}

void
NamedDataHeader::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8(m_type);
}

void
NamedDataHeader::Print(std::ostream &os) const
{
  os << "Named Data Header is: PType=";
  switch (m_type){
    case NDN_INTEREST:   os << "INTEREST";  break;
    case NDN_DATA:       os << "DATA";    break;
    case NDN_DISCOVERY:  os << "DISCOVERY";   break;
  }
  os << "\n";
}

TypeId
NamedDataHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

uint8_t
NamedDataHeader::GetPType()
{
  return m_type;
}
void
NamedDataHeader::SetPType(uint8_t type)
{
  m_type = type;
}
