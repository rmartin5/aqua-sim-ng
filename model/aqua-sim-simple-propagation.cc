/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 UWSN Lab at the University of Connecticut
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

#include "aqua-sim-simple-propagation.h"

#include <stdio.h>
#include <vector>
#include <math.h>

#define _USE_MATH_DEFINES

namespace ns3 {

const double SOUND_SPEED_IN_WATER = 1500.0;

NS_LOG_COMPONENT_DEFINE ("AquaSimSimplePropagation");
NS_OBJECT_ENSURE_REGISTERED (AquaSimSimplePropagation);

TypeId
AquaSimSimplePropagation::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::AquaSimSimplePropagation")
    .SetParent<AquaSimPropagation>()
  ;
  return tid;
}

AquaSimSimplePropagation::AquaSimSimplePropagation ()
{

}

std::vector<PktRecvUnit>*
AquaSimSimplePropagation::ReceivedCopies (Ptr<AquaSimNode> s, Ptr<Packet> p, std::vector<Ptr<AquaSimNetDevice> > dList)
{
  std::vector<PktRecvUnit>* res = new std::vector<PktRecvUnit>;
  //find all nodes which will receive a copy
  PktRecvUnit pru;
  Ptr<AquaSimNode> node = NULL;
  double dist = 0;

  AquaSimHeader asHeader;
  p->PeekHeader(asHeader);

  Ptr<AquaSimNode> x, y;

  std::vector<Ptr<AquaSimNetDevice> >::const_iterator it = dList.begin();	//FIXME this is buggy...
  for(; it != dList.end(); ++it)
  {
    dist = x->DistanceFrom(y);
    pru.recver = node;
    pru.pDelay = Time::FromDouble(dist / SOUND_SPEED_IN_WATER,Time::S);
    pru.pR = RayleighAtt(dist, asHeader.GetFreq(), asHeader.GetPt());
    res->push_back(pru);
  }

  return res;
}

Time
AquaSimSimplePropagation::PDelay (Ptr<AquaSimNode> s, Ptr<AquaSimNode> r)
{
  return Time::FromDouble((s->DistanceFrom(r) / SOUND_SPEED_IN_WATER ),Time::S);
}
    
double
AquaSimSimplePropagation::RayleighAtt (double dist, double freq, double pT)
{
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

  return -2 * segma * m_rand->GetValue();       // std::log(Random::uniform())
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

double
AquaSimSimplePropagation::distance (Ptr<AquaSimNode> s, Ptr<AquaSimNode> r)
{
  NS_LOG_FUNCTION(this);
  return s->DistanceFrom(r);	//redundant...
}

}  // namespace ns3

