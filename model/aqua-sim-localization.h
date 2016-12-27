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
  //Vector m_nodeSpeed;     //estimated speed vector of node
  double m_nodeConfidence;  //location confidence of node
  int m_nodeID;
};

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Basic components for underwater localization
 */
class AquaSimLocalization : public Object {
public:
  AquaSimLocalization();
  static TypeId GetTypeId(void);
  void SetDevice(Ptr<AquaSimNetDevice> device);
  void SetPosition(Vector nodePosition);
  void SetPr(double pr);

  virtual void Recv(Ptr<Packet> p) = 0;
  virtual void SendLoc() = 0;

protected:
  std::list<LocalizationStructure> GetLocalizationList();
  void ClearLocalizationList();
  virtual void Lateration() = 0;
  virtual Vector GetAngleOfArrival(Ptr<Packet> p) = 0;
  double EuclideanDistance2D(Vector2D s, Vector2D r);
  double EuclideanDistance3D(Vector s, Vector r);
  double LocationError(Vector s, Vector r, double estRange);
  void DoDispose();

  Time m_localizationRefreshRate;
  Vector m_nodePosition;  //last known position, may vary due to mobility
  double m_pr;
  std::list<LocalizationStructure> m_localizationList;
  Ptr<AquaSimNetDevice> m_device;

}; // class AquaSimLocalization

/**
 * \brief Range-based underwater localization
 *
 * Z. Zhou, Z. Peng, J. H. Cui, Z. Shi and A. Bagtzoglou, "Scalable Localization with
 *  Mobility Prediction for Underwater Sensor Networks," in IEEE Transactions on
 *  Mobile Computing, vol. 10, no. 3, pp. 335-348, March 2011.
 */
class AquaSimRBLocalization : public AquaSimLocalization {
public:
  AquaSimRBLocalization();
  static TypeId GetTypeId(void);

  void Recv(Ptr<Packet> p);
  void SendLoc();

  void SetReferenceNode(bool ref);
  void SetConfidenceThreshold(double confidence);
  void SetLocalizationThreshold(double locThreshold);

protected:
  void Lateration();
  Vector GetAngleOfArrival(Ptr<Packet> p);

private:
  bool m_referenceNode;
  double m_confidence;  //estimated location confidence
  double m_confidenceThreshold;
  int m_localizationThreshold;
}; // class AquaSimRBLocalization

} // namespace ns3

#endif /* AQUA_SIM_LOCALIZATION_H */
