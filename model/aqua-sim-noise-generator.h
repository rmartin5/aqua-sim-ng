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

#ifndef AQUA_SIM_NOISE_GENERATOR_H
#define AQUA_SIM_NOISE_GENERATOR_H

#include "ns3/object.h"
#include "ns3/nstime.h"

namespace ns3 {

  /*
   * TODO Add more class variations i.e. fluctuating noise, random noise, etc.
   */

  /*
   * Noise Generator Base Class
   */

class AquaSimNoiseGen : public Object {
public:
  static TypeId GetTypeId (void);

  // return the noise strength at location (x,y,z) at time t
  virtual double Noise (Time t, double x, double y, double z) = 0;
  virtual double Noise (void) = 0;
};	//class AquaSimNoiseGen

class AquaSimConstNoiseGen : public AquaSimNoiseGen {
public:
  AquaSimConstNoiseGen ();
  ~AquaSimConstNoiseGen ();
  static TypeId GetTypeId (void);

  virtual double Noise (Time t, double x, double y, double z);
  virtual double Noise (void);

private:
  double m_noise;
};	// class AquaSimConstNoiseGen

}  //namespace ns3

#endif /* AQUA_SIM_NOISE_GENERATOR_H */
