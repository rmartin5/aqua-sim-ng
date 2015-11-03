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

#ifndef AQUA_SIM_PROPAGATION_H
#define AQUA_SIM_PROPAGATION_H

#include "ns3/mobility-model.h"
#include "ns3/nstime.h"
#include "ns3/log.h"
#include "ns3/object.h"

#include "aqua-sim-node.h"
#include <vector>

namespace ns3 {

/*
 * \ Underwater propagation model.
 *
 * Calculates the Pr by which the receiver will get a packet sent by
 * the node that applied the tx PacketStamp for a given interface type
 */

class AquaSimNode;

struct PktRecvUnit {
  double pR;
  Time pDelay;
  Ptr<AquaSimNode> recver;
  PktRecvUnit (): pR(-1), pDelay(-1), recver() {}
};

class AquaSimPropagation : public Object
{
public:
  AquaSimPropagation();
  ~AquaSimPropagation ();

  static TypeId GetTypeId (void);

  virtual std::vector<PktRecvUnit> ReceivedCopies (Ptr<AquaSimNode> s,
                                                     Ptr<Packet> p) = 0;
  virtual Time PDelay (Ptr<AquaSimNode> s, Ptr<AquaSimNode> r) = 0;

protected:
  virtual double Rayleigh (double SL) = 0;
  virtual double Thorp (double range, double freq) = 0;
  virtual double distance (Ptr<AquaSimNode> s, Ptr<AquaSimNode> r) = 0;

//Topography *topo;

};  //class AquaSimPropagation

}  // namespace ns3

#endif /* AQUA_SIM_PROPAGATION_H */
