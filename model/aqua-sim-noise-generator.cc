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
#include "ns3/random-variable-stream.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimNoiseGen");
NS_OBJECT_ENSURE_REGISTERED (AquaSimNoiseGen);
NS_OBJECT_ENSURE_REGISTERED (AquaSimConstNoiseGen);

TypeId
AquaSimNoiseGen::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AquaSimNoiseGen")
    .SetParent<Object> ()
    .AddAttribute("Wind", "Wind in m/s",
      DoubleValue(1),
      MakeDoubleAccessor(&AquaSimNoiseGen::m_windNoise),
      MakeDoubleChecker<double>())
    .AddAttribute("Shipping", "Normalized shipping noise",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimNoiseGen::m_shippingNoise),
      MakeDoubleChecker<double>())
    ;
  return tid;
}

/*
 * Urick noise generator
 *    "Principles of Underwater Sound" by Robert J. Urick
 *
 *  Given frequency (Khz), returns Urick acoustic model noise in dB
 */
double
AquaSimNoiseGen::Noise(double frequency) {
  double turbulence, wind, ship, thermal;
  double turbulenceDb, windDb, shipDb, thermalDb;

  turbulenceDb = 17.0 - 30.0 * std::log10 (frequency);
  turbulence = std::pow (10.0, turbulenceDb * 0.1);

  shipDb = 40.0 + 20.0 * (m_shippingNoise - 0.5) + 26.0 *
            std::log10 (frequency) - 60.0 * std::log10 (frequency + 0.03);
  ship = std::pow (10.0, (shipDb * 0.1));

  windDb = 50.0 + 7.5 * std::pow (m_windNoise, 0.5) + 20.0 *
            std::log10 (frequency) - 40.0 * std::log10 (frequency + 0.4);
  wind = std::pow (10.0, windDb * 0.1);

  thermalDb = -15 + 20 * std::log10 (frequency);
  thermal = std::pow (10, thermalDb * 0.1);

  return (10 * std::log10 (turbulence + ship + wind + thermal) );
}

/* AquaSimConstNoiseGen */
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

/*AquaSimRandNoiseGen */
AquaSimRandNoiseGen::AquaSimRandNoiseGen() :
    m_noise(0)
{
}

AquaSimRandNoiseGen::~AquaSimRandNoiseGen()
{
}

TypeId
AquaSimRandNoiseGen::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AquaSimRandNoiseGen")
    .SetParent<AquaSimNoiseGen> ()
    .AddConstructor<AquaSimRandNoiseGen> ()
    .AddAttribute ("MinNoise", "The minimum noise of the channel (dB re 1uPa @ 1m).",
       DoubleValue (0),
       MakeDoubleAccessor (&AquaSimRandNoiseGen::m_min),
       MakeDoubleChecker<double> ())
   .AddAttribute ("MaxNoise", "The maximum noise of the channel (dB re 1uPa @ 1m).",
      DoubleValue (150),
      MakeDoubleAccessor (&AquaSimRandNoiseGen::m_max),
      MakeDoubleChecker<double> ())
  ;
  return tid;
}

double
AquaSimRandNoiseGen::Noise(Time t, Vector vector) {
  //TODO update this in future work
  return m_noise;
}

double
AquaSimRandNoiseGen::Noise() {
  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
  m_noise = m_rand->GetValue(m_min,m_max);
  return m_noise;
}

void
AquaSimRandNoiseGen::SetBounds(double min, double max) {
  m_min = min;
  m_max = max;
}

}  // namespace ns3
