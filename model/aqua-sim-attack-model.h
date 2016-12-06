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
#include <map>

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
 *
 *    Flooding network with malicious packets
 */
class AquaSimAttackDos : public AquaSimAttackModel {
public:
  static TypeId GetTypeId (void);
  AquaSimAttackDos();

  virtual void Recv(Ptr<Packet> p);
  void SetSendFrequency(double sendFreq);
  void SetDestAddress(AquaSimAddress dest);
  void SetPacketSize(int packetSize);
  Ptr<Packet> CreatePkt();

private:
  void SendPacket();

  double m_sendFreq; //in seconds
  int m_packetSize;
  AquaSimAddress m_dest;
};  //class AquaSimAttackDos

/**
 * \brief Attack Model for Sinkholes
 *
 *  Attract traffic through attractive characteristics (high-quality route / capabilities)
 *    and drop all packets or selective forward packets
 */
class AquaSimAttackSinkhole : public AquaSimAttackModel {
public:
  static TypeId GetTypeId (void);
  AquaSimAttackSinkhole();

  virtual void Recv(Ptr<Packet> p);
  Ptr<Packet> CreatePkt();
  void SendAdvertisePacket();

  void SetDataRate(double rate);
  void SetEnergy(double energy);
  void SetDepth(double depth);
  void SetDropFrequency(double drop);

private:
  //fake characteristics advertising
  double m_dataRate;  //Bytes per sec.
  double m_energy;    //J
  double m_depth;     //meters
  double m_dropFrequency;   // should be percentage (between 0 and 1)

  int m_pktDropped;
  int m_totalPktRecv;
};  //class AquaSimAttackSinkhole

/**
 * \brief Attack Model for Selective Forward
 *
 *  Selectively drop or forward packets
 */
class AquaSimAttackSelective : public AquaSimAttackModel {
public:
  static TypeId GetTypeId (void);
  AquaSimAttackSelective();

  virtual void Recv(Ptr<Packet> p);
  void SetDropFrequency(double drop);
  void BlockNode(int sender);
  void BlockNode(AquaSimAddress sender);

private:
  int m_blockSender;
  double m_dropFrequency;   // should be percentage (between 0 and 1)
  int m_pktDropped;
  int m_totalPktRecv;
};  //class AquaSimAttackSelective

/**
 * \brief Attack Model for Sybil
 *
 *  Pretend to be in a different location or multiple locations
 *    or have multiple identities (i.e. behave as base station)
 */
class AquaSimAttackSybil : public AquaSimAttackModel {
public:
  static TypeId GetTypeId (void);
  AquaSimAttackSybil();

  virtual void Recv(Ptr<Packet> p);
  void AddFakeNode(int id, Vector location);
  void RemoveFakeNode(int id);
  Vector GetLocation(int id);

private:
  std::map<int,Vector> m_locations;
};  //class AquaSimAttackSybil

} // namespace ns3

#endif /* AQUA_SIM_ATTACK_MODEL_H */
