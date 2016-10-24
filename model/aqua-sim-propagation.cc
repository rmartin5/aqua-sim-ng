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


#include "ns3/nstime.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"

#include "aqua-sim-propagation.h"

namespace ns3 {

const double SOUND_SPEED_IN_WATER = 1500;

NS_LOG_COMPONENT_DEFINE ("AquaSimPropagation");
NS_OBJECT_ENSURE_REGISTERED (AquaSimPropagation);

TypeId
AquaSimPropagation::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::AquaSimPropagation")
    .SetParent<Object>()
  ;
  return tid;
}

Time
AquaSimPropagation::PDelay (Ptr<MobilityModel> s, Ptr<MobilityModel> r)
{
  NS_LOG_FUNCTION(this);
  return Time::FromDouble((s->GetDistanceFrom(r) / ns3::SOUND_SPEED_IN_WATER), Time::S);
}

/*
 *  Attentuation Model:
 *  A(l,f) = l^k * (10^(a(f)/10))^l
 *
 *  l=distance, f=frequency, k=spread factor,
 *  a(f)=absorpotion coefficient (Thorp's equation)
*/
double
AquaSimPropagation::Rayleigh (double d, double f)
{
  /* the distance unit used for absorption coefficient is km,
     but for the attenuation, the used unit is meter
   */
  double k;
  k=2;  //static practical spreading type
  /*
  if (d <= 500) {
   k = 3;
  } else if (d <= 2000) {
   k = 2;
  } else {
   k = 1.5;
  }
  */

  double d1=d/1000.0; // convert to km
  double t1=pow(d,k);
  double alpha_f=Thorp(d,f);
  double alpha=pow(10.0,(alpha_f/10.0));
  double t3=pow(alpha,d1);
  NS_LOG_DEBUG("Rayleigh dump: distance(km):" << d1 <<
                  ", k:" << k <<
                  ", f:" << f <<
                  ", a(f):" << alpha_f <<
                  ", A(l,f):" << t1*t3);
  return t1*t3;
}


/**
 * @param SL sound level in dB
 * @return receiving power in J
 */
double
AquaSimPropagation::Rayleigh (double SL)
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
AquaSimPropagation::Thorp (double dist, double freq)
{
  /*TODO re-evaluate all of this attenuation model (from 2.0)
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
  */
  return (0.11 * pow(freq,2) / (1 + pow(freq,2) )
      + 44 * pow(freq,2) / (4100 + pow(freq,2) )
      + 0.000275 * pow(freq,2) + 0.0003 );
}

//2.0 verison below:

/**
 * @param SL  sound level in dB
 * @return receiving power in J
 */
double
AquaSimPropagation::Rayleigh2( double SL )
{
	double MPr = std::pow(10, SL/20 - 6); //signal strength (pressure in Pa)
	double segma = pow(MPr, 2) * 2 / M_PI;

  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();

	return -2 * segma * std::log(m_rand->GetValue());
}

/**
 * Thorp equition, calculating attenuation according
 *
 * @param dist	distance that singal travels
 * @param freq  central frequency
 * @return attenuation in dB *
 */
double
AquaSimPropagation::Thorp2( double dist, double freq )
{

	double k, spre, abso;

	if ( dist <= 500 ) {
		k = 3;
	} else if ( dist <= 2000 ) {
		k = 2;
	} else {
		k = 1.5;
	}

	spre = 10 * k * log10( dist );

	abso = dist/1000 * ( 0.11 * pow(freq,2) / ( 1 + pow(freq,2) )
			+ 44 * pow(freq,2) / ( 4100 + pow(freq,2) )
			+ 0.000275 * pow(freq,2) + 0.003 );
	return spre + abso;
}

}  // namespace n3
