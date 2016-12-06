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

#ifndef AQUA_SIM_NOISE_GENERATOR_H
#define AQUA_SIM_NOISE_GENERATOR_H

#include "ns3/object.h"
#include "ns3/vector.h"

namespace ns3 {

class Time;

 /**
  * \ingroup aqua-sim-ng
  *
  * \brief Noise Generator Base class
  */
class AquaSimNoiseGen : public Object {
public:
  static TypeId GetTypeId (void);

  // return the noise strength at location (x,y,z) at time t
  virtual double Noise (Time t, Vector vector) = 0;
  virtual double Noise (void) = 0;
  double Noise(double frequency);

private:
  double m_windNoise;
  double m_shippingNoise;
};	//class AquaSimNoiseGen

/**
 * \brief Constant noise generator
 */
class AquaSimConstNoiseGen : public AquaSimNoiseGen {
public:
  AquaSimConstNoiseGen ();
  ~AquaSimConstNoiseGen ();
  static TypeId GetTypeId (void);

  virtual double Noise (Time t, Vector vector);
  virtual double Noise (void);

private:
  double m_noise;
};	// class AquaSimConstNoiseGen

/**
 * \brief Random noise generator
 *    Random noise within the set bounds.
 *    Seed OR run # must be different to obtain randomness between simulation runs.
 */
class AquaSimRandNoiseGen : public AquaSimNoiseGen {
public:
  AquaSimRandNoiseGen ();
  ~AquaSimRandNoiseGen ();
  static TypeId GetTypeId (void);

  virtual double Noise (Time t, Vector vector);
  virtual double Noise (void);
  void SetBounds(double min, double max);

private:
  double m_noise;
  double m_min;
  double m_max;
};	// class AquaSimRandNoiseGen

/**
 * \brief Periodic noise generator
 *    Periodically generate noise
 */
class AquaSimPeriodicNoiseGen : public AquaSimNoiseGen {
public:
  AquaSimPeriodicNoiseGen ();
  ~AquaSimPeriodicNoiseGen ();
  static TypeId GetTypeId (void);

  virtual double Noise (Time t, Vector vector);
  virtual double Noise (void);
  void SetPeriod(double period);
  void SetLength(double length);

private:
  void SetNoise(double noise);
  void ResetNoise();
  double m_noise;
  double m_noiseAmount;
  double m_period;  //frequency of noise in seconds.
  double m_length;  //how long will noise occur in seconds
};	// class AquaSimPeriodicNoiseGen

}  //namespace ns3

#endif /* AQUA_SIM_NOISE_GENERATOR_H */
