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

#ifndef AQUA_SIM_SIMPLE_PROPAGATION_H
#define AQUA_SIM_SIMPLE_PROPAGATION_H

#include <vector>
#include "aqua-sim-propagation.h"

namespace ns3 {

class AquaSimNetDevice;
class Packet;

//extern const double SOUND_SPEED_IN_WATER;
/**
 * This propagation model calculates attenuation using rayleigh model
 * and allows all nodes in the network to receive a copy.
 */

class AquaSimSimplePropagation : public AquaSimPropagation
{
public:
  static TypeId GetTypeId (void);
  AquaSimSimplePropagation (void);
  ~AquaSimSimplePropagation (void);

  virtual std::vector<PktRecvUnit> * ReceivedCopies (Ptr<AquaSimNetDevice> s,
						     Ptr<Packet> p,
						     std::vector<Ptr<AquaSimNetDevice> > dList);

protected:
  virtual double RayleighAtt (double dist, double freq, double pT);
  virtual double Rayleigh (double SL);
  virtual double Rayleigh (double d, double f);
  virtual double Thorp (double range, double freq);
};  //class AquaSimSimplePropagation

}  // namespace ns3

#endif /* AQUA_SIM_SIMPLE_PROPAGATION_H */
