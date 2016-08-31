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

#include "aqua-sim-address.h"
#include "ns3/address.h"


namespace ns3 {

AquaSimAddress::AquaSimAddress ()
{
  memset (m_address, 0, 2);
}

AquaSimAddress::AquaSimAddress (uint16_t addr)
{
  m_address[0] = (addr >> 8) & 0xff;
  m_address[1] = (addr >> 0) & 0xff;
}

AquaSimAddress::~AquaSimAddress ()
{
}

uint8_t
AquaSimAddress::GetType (void)
{
  static uint8_t type = Address::Register ();
  return type;
}

Address
AquaSimAddress::ConvertTo (void) const
{
  return Address (GetType (), m_address, 2);
}

AquaSimAddress
AquaSimAddress::ConvertFrom (const Address &address)
{
  NS_ASSERT (IsMatchingType (address));
  AquaSimAddress uAddr;
  address.CopyTo (uAddr.m_address);
  return uAddr;
}

uint16_t
AquaSimAddress::GetAsInt (void) const
{
  return ((m_address[0] << 8) | (m_address[1] & 0xff));
}

bool
AquaSimAddress::IsMatchingType (const Address &address)
{
  return address.CheckCompatible (GetType (), 2);
}

AquaSimAddress::operator Address () const
{
  return ConvertTo ();
}

void
AquaSimAddress::CopyFrom (const uint8_t pBuffer[2])
{
  memcpy (m_address, pBuffer, 2);
}

void
AquaSimAddress::CopyTo (uint8_t pBuffer[2]) const
{
  memcpy (pBuffer, m_address, 2);
}

AquaSimAddress
AquaSimAddress::GetBroadcast ()
{
  return AquaSimAddress (255);
}
AquaSimAddress
AquaSimAddress::Allocate ()
{
  static uint16_t nextAllocated = 0;
  nextAllocated++;
  AquaSimAddress address;
  if (nextAllocated == 255) nextAllocated++;
  address.m_address[0] = (nextAllocated >> 8) & 0xff;
  address.m_address[1] = (nextAllocated >> 0) & 0xff;
    //ignore rollover due to 65535 available addresses
  return address;
}


std::ostream&
operator<< (std::ostream& os, const AquaSimAddress & address)
{
  uint8_t addr[2];
  address.CopyTo(addr);

  os << (uint32_t)addr[0] << (uint32_t)addr[1];
  return os;
}
std::istream&
operator>> (std::istream& is, AquaSimAddress & address)
{
  int x;
  is >> x;
  NS_ASSERT (0 <= x);
  address.m_address[0] = (x >> 8) & 0xff;;
  address.m_address[1] = (x >> 0) & 0xff;;
  return is;

  /*std::string v;
  is >> v;

  std::string::size_type col = 0;
  for (uint8_t i = 0; i < 2; ++i)
    {
      std::string tmp;
      std::string::size_type next;
      next = v.find (":", col);
      if (next == std::string::npos)
        {
          tmp = v.substr (col, v.size ()-col);
          address.m_address[i] = strtoul (tmp.c_str(), 0);
          break;
        }
      else
        {
          tmp = v.substr (col, next-col);
          address.m_address[i] = strtoul (tmp.c_str(), 0);
          col = next + 1;
        }
    }
  return is;
  */
}

} // namespace ns3
