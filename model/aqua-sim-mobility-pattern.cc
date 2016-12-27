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

#include "ns3/log.h"
#include "ns3/double.h"
#include "aqua-sim-mobility-pattern.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimMobilityPattern");

AquaSimPosUpdateHelper::~AquaSimPosUpdateHelper()
{
  delete m_mP;
  m_mP=0;
}

void
AquaSimPosUpdateHelper::Expire()
{
  m_mP->HandleLocUpdate();
  if (m_updateIntv.IsRunning()) {
      m_updateIntv.Cancel();
  }
  m_updateIntv.Schedule(Seconds(m_mP->UptIntv()));
}

/**
* @param duration trajectory during this duration time can be cached
* @param interval the time interval between two adjacent locations
* 					in the trajectory, i.e., location update interval
* @param X	   	node's X coordinate
* @param Y		node's Y coordinate
* @param Z		node's Z coordinate
*/
LocationCache::LocationCache(double duration, double interval,
			     double X, double Y, double Z,
			     double dX, double dY, double dZ) :
  m_locations(1 + size_t(ceil(duration / interval))),
  m_bIndex(0), m_size(1)
{
  m_interval = interval;
  m_locations[0].m_loc = Vector(X, Y, Z);
  m_locations[0].m_sp = Speed(Vector(dX, dY, dZ));
}

/**
* @return the max number of locations can be stored
*/
inline size_t
LocationCache::Capacity() {
  return m_locations.size();
}

inline double
LocationCache::LastUpdateTime() {
  return Empty() ? -1 : m_fstUptTime + m_interval * (m_size - 1);
}

bool
LocationCache::InRange(double t) {
  return t >= m_fstUptTime &&
          t < (Simulator::Now() + m_interval * (Capacity() - 1));
}

/**
* append a new location in the cache
* the time gap between this new one and the last one in
* the cache must equal to the attribute interval
*
* @param loc the new location
*/
void
LocationCache::AddNewLoc(const LocationCacheElem &lce) {
  m_locations[(m_bIndex + m_size) % Capacity()] = lce;
  if (Full())
    m_bIndex++;
  else
    m_size++;
}

LocationCacheElem
LocationCache::GetLocByTime(double t) {
  if (t < m_fstUptTime || t > LastUpdateTime()) {
    throw std::out_of_range("LocationCache::GetLocByTime");
  }

  return m_locations[m_bIndex + size_t((t - m_fstUptTime) / m_interval)];
}

LocationCacheElem
LocationCache::GetLastLoc()
{
  return m_locations[m_size];
}

/*****
* implementations of AquaSimMobilityPattern
*****/
NS_OBJECT_ENSURE_REGISTERED(AquaSimMobilityPattern);

AquaSimMobilityPattern::AquaSimMobilityPattern() :
  m_lc(NULL), m_posUpdateHelper(this)
{
}

TypeId
AquaSimMobilityPattern::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimMobilityPattern")
    .SetParent<MobilityModel>()
    .AddConstructor<AquaSimMobilityPattern>()
    .AddAttribute ("UpdateInt", "Set the update interval. Default is 0.001.",
      DoubleValue(0.001),
      MakeDoubleAccessor(&AquaSimMobilityPattern::m_updateInterval),
      MakeDoubleChecker<double>())
    .AddAttribute ("MinBound", "Minimum topography boundry (x,y,z).",
      Vector3DValue(),
      MakeVector3DAccessor(&AquaSimMobilityPattern::m_minBound),
      MakeVector3DChecker())
    .AddAttribute ("MaxBound", "Maximum topography boundry (x,y,z).",
      Vector3DValue(),
      MakeVector3DAccessor(&AquaSimMobilityPattern::m_maxBound),
      MakeVector3DChecker())
    ;
  return tid;
}


AquaSimMobilityPattern::~AquaSimMobilityPattern() {
  delete m_lc;
}

/**
* mobility pattern starts to work, i.e., the host node starts to move
*/
void
AquaSimMobilityPattern::Start() {
  Vector position = Vector(0,0,0);
  Vector speed = Vector(0,0,0);
  if (NULL != m_lc) {
    position = m_lc->GetLastLoc().m_loc;
    speed = m_lc->GetLastLoc().m_sp.GetSpeedVect();
    delete m_lc;
    m_lc = NULL;
  }

  m_lc = new LocationCache(5, m_updateInterval,
                            position.x,position.y,position.z,
                            speed.x, speed.y, speed.z);
  Init();
  m_posUpdateHelper.Expire();
}


/**
* initiate aquasim mobility pattern, derived class
* should overload this one
*/
void AquaSimMobilityPattern::Init() {
return;
}

/**
* a dummy one
*/
LocationCacheElem
AquaSimMobilityPattern::GenNewLoc() {
  NS_LOG_FUNCTION(this << "A Dummy one! Shouldn't call this function!");
  LocationCacheElem newLoc;
  return newLoc;
}

/*
void
AquaSimMobilityPattern::UpdateGridKeeper()
{
//If we need this ??? new_moves involves destX, Y
//but we don't know them in many mobility patterns
	if(GridKeeper::instance() != NULL) {
		GridKeeper::instance()->new_moves(node);
	}
}*/

