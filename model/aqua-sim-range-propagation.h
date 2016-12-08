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

#ifndef AQUA_SIM_RANGE_PROPAGATION_H
#define AQUA_SIM_RANGE_PROPAGATION_H

#include "aqua-sim-simple-propagation.h"

namespace ns3 {

 /**
  * \ingroup aqua-sim-ng
  *
  * \brief Helper class used to detect which nodes are within propagation range of the sender.
  *
  *
  * Each node within the range specified by packet will receive a copy
  * still calculates attenuatin using rayleigh model
  * this would speed up the simulation if we don't expect
  * a very high accuracy in terms of collisions
  *
  * MUST make sure Pt and tx_range are consistent at the physical layer!!
  *
  * Additional acoutic models are provided.
  */
class AquaSimRangePropagation : public AquaSimSimplePropagation {
public:
  static TypeId GetTypeId (void);
  AquaSimRangePropagation();
  virtual std::vector<PktRecvUnit> * ReceivedCopies (Ptr<AquaSimNetDevice> s,
                 Ptr<Packet> p,
                 std::vector<Ptr<AquaSimNetDevice> > dList);
  double AcousticSpeed(double depth);
  double Urick(Ptr<AquaSimNetDevice> sender, Ptr<AquaSimNetDevice> recver);

  void SetBandwidth(double bandwidth);
  void SetTemp(double temp);
  void SetSalinity(double salinity);
  void SetNoiseLvl(double noiseLvl);

  void Initialize();
  virtual void SetTraceValues(double temp, double salinity, double noiseLvl);

private:
  double m_bandwidth;
  double m_temp;
  double m_salinity;
  double m_noiseLvl;
};  // class AquaSimRangePropagation

}  // namespace ns3

#endif /* AQUA_SIM_RANGE_PROPAGATION_H */
