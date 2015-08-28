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

#ifndef AQUA_SIM_MODULATION_H
#define AQUA_SIM_MODULATION_H

#include "ns3/object.h"

namespace ns3 {

class AquaSimModulation : public Object {

public:
  static TypeId GetTypeId (void);

  AquaSimModulation ();
  virtual ~AquaSimModulation () {}

  /*
   *  Get transmission time by packet size
   */
  virtual double TxTime (int pktSize);

  /*
   *  Get packet size by transmission time
   */
  virtual int PktSize (double txTime);

  /*
   *  Give the packet error rate of a packet of size pktsize
   */
  virtual double Per (int pktSize);

  /*
   *  Returns number bits per second
   */
  virtual double Bps () { return m_sps/m_codingEff; }

protected:
  /*
   *  Preamble of physical frame
   */
  double m_codingEff;  //coding efficiency: number of symbols per bit
  int m_sps;  //number of symbols per second
  double m_ber;  //bit error rate

};  // AquaSimModulation

}  // namespace ns3

#endif /* AQUA_SIM_MODULATION_H */
