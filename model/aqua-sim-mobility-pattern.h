//...

/******

#ifndef AQUA_SIM_MOBILITY_PATTERN_H
#define AQUA_SIM_MOBILITY_PATTERN_H


#include <string.h>
#include <cmath>
#include <vector>
//#include <random.h>
#include <stdexcept>
//#include "ns3/vector.h"
#include "ns3/nstime.h"
#include "ns3/mobility-model.h"		//redundant

#include "aqua-sim-node.h"


#ifndef PI
#define PI 3.1415926
#endif

// Aqua Sim Mobility Pattern

namespace ns3{

/* Some redudency within these classes, compared to ns3,
but for this port this ns2 version will suffix
*/
/******

class AquaSimMobilityPattern;

class AquaSimPosUpdateHelper : public Timer {

public:
  AquaSimPosUpdateHelper(AquaSimMobilityPattern *mP) : Timer() {
    m_mP = mP;
  }
private:
  void Expire(void);	// this can just use MobilityModel functions...
  AquaSimMobilityPattern * m_mP;
  Timer m_updateIntv;
};  //class AquaSimPosUpdateHelper

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

class Speed{
private:
  double m_dX, m_dY, m_dZ;
public:
  Speed(double dX = 0, double dY = 0, double dZ = 0) : m_dX(dX), m_dY(dY), m_dZ(dZ){}
  void Set(double dX, double dY, double dZ) {
    m_dX = dX; m_dY = dY; m_dZ = dZ;
  }
  double GetSpeed() {
    return std::sqrt(m_dX*m_dX + m_dY*m_dY + m_dZ*m_dZ);
  }
  double & dX() { return m_dX; }
  double & dY() { return m_dY; }
  double & dZ() { return m_dZ; }
};  // class Speed


struct LocationCacheElem {
  Location3D loc;
  Speed sp;
  LocationCacheElem() : loc(0, 0, 0) {};
  void Set(double X, double Y, double Z, double dX, double dY, double dZ) {
	  loc.Set(X, Y, Z);
	  sp.Set(dX, dY, dZ);
  }
};

class LocationCache{
private:
  std::vector<LocationCacheElem> locations;
  size_t	m_bIndex; //beginning index;
  size_t	m_size;	//end index
  Time	m_interval;
  Time	m_fstUptTime;

protected:
  bool	Empty() { return m_size == 0; }
  bool	Full() { return m_size == Capacity(); }

public:
  LocationCache(Time duration, Time interval,
	  double X, double Y, double Z,
	  double dX, double dY, double dZ);

  size_t Capacity();
  //Time FirstUpdateTime();
  Time LastUpdateTime();
  bool InRange(Time t);

  LocationCacheElem GetLocByTime(Time t);
  void AddNewLoc(const LocationCacheElem &lce);
  /*
  LocationCacheElem getLastLoc();
  void pop_front();
  const LocationCacheElem & front();
  */
/******

};  // class LocationCache


/**
* base class of mobility pattern
* AquaSimNode will possess a member of this type
*/
/******

class AquaSimMobilityPattern : public Object{
public:
  AquaSimMobilityPattern();
  virtual ~AquaSimMobilityPattern();
  static TypeId GetTypeId(void);

  void Start();
  void SetNode(AquaSimNode *n);
  void HandleLocUpdate(); //a public interface for AquaSimPosUpdateHelper
  Time UptIntv() { return m_updateInterval; };

  //tell future position
  LocationCacheElem GetLocByTime(Time t);

protected:
  virtual LocationCacheElem GenNewLoc(); /* the actual method that each
					  * derived class need to overload to
					  * update node's position;
					  */
  /*initialize mobility pattern here*/ /******

  virtual void Init();

  //void UpdateGridKeeper();
  void RestrictLocByBound(LocationCacheElem &lce);
  //void namLogMobility(Time t, LocationCacheElem &lce);	//Not currently implemented
private:
  bool BounceByEdge(double &coord, double &speed,
		double bound, bool lowerBound);

protected:
  AquaSimNode *m_node;
  LocationCache *m_lc;
  Time m_updateInterval;
  AquaSimPosUpdateHelper m_posUpdateHelper;
};  //class AquaSimMobilityPattern

} // namespace ns3

#endif /* AQUA_SIM_MOBILITY_PATTERN_H */
