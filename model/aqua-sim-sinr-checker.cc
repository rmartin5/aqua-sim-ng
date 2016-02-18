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

#include "aqua-sim-sinr-checker.h"
#include "ns3/double.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (AquaSimSinrChecker);
NS_OBJECT_ENSURE_REGISTERED (AquaSimThresholdSinrChecker);

TypeId
AquaSimSinrChecker::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AquaSimSinrChecker")
      .SetParent<Object>()
    ;
  return tid;
}

TypeId
AquaSimThresholdSinrChecker::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AquaSimThresholdSinrChecker")
    .SetParent<AquaSimSinrChecker> ()
    .AddConstructor<AquaSimThresholdSinrChecker>()
    .AddAttribute ("DecodeableThresh", "The decodable threshold of a packet.",
       DoubleValue (0),
       MakeDoubleAccessor (&AquaSimThresholdSinrChecker::m_decThresh),
       MakeDoubleChecker<double> ())
    ;
  return tid;
}

AquaSimThresholdSinrChecker::AquaSimThresholdSinrChecker ()
{
  m_decThresh = 0;
}

AquaSimThresholdSinrChecker::~AquaSimThresholdSinrChecker()
{
}

bool
AquaSimThresholdSinrChecker::Decodable (double sinr) {
  return sinr > m_decThresh;
}

}  // namespace ns3
