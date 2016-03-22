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

#include "aqua-sim-mobility-rwp.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
#include "ns3/double.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimMobilityRWP");
NS_OBJECT_ENSURE_REGISTERED(AquaSimMobilityRWP);


AquaSimMobilityRWP::AquaSimMobilityRWP()
{
}

void
AquaSimMobilityRWP::Init()
{
	//obsolete
	LocationCacheElem elem = m_lc->GetLastLoc();
	m_destX = elem.m_loc.x;
	m_destY = elem.m_loc.y;
	m_destZ = elem.m_loc.z;
	m_speed = elem.m_sp.GetSpeed();
	PrepareNextPoint();
	m_startTime = Simulator::Now().ToDouble(Time::S);
	m_duplicatedNamTrace = false;
}

TypeId
AquaSimMobilityRWP::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimMobilityRWP")
    .SetParent<AquaSimMobilityPattern>()
    .AddConstructor<AquaSimMobilityRWP>()
    .AddAttribute ("MaxSpeed", "Set the max speed.",
      DoubleValue(0.0),
      MakeDoubleAccessor(&AquaSimMobilityRWP::m_maxSpeed),
      MakeDoubleChecker<double>())
    .AddAttribute ("MinSpeed", "Set the min speed.",
      DoubleValue(0.0),
      MakeDoubleAccessor(&AquaSimMobilityRWP::m_minSpeed),
      MakeDoubleChecker<double>())
    .AddAttribute ("MaxThinkTime", "Set the max think time.",
      DoubleValue(0.0),
      MakeDoubleAccessor(&AquaSimMobilityRWP::m_maxThinkTime),
      MakeDoubleChecker<double>())
    ;
  return tid;
}

void
AquaSimMobilityRWP::PrepareNextPoint()
{
	Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();

	m_speed = rand->GetValue(m_minSpeed, m_maxSpeed);

	m_originalX = m_destX;
	m_originalY = m_destY;
	m_originalZ = m_destZ;
	//calculate the next way point
	m_destX = rand->GetValue(m_minBound.x, m_maxBound.x);
	m_destY = rand->GetValue(m_minBound.y, m_maxBound.y);
	m_destZ = rand->GetValue(m_minBound.z, m_maxBound.z);

	m_distance = sqrt( (m_destX - m_originalX)*(m_destX - m_originalX)
		+(m_destY - m_originalY)*(m_destY - m_originalY)
		+(m_destZ - m_originalZ)*(m_destZ - m_originalZ) );

	m_ratioX = (m_destX - m_originalX)/m_distance;
	m_ratioY = (m_destY - m_originalY)/m_distance;
	m_ratioZ = (m_destZ - m_originalZ)/m_distance;

	m_thinkTime = rand->GetValue(0, m_maxThinkTime);
}


LocationCacheElem
AquaSimMobilityRWP::GenNewLoc()
{
	LocationCacheElem ret;
	//the time since node start to move from previous way point
	double passed_len = m_speed*(Simulator::Now().ToDouble(Time::S) - m_startTime);

	if( passed_len < m_distance )
	{
		m_x = m_originalX + passed_len * m_ratioX;
		m_y = m_originalY + passed_len * m_ratioY;
		m_z = m_originalZ + passed_len * m_ratioZ;
	}
	else{
		//now I must arrive at the way point

		if( (Simulator::Now().ToDouble(Time::S) - m_startTime - m_distance/m_speed) < m_thinkTime )
		{
			//I am still thinking of that where I will go
			m_x = m_destX;
			m_y = m_destY;
			m_z = m_destZ;
		}
		else{
			//I am on the way to next way point again.
			m_startTime = m_startTime + m_distance/m_speed + m_thinkTime;
			PrepareNextPoint();
			return GenNewLoc();
		}
	}
	ret.Set(m_x, m_y, m_z, m_speed*m_ratioX, m_speed*m_ratioY, m_speed*m_ratioZ);

	return ret;
}
