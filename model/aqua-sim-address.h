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

/************

*************/

#ifndef AQUA_SIM_ADDRESS_H
#define AQUA_SIM_ADDRESS_H

#include <stdint.h>
#include "ns3/address.h"
#include <iostream>
#include <cstring>


namespace ns3 {

class Address;
/**
 * \ingroup aqua-sim-ng
 *
 * \brief Specialized address for distinguishing nodes and protocol support. 16 bit addresses supported.
 *
 * Inspired by UanAddress but implemented for future modification to adhere to aqua-sim modification.
 */
class AquaSimAddress
{
public:
  /** Constructor */
  AquaSimAddress ();
  AquaSimAddress (uint16_t addr);
  virtual ~AquaSimAddress ();

  static AquaSimAddress ConvertFrom (const Address &address);
  static bool IsMatchingType  (const Address &address);
  operator Address () const;
  void CopyFrom (const uint8_t pBuffer[2]);
  void CopyTo (uint8_t pBuffer[2]) const;
  uint16_t GetAsInt (void) const;
  static AquaSimAddress GetBroadcast (void);

  static AquaSimAddress Allocate ();

private:
  uint8_t m_address[2]; //16-bit address
  static uint8_t GetType (void);
  Address ConvertTo (void) const;

  friend bool operator <  (const AquaSimAddress &a, const AquaSimAddress &b);
  friend bool operator == (const AquaSimAddress &a, const AquaSimAddress &b);
  friend bool operator != (const AquaSimAddress &a, const AquaSimAddress &b);
  friend std::ostream& operator<< (std::ostream& os, const AquaSimAddress & address);
  friend std::istream& operator>> (std::istream& is, AquaSimAddress & address);

};  // class AquaSimAddress


/**
 * Address comparison, less than.
 *
 * \param a First address to compare.
 * \param b Second address to compare.
 * \return True if a < b.
 */
inline bool operator < (const AquaSimAddress &a, const AquaSimAddress &b)
{
  return memcmp (a.m_address, b.m_address, 2) < 0;
}


/**
 * Address comparison, equalit.
 *
 * \param a First address to compare.
 * \param b Second address to compare.
 * \return True if a == b.
 */
inline bool operator == (const AquaSimAddress &a, const AquaSimAddress &b)
{
  return memcmp (a.m_address, b.m_address, 2) == 0;
}

/**
 * Address comparison, unequal.
 *
 * \param a First address to compare.
 * \param b Second address to compare.
 * \return True if a != b.
 */
inline bool operator != (const AquaSimAddress &a, const AquaSimAddress &b)
{
  return memcmp (a.m_address, b.m_address, 2) != 0;
}

/**
 * Write \pname{address} to stream \pname{os} as 8 bit integer.
 *
 * \param os The output stream.
 * \param address The address
 * \return The output stream.
 */
std::ostream& operator<< (std::ostream& os, const AquaSimAddress & address);

/**
 * Read \pname{address} from stream \pname{is} as 8 bit integer.
 *
 * \param is The input stream.
 * \param address The address variable to set.
 * \return The input stream.
 */
std::istream& operator>> (std::istream& is, AquaSimAddress & address);

} // namespace ns3

#endif /* AQUA_SIM_ADDRESS_H */
