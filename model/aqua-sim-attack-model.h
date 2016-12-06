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
* 96Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
* Author: Robert Martin <robert.martin@engr.uconn.edu>
*/


#ifndef AQUA_SIM_ATTACK_MODEL_H
#define AQUA_SIM_ATTACK_MODEL_H

#include "ns3/object.h"
#include "aqua-sim-net-device.h"


namespace ns3 {

class Packet;

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Attack Model Base class
 */
class AquaSimAttackModel : public Object {
public:
  static TypeId GetTypeId (void);
  void SetDevice(Ptr<AquaSimNetDevice> device);
  virtual void Recv(Ptr<Packet> p)=0;
  virtual void SendDown(Ptr<Packet> p);

protected:
  Ptr<AquaSimNetDevice> m_device;
};	//class AquaSimAttackModel

/**
 * \brief Attack Model for Denial of Service or packet flooding
 */
class AquaSimAttackDos : public AquaSimAttackModel {
public:
  static TypeId GetTypeId (void);
  AquaSimAttackDos();

  virtual void Recv(Ptr<Packet> p);
  Ptr<Packet> CreatePkt();
  void SetSendFrequency(double sendFreq);
  void SetPacketSize(int packetSize);
  void SetDestAddress(AquaSimAddress dest);

private:
  double m_sendFreq; //in seconds
  int m_packetSize;
  AquaSimAddress m_dest;
};  //class AquaSimAttackDos


//XXX add all models here...


} // namespace ns3

#endif /* AQUA_SIM_ATTACK_MODEL_H */
