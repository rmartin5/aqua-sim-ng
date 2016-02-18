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

  AquaSimHeader asHeader;
  p->PeekHeader(asHeader);

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
    pru.pR = RayleighAtt(dist, asHeader.GetFreq(), asHeader.GetPt());
    res->push_back(pru);

    NS_LOG_DEBUG("dist:" << dist
		 << " recver:" << pru.recver
		 << " pDelay" << pru.pDelay
		 << " pR" << pru.pR);
  }

  return res;
}
    
double
AquaSimSimplePropagation::RayleighAtt (double dist, double freq, double pT)
{
  if (dist <= 0) return 0;
  double SL = pT - Thorp(dist, freq);
  return Rayleigh(SL);
}


/**
 * @param SL sound level in dB
 * @return receiving power in J
 */
double
AquaSimSimplePropagation::Rayleigh (double SL)
{
  double mPr = std::pow(10, SL/20 - 6);  //signal strength (pressure in Pa)
  double segma = pow(mPr, 2) * 2 / M_PI;

  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();

  return -2 * segma * std::log(m_rand->GetValue());
}

/**
 * Thorp equation, calculating attenuation according
 *
 * @param dist  distance that signal travels
 * @param freq  central frequency
 * @return attentuation in dB *
 */
double
AquaSimSimplePropagation::Thorp (double dist, double freq)
{
  double k, spre, abso;

  if (dist <= 500) {
    k = 3;
  } else if (dist <= 2000) {
    k = 2;
  } else {
    k = 1.5;
  }

  spre = 10 * k * log10(dist);

  abso = dist/1000 * (0.11 * pow(freq,2) / (1 + pow(freq,2) )
                + 44 * pow(freq,2) / (4100 + pow(freq,2) )
                + 0.000275 * pow(freq,2) + 0.003 );
  
  return spre + abso;
}

}  // namespace ns3

