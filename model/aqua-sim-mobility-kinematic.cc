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

#include "aqua-sim-mobility-kinematic.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
#include "ns3/double.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimMobilityKinematic");
NS_OBJECT_ENSURE_REGISTERED(AquaSimMobilityKinematic);

AquaSimMobilityKinematic::AquaSimMobilityKinematic()
{
	Ptr<NormalRandomVariable> rand = CreateObject<NormalRandomVariable> ();

	m_k1 = rand->GetValue(PI, 0.1*PI);
	m_k2 = rand->GetValue(PI, 0.1*PI);
	m_k3 = rand->GetValue(2*PI, 0.2*PI);
	m_k4 = rand->GetValue(0, 0.2);
	m_k5 = rand->GetValue(0, 0.2);
	m_lambda = rand->GetValue(6, 0.3);
	m_v = rand->GetValue(1, 0.1);
}

TypeId
AquaSimMobilityKinematic::GetTypeId(void)
{
	Ptr<NormalRandomVariable> rand = CreateObject<NormalRandomVariable> ();

  static TypeId tid = TypeId("ns3::AquaSimMobilityKinematic")
    .SetParent<AquaSimMobilityPattern>()
    .AddConstructor<AquaSimMobilityKinematic>()
    .AddAttribute ("K1", "Kinematic 1",
      DoubleValue(rand->GetValue(PI, 0.1*PI)),
      MakeDoubleAccessor(&AquaSimMobilityKinematic::m_k1),
      MakeDoubleChecker<double>())
    .AddAttribute ("K2", "Kinematic 2",
      DoubleValue(rand->GetValue(PI, 0.1*PI)),
      MakeDoubleAccessor(&AquaSimMobilityKinematic::m_k2),
      MakeDoubleChecker<double>())
    .AddAttribute ("K3", "Kinematic 3",
      DoubleValue(rand->GetValue(2*PI, 0.2*PI)),
      MakeDoubleAccessor(&AquaSimMobilityKinematic::m_k3),
      MakeDoubleChecker<double>())
    .AddAttribute ("K4", "Kinematic 4",
      DoubleValue(rand->GetValue(0, 0.2)),
      MakeDoubleAccessor(&AquaSimMobilityKinematic::m_k4),
      MakeDoubleChecker<double>())
    .AddAttribute ("K5", "Kinematic 5",
      DoubleValue(rand->GetValue(0, 0.2)),
      MakeDoubleAccessor(&AquaSimMobilityKinematic::m_k5),
      MakeDoubleChecker<double>())
    .AddAttribute ("Lambda", "Lambda",
      DoubleValue(rand->GetValue(6, 0.3)),
      MakeDoubleAccessor(&AquaSimMobilityKinematic::m_lambda),
      MakeDoubleChecker<double>())
    .AddAttribute ("V", "V",
      DoubleValue(rand->GetValue(1, 0.1)),
      MakeDoubleAccessor(&AquaSimMobilityKinematic::m_v),
      MakeDoubleChecker<double>())
    ;
  return tid;
}

void
AquaSimMobilityKinematic::Init()
{
	//this is obsolete
	m_lce.Set(0,0,0, 0,0,0);
			//node->X(), node->Y(), node->Z(),
			//node->dX(), node->dY(), node->dZ());
}

LocationCacheElem
AquaSimMobilityKinematic::GenNewLoc()
{
	double interval = UptIntv();

    //start to calculate the position after the coming interval
	double yVel = m_k5-m_lambda*m_v*cos(m_k2*m_lce.m_loc.x)*sin(m_k3*m_lce.m_loc.y);
	double xVel = m_k1*m_lambda*m_v*sin(m_k2*m_lce.m_loc.x)*cos(m_k3*m_lce.m_loc.y)
					+ m_k4 + m_k1*m_lambda*cos(2*m_k1*Simulator::Now().ToDouble(Time::S));
	double x = m_lce.m_loc.x + xVel*interval;
	double y = m_lce.m_loc.y + yVel*interval;

	m_lce.Set(x,y,m_lce.m_loc.z, xVel,yVel,m_lce.m_sp.GetSpeedVect().z);

	return m_lce;
}
