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

#ifndef AQUA_SIM_MOBILITY_PATTERN_H
#define AQUA_SIM_MOBILITY_PATTERN_H


#include <cmath>
#include <vector>
#include <stdexcept>

#include "ns3/vector.h"
#include "ns3/mobility-model.h"
#include "ns3/timer.h"


#ifndef PI
#define PI 3.1415926
#endif

// Aqua Sim Mobility Pattern

namespace ns3{

/* Some redudency within these classes, compared to ns3,
but for this port this ns2 version will suffix
*/

class AquaSimMobilityPattern;

class AquaSimPosUpdateHelper : public Timer {
public:
  AquaSimPosUpdateHelper(AquaSimMobilityPattern *mP) : Timer() {
    m_mP = mP;
  }
  void Expire(void);
private:
  AquaSimMobilityPattern * m_mP;
  Timer m_updateIntv;
};  //class AquaSimPosUpdateHelper

/*  **NOTE: Vector does exactly this **
class Location3D {
private:
  double m_X, m_Y, m_Z;
public:
  double & X() { return m_X; }
  double & Y() { return m_Y; }
  double & Z() { return m_Z; }
  Location3D(double X = 0, double Y = 0, double Z = 0) : m_X(X), m_Y(Y), m_Z(Z) {}
  void Set(double X, double Y, double Z) {
    m_X = X; m_Y = Y; m_Z = Z;
  }
};  // class Location3D
*/

class Speed{
private:
  Vector m_speedVect;
public:
  Speed(Vector speedVect = Vector(0,0,0) ) : m_speedVect(speedVect) {}
  void Set(Vector speedVect) {
    m_speedVect = speedVect;
  }
  Vector GetSpeedVect() { return m_speedVect; }
  double GetSpeed() {
    return std::sqrt(m_speedVect.x*m_speedVect.x +
                      m_speedVect.y*m_speedVect.y +
                      m_speedVect.z*m_speedVect.z);
  }
};  // class Speed


struct LocationCacheElem {
  Vector m_loc;
  Speed m_sp;
  LocationCacheElem() : m_loc(Vector(0,0,0)) {};
  void Set(double X, double Y, double Z, double dX, double dY, double dZ) {
	  m_loc = Vector(X,Y,Z);
	  m_sp.Set(Vector(dX,dY,dZ));
  }
};

class LocationCache{
private:
  std::vector<LocationCacheElem> m_locations;
  size_t m_bIndex; //beginning index;
  size_t m_size;	//end index
  double m_interval;
  double m_fstUptTime;

protected:
  bool	Empty() { return m_size == 0; }
  bool	Full() { return m_size == Capacity(); }

public:
  LocationCache(double duration, double interval,
	  double X, double Y, double Z,
	  double dX, double dY, double dZ);

  size_t Capacity();
  //double FirstUpdateTime();
  double LastUpdateTime();
  bool InRange(double t);

  LocationCacheElem GetLocByTime(double t);
  void AddNewLoc(const LocationCacheElem &lce);
  LocationCacheElem GetLastLoc();
  /*
  void pop_front();
  const LocationCacheElem & front();
  */
};  // class LocationCache


/**
* base class of mobility pattern
* AquaSimNode will possess a member of this type
*/

class AquaSimMobilityPattern : public MobilityModel {
public:
  AquaSimMobilityPattern();
  virtual ~AquaSimMobilityPattern();
  static TypeId GetTypeId(void);

  void Start();
  void HandleLocUpdate(); //a public interface for AquaSimPosUpdateHelper
  double UptIntv() { return m_updateInterval; };

  //tell future position
  LocationCacheElem GetLocByTime(double t);
  void SetBounds(double minx,double miny,double minz,
                  double maxx, double maxy, double maxz);
  void SetBounds(Vector min, Vector max);
  void SetVelocity(Vector vector);

protected:
  virtual LocationCacheElem GenNewLoc(); /* the actual method that each
					  * derived class need to overload to
					  * update node's position;
					  */
  /*initialize mobility pattern here*/
  virtual void Init();

  //void UpdateGridKeeper();
  void RestrictLocByBound(LocationCacheElem &lce);
  void NamLogMobility(double t, LocationCacheElem &lce);
private:
  bool BounceByEdge(double coord, double speed,
		double bound, bool lowerBound);


  //inherited functions
  virtual Vector DoGetPosition (void) const;
  virtual void DoSetPosition (const Vector &position);
  virtual Vector DoGetVelocity (void) const;

protected:  
  LocationCache *m_lc;
  double m_updateInterval;
  AquaSimPosUpdateHelper m_posUpdateHelper;

  //topography
  Vector m_minBound;
  Vector m_maxBound;
};  //class AquaSimMobilityPattern

} // namespace ns3

#endif /* AQUA_SIM_MOBILITY_PATTERN_H */
