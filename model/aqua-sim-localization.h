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

#ifndef AQUA_SIM_LOCALIZATION_H
#define AQUA_SIM_LOCALIZATION_H

#include "ns3/vector.h"
#include "ns3/object.h"
#include "ns3/nstime.h"
#include "aqua-sim-net-device.h"
#include <list>

namespace ns3 {

struct LocalizationStructure{
  double m_RSSI;  //Received Signal Strength Indicator
  Vector m_AoA;   //Angle of Arrival
  Time m_TDoA;    //Time Difference of Arrival
  Time m_ToA;     //Time of Arrival
  Vector m_knownLocation;   //last known location of node
  int m_nodeID;
};

class AquaSimLocalization : public Object {
public:
  AquaSimLocalization();
  static TypeId GetTypeId(void);
  void SetDevice(Ptr<AquaSimNetDevice> device);
  void SetPosition(Vector nodePosition);
  void SetPr(double pr);

  virtual void Recv(Ptr<Packet> p);
  virtual void SendLoc();

protected:
  std::list<LocalizationStructure> GetLocalizationList();
  void ClearLocalizationList();
  virtual void Lateration();
  virtual Vector GetAngleOfArrival(Ptr<Packet> p);  //should be overloaded

  Time m_localizationRefreshRate;
  Vector m_nodePosition;  //last known position, may vary due to mobility
  double m_pr;
  std::list<LocalizationStructure> m_localizationList;
  Ptr<AquaSimNetDevice> m_device;

}; // class AquaSimLocalization

} // namespace ns3

#endif /* AQUA_SIM_LOCALIZATION_H */
