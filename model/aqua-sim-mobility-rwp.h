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

#ifndef AQUA_SIM_MOBILITY_RWP_H
#define AQUA_SIM_MOBILITY_RWP_H

#include "aqua-sim-mobility-pattern.h"

namespace ns3{

/**
 * \ingroup aqua-sim-ng
 *
 * \brief RWP mobility model
 */
class AquaSimMobilityRWP : public AquaSimMobilityPattern {
public:
	AquaSimMobilityRWP();
	static TypeId GetTypeId(void);
	virtual LocationCacheElem GenNewLoc();
	virtual void Init();

private:
	inline void PrepareNextPoint();

private:
	double m_destX;  //the coordinate of next way point
	double m_destY;
	double m_destZ;
	double m_originalX;  //the coordinate of previous way point
	double m_originalY;
	double m_originalZ;

	double m_x;
	double m_y;
	double m_z;

	/* the ratio between dimensions and the distance between
	 * previous way point and next way point
	 */
	double m_ratioX;
	double m_ratioY;
	double m_ratioZ;

	double m_speed; //for speed of the node


	double m_maxSpeed, m_minSpeed;
	double m_maxThinkTime; //the max time for thinking where to go after reaching a dest
	double m_thinkTime;

	double m_distance;     //the distance to next point
	double m_startTime;   //the time when this node start to next point
	double m_thoughtTime; //the time taken by deciding where I will go

	bool   m_duplicatedNamTrace;
};  // class AquaSimMobilityRWP

}  // namespace ns3

#endif /* AQUA_SIM_MOBILITY_RWP_H */
