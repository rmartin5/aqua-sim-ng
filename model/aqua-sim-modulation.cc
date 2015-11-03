/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 UWSN Lab at the University of Connecticut
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

#include "ns3/double.h"
#include "ns3/uinteger.h"

#include "aqua-sim-modulation.h"

#include <cmath>

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (AquaSimModulation);

AquaSimModulation::AquaSimModulation () :
    m_codingEff(1), m_sps(1), m_ber(0)
{
}

TypeId
AquaSimModulation::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AquaSimModulation")
    .SetParent<Object> ()
    .AddAttribute ("CodingEff", "The coding efficiency: number of symbols per bit.",
       DoubleValue (1.0),
       MakeDoubleAccessor (&AquaSimModulation::m_codingEff),
       MakeDoubleChecker<double> ())
    .AddAttribute ("SPS", "The number of symbols per second.",
       UintegerValue (1),
       MakeUintegerAccessor (&AquaSimModulation::m_sps),
       MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("BER", "The bit error rate.",
       DoubleValue (0.0),
       MakeDoubleAccessor (&AquaSimModulation::m_ber),
       MakeDoubleChecker<double> ())
  ;
  return tid;
}

double
AquaSimModulation::TxTime (int pktSize) {
  return pktSize*8/Bps();
}

int
AquaSimModulation::PktSize (double txTime) {
  return int(txTime*Bps()/8);
}

double
AquaSimModulation::Per(int pktSize) {
  return 1 - std::pow(m_ber, pktSize);
}

}  // namespace ns3
