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

#ifndef AQUA_SIM_SINR_CHECKER_H
#define AQUA_SIM_SINR_CHECKER_H

#include "ns3/object.h"

namespace ns3 {

//interface of Aqua Sim SINR Checker
class AquaSimSinrChecker : public Object{
public:
  static TypeId GetTypeId (void);
  virtual bool Decodable (double sinr) = 0;
};


class AquaSimThresholdSinrChecker : public AquaSimSinrChecker {
public:
  static TypeId GetTypeId (void);
  AquaSimThresholdSinrChecker ();
  virtual ~AquaSimThresholdSinrChecker();
  virtual bool Decodable (double sinr);
protected:
  double m_decThresh;  //threshold of decodable packet
};

}  //namespace ns3

#endif /* AQUA_SIM_SINR_CHECKER_H */
