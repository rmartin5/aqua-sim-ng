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

#include "aqua-sim-noise-generator.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/vector.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimNoiseGen");
NS_OBJECT_ENSURE_REGISTERED (AquaSimNoiseGen);
NS_OBJECT_ENSURE_REGISTERED (AquaSimConstNoiseGen);

TypeId
AquaSimNoiseGen::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AquaSimNoiseGen")
    .SetParent<Object> ()
    ;
  return tid;
}

AquaSimConstNoiseGen::AquaSimConstNoiseGen() :
    m_noise(0)
{
}

AquaSimConstNoiseGen::~AquaSimConstNoiseGen()
{
}

TypeId
AquaSimConstNoiseGen::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AquaSimConstNoiseGen")
    .SetParent<AquaSimNoiseGen> ()
    .AddConstructor<AquaSimConstNoiseGen> ()
    .AddAttribute ("Noise", "The constant noise of the channel.",
       DoubleValue (0),
       MakeDoubleAccessor (&AquaSimConstNoiseGen::m_noise),
       MakeDoubleChecker<double> ())
  ;
  return tid;
}

double
AquaSimConstNoiseGen::Noise(Time t, Vector vector) {
  //TODO update this in future work
  return m_noise;
}

double
AquaSimConstNoiseGen::Noise() {
  return m_noise;
}

//Could add characteristics to noise in the future.

}  // namespace ns3
