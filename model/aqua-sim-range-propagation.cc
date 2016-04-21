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

#include "aqua-sim-range-propagation.h"
#include "aqua-sim-header.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("AquaSimRangePropagation");
NS_OBJECT_ENSURE_REGISTERED (AquaSimRangePropagation);

AquaSimRangePropagation::AquaSimRangePropagation()
{
}

/**
* only nodes within range will receive a copy
*/
std::vector<PktRecvUnit> *
AquaSimRangePropagation::ReceivedCopies (Ptr<AquaSimNetDevice> s,
               Ptr<Packet> p,
               std::vector<Ptr<AquaSimNetDevice> > dList)
{
  NS_LOG_FUNCTION(this);
  NS_ASSERT(dList.size());

	std::vector<PktRecvUnit> * res = new std::vector<PktRecvUnit>;
	//find all nodes which will receive a copy
	PktRecvUnit pru;
	double dist = 0;

  AquaSimPacketStamp pstamp;
  p->PeekHeader(pstamp);

  Ptr<Object> sObject = s->GetNode();
  Ptr<MobilityModel> senderModel = sObject->GetObject<MobilityModel> ();

  unsigned i = 0;
  std::vector<Ptr<AquaSimNetDevice> >::iterator it = dList.begin();
  for(; it != dList.end(); it++, i++)
  {
    Ptr<Object> rObject = dList[i]->GetNode();
    Ptr<MobilityModel> recvModel = rObject->GetObject<MobilityModel> ();
    if (std::fabs(recvModel->GetPosition().x - senderModel->GetPosition().x) > pstamp.GetTxRange())
      break;
    if ( (dist = senderModel->GetDistanceFrom(recvModel)) > pstamp.GetTxRange() )
      continue;

		pru.recver = dList[i];
		pru.pDelay = Time::FromDouble(dist / ns3::SOUND_SPEED_IN_WATER,Time::S);
		pru.pR = RayleighAtt(dist, pstamp.GetFreq(), pstamp.GetPt());
		res->push_back(pru);

    NS_LOG_DEBUG("AquaSimRangePropagation::ReceivedCopies: Sender("
    << s->GetAddress() << ") Recv(" << (pru.recver)->GetAddress()
    << ") dist(" << dist << ") pDelay(" << pru.pDelay.GetMilliSeconds()
    << ") pR(" << pru.pR << ")" << " Pt(" << pstamp.GetPt() << ")\n");
	}

	return res;
}
