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

#ifndef AQUA_SIM_PROPAGATION_H
#define AQUA_SIM_PROPAGATION_H

#include <vector>

#include "ns3/nstime.h"
#include "ns3/object.h"

#include "aqua-sim-net-device.h"

namespace ns3 {

class Packet;
class MobilityModel;
//class AquaSimNetDevice;

extern const double SOUND_SPEED_IN_WATER;

/*
 * \ Underwater propagation model base class.
 *
 * Calculates the Pr by which the receiver will get a packet sent by
 * the node that applied the tx PacketStamp for a given interface type
 */


struct PktRecvUnit {
  double pR;
  Time pDelay;
  Ptr<AquaSimNetDevice> recver;
  PktRecvUnit () : pR(-1), pDelay(-1), recver(NULL) {}
};

class AquaSimPropagation : public Object
{
public:
  static TypeId GetTypeId (void);

  virtual std::vector<PktRecvUnit> * ReceivedCopies (Ptr<AquaSimNetDevice> s,
                                                     Ptr<Packet> p,
						     std::vector<Ptr<AquaSimNetDevice> > dList) = 0;
  virtual Time PDelay (Ptr<MobilityModel> s, Ptr<MobilityModel> r);

protected:
  double Rayleigh (double SL);
  double Rayleigh (double d, double f);
  double Thorp (double range, double freq);
  //2.0 version below:
  double Rayleigh2 (double SL);
  double Thorp2 (double range, double freq);
};  //class AquaSimPropagation

}  // namespace ns3

#endif /* AQUA_SIM_PROPAGATION_H */
