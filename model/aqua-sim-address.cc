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
  m_address = 255;
}

AquaSimAddress::AquaSimAddress (uint8_t addr)
  : m_address (addr)
{
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
  return Address (GetType (), &m_address, 1);
}

AquaSimAddress
AquaSimAddress::ConvertFrom (const Address &address)
{
  NS_ASSERT (IsMatchingType (address));
  AquaSimAddress uAddr;
  address.CopyTo (&uAddr.m_address);
  return uAddr;
}

uint8_t
AquaSimAddress::GetAsInt (void) const
{
  return m_address;
}
bool
AquaSimAddress::IsMatchingType (const Address &address)
{
  return address.CheckCompatible (GetType (), 1);
}

AquaSimAddress::operator Address () const
{
  return ConvertTo ();
}

void
AquaSimAddress::CopyFrom (const uint8_t *pBuffer)
{
  m_address = *pBuffer;
}

void
AquaSimAddress::CopyTo (uint8_t *pBuffer)
{
  *pBuffer = m_address;

}

AquaSimAddress
AquaSimAddress::GetBroadcast ()
{
  return AquaSimAddress (255);
}
AquaSimAddress
AquaSimAddress::Allocate ()
{
  static uint8_t nextAllocated = 0;

  uint32_t address = nextAllocated++;
  if (nextAllocated == 255)
    {
      nextAllocated = 0;
    }

  return AquaSimAddress (address);
}

bool
operator < (const AquaSimAddress &a, const AquaSimAddress &b)
{
  return a.m_address < b.m_address;
}

bool
operator == (const AquaSimAddress &a, const AquaSimAddress &b)
{
  return a.m_address == b.m_address;
}

bool
operator != (const AquaSimAddress &a, const AquaSimAddress &b)
{
  return !(a == b);
}

std::ostream&
operator<< (std::ostream& os, const AquaSimAddress & address)
{
  os << (int) address.m_address;
  return os;
}
std::istream&
operator>> (std::istream& is, AquaSimAddress & address)
{
  int x;
  is >> x;
  NS_ASSERT (0 <= x);
  NS_ASSERT (x <= 255);
  address.m_address = x;
  return is;
}

} // namespace ns3