/**
* the interface for AquaSimPosUpdateHelper to call
*/
void
AquaSimMobilityPattern::HandleLocUpdate() {
  LocationCacheElem e;
  while (m_lc->LastUpdateTime() < Simulator::Now().ToDouble(Time::S)) {
    e = GenNewLoc();
    RestrictLocByBound(e);
    m_lc->AddNewLoc(e);
  }

  //find the time closest one to NOW
  double t = Simulator::Now().ToDouble(Time::S);
  e = GetLocByTime(t);
  //Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
  //double oldX = GetPosition().x;
  SetPosition(Vector(e.m_loc.x,e.m_loc.y,e.m_loc.z));
  SetVelocity(Vector(e.m_sp.GetSpeedVect().x,
                        e.m_sp.GetSpeedVect().y,
                        e.m_sp.GetSpeedVect().z));
  //m_node->speed() = e.m_sp.GetSpeed(); //already handled by model->GetVelocity()

  /*if (oldX != model->GetPosition().x)			// adjust node to satisfy topography use here T()
    m_node->T()->UpdateNodesList(m_node, oldX); //X_ is the key value of SortList
  */      //Topography not supported

  //updateGridKeeper(); //do I really need this?!!!!!
      // TODO this shoudl be a callback... see MobilityModel class...
      //m_device->SetPositionUpdateTime(Simulator::Now());
  //record the movement
  //namLogMobility(m_lc->lastUpdateTime(), e);
}


LocationCacheElem
AquaSimMobilityPattern::GetLocByTime(double t) {
  if (!m_lc->InRange(t)) {
    throw std::out_of_range("AquaSimMobilityPattern::GetLocByTime");
  }

  LocationCacheElem newLce;
  while (m_lc->LastUpdateTime() < t) {
    newLce = GenNewLoc();
    RestrictLocByBound(newLce);
    m_lc->AddNewLoc(newLce);
  }

  return m_lc->GetLocByTime(t);
}


/**
* log the position change in nam file
*	Not implemented currently
*
* @param vx velocity projected to x axis
* @param vy velocity projected to y axis
* @param interval the interval to the next update
*/
void AquaSimMobilityPattern::NamLogMobility(double t, LocationCacheElem &lce)
{
  NS_LOG_FUNCTION(this << "Not supported.");
  /*
	node->namlog("n -t %f -s %d -x %f -y %f -z %f -U %f -V %f -T %f",
	             t,
	             node->nodeid(),
	             lce.m_loc.x, lce.m_loc.y, lce.m_loc.z,
	             lce.m_sp.GetSpeedVect().x, lce.m_sp.GetSpeedVect().y,
	             m_updateInterval);
  */
}

void
AquaSimMobilityPattern::SetBounds(double minx,double miny,double minz,
                                  double maxx, double maxy, double maxz)
{
  SetBounds(Vector(minx,miny,minz),Vector(maxx,maxy,maxz));
}

void
AquaSimMobilityPattern::SetBounds(Vector min, Vector max)
{
  m_minBound = min;
  m_maxBound = max;
}

/**
* update node's position. if it's out of bound
* We simply bounce the node by the corresponding edge
*/
void
AquaSimMobilityPattern::RestrictLocByBound(LocationCacheElem &lce)
{
  /*
   * //should probably write a CubicPositionAllocator class to clean this up
   */
  bool recheck = true;
  //Ptr<CubicPositionAllocator> T = m_node->T();

  while (recheck) {
    recheck = false;
    recheck = recheck || BounceByEdge(lce.m_loc.x, lce.m_sp.GetSpeedVect().x, m_minBound.x, true);
    recheck = recheck || BounceByEdge(lce.m_loc.x, lce.m_sp.GetSpeedVect().x, m_maxBound.x, false);
    recheck = recheck || BounceByEdge(lce.m_loc.y, lce.m_sp.GetSpeedVect().y, m_minBound.y, true);
    recheck = recheck || BounceByEdge(lce.m_loc.y, lce.m_sp.GetSpeedVect().y, m_maxBound.y, false);
    recheck = recheck || BounceByEdge(lce.m_loc.z, lce.m_sp.GetSpeedVect().z, m_minBound.z, true);
    recheck = recheck || BounceByEdge(lce.m_loc.z, lce.m_sp.GetSpeedVect().z, m_maxBound.z, false);
  }
}

/**
* bounce the node by the edge if it is out of range
*
* @param coord 1D coordinate, could be one of X, Y, Z
* @param speed speed projected to the same axis as coord
* @param bound the value of lower/upper bound
* @param lowerBound to specify if bound is lower Bound (true),
* 				otherwise, it's upper bound
*
* @return  true for coord is changed, false for not
*/
bool
AquaSimMobilityPattern::BounceByEdge(double coord, double dspeed,
double bound, bool lowerBound) {
  if ((lowerBound && (coord < bound)) /*below lower bound*/
	  || (!lowerBound && (coord > bound)) /*beyond upper bound*/) {
    coord = bound + bound - coord;
    dspeed = -dspeed;
    return true;
  }

  return false;
}

void
AquaSimMobilityPattern::SetVelocity(Vector vector)
{
  m_lc->GetLastLoc().m_sp.Set(vector);
}

Vector
AquaSimMobilityPattern::DoGetPosition (void) const
{
  return m_lc->GetLastLoc().m_loc;
}

void
AquaSimMobilityPattern::DoSetPosition (const Vector &position)
{
  LocationCacheElem e = GenNewLoc();
  e.Set(position.x,position.y,position.z,0,0,0);
  //e.m_sp = m_lc->GetLastLoc().m_sp;
  RestrictLocByBound(e);
  m_lc->AddNewLoc(e);
}

Vector
AquaSimMobilityPattern::DoGetVelocity (void) const
{
  return m_lc->GetLastLoc().m_sp.GetSpeedVect();
}

void AquaSimMobilityPattern::DoDispose()
{
  delete m_lc;
  m_lc=0;
}
