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
#include "ns3/nstime.h"
#include "ns3/mobility-model.h"
#include "ns3/random-variable-stream.h"

#include "aqua-sim-simple-propagation.h"
#include "aqua-sim-header.h"

#include <stdio.h>
#include <vector>
#include <math.h>

#define _USE_MATH_DEFINES

namespace ns3 {

//const double SOUND_SPEED_IN_WATER = 1500.0;

NS_LOG_COMPONENT_DEFINE ("AquaSimSimplePropagation");
NS_OBJECT_ENSURE_REGISTERED (AquaSimSimplePropagation);

TypeId
AquaSimSimplePropagation::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::AquaSimSimplePropagation")
    .SetParent<AquaSimPropagation>()
    .AddConstructor<AquaSimSimplePropagation>()
  ;
  return tid;
}

AquaSimSimplePropagation::AquaSimSimplePropagation () :
    AquaSimPropagation()
{
}

AquaSimSimplePropagation::~AquaSimSimplePropagation()
{
}

std::vector<PktRecvUnit> *
AquaSimSimplePropagation::ReceivedCopies (Ptr<AquaSimNetDevice> s,
					  Ptr<Packet> p,
					  std::vector<Ptr<AquaSimNetDevice> > dList)
{
  NS_LOG_FUNCTION(this);
  NS_ASSERT(dList.size());

  std::vector<PktRecvUnit>* res = new std::vector<PktRecvUnit>;
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

    dist = senderModel->GetDistanceFrom(recvModel);
    pru.recver = dList[i];
    pru.pDelay = Time::FromDouble(dist / ns3::SOUND_SPEED_IN_WATER,Time::S);
    pru.pR = RayleighAtt(dist, pstamp.GetFreq(), pstamp.GetPt());
    res->push_back(pru);

    NS_LOG_DEBUG("dist:" << dist
		 << " recver:" << pru.recver
		 << " pDelay" << pru.pDelay.GetMilliSeconds()
		 << " pR" << pru.pR
		 << " freq" << pstamp.GetFreq()
		 << " Pt" << pstamp.GetPt());
  }
  return res;
}

double
AquaSimSimplePropagation::RayleighAtt (double dist, double freq, double pT)
{
  if (dist <= 0) return 0;
  /*
  double SL = pT - Thorp(dist, freq);
  return Rayleigh(SL);
  */
  //resorting to 1.0 model for reliability purposes
  NS_LOG_DEBUG("RayleighAtt DUMP: dist(" << dist << ") freq(" << freq << ") pT("
    << pT << ") Rayleigh(" << Rayleigh(dist,freq) << ") pR(" << pT/Rayleigh(dist,freq) << ")");
   return pT/Rayleigh(dist,freq);
}

//2.0 version below:
double
AquaSimSimplePropagation::RayleighAtt2(double dist, double freq, double Pt)
{
	double SL = Pt - Thorp2(dist, freq);
	return Rayleigh2(SL);
}

}  // namespace ns3
