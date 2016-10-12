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

#ifndef AQUA_SIM_MOBILITY_KINEMATIC_H
#define AQUA_SIM_MOBILITY_KINEMATIC_H


#include "aqua-sim-mobility-pattern.h"

namespace ns3{

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Kinematic Mobility Model
 */
class AquaSimMobilityKinematic : public AquaSimMobilityPattern {
public:
	AquaSimMobilityKinematic();
	static TypeId GetTypeId(void);
	virtual	LocationCacheElem GenNewLoc();
	virtual void Init();

private:
	double m_k1;
	double m_k2;
	double m_k3;
	double m_k4;
	double m_k5;
	double m_lambda;
	double m_v;

	LocationCacheElem m_lce;
};  // class AquaSimMobilityKinematic

}  // namespace ns3

#endif /* AQUA_SIM_MOBILITY_KINEMATIC_H */
