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

#ifndef AQUA_SIM_MAC_FAMA_H
#define AQUA_SIM_MAC_FAMA_H

#include "ns3/random-variable-stream.h"
#include "aqua-sim-mac.h"

#include <queue>
#include <vector>

#define CALLBACK_DELAY 0.001

namespace ns3{

class Time;
class Packet;
class Address;
class Timer;

class AquaSimFama: public AquaSimMac {
public:
  AquaSimFama();
  ~AquaSimFama();
  static TypeId GetTypeId(void);

  void TxProcess(Ptr<Packet> pkt);
  void RecvProcess(Ptr<Packet> pkt);

protected:

  enum {
    PASSIVE,
    BACKOFF,
    WAIT_CTS,
    WAIT_DATA_FINISH,
    WAIT_DATA,
    REMOTE   /*I don't know what it means. but
		     node can only receive packet in this status*/
  }FamaStatus;

  double m_NDPeriod;
  int  m_maxBurst;	//the maximum number of packet burst. default is 1
  Time m_dataPktInterval;  //0.0001??

  Time m_estimateError;		//Error for timeout estimation
  int m_dataPktSize;
  int m_neighborId; //use this value to pick the next hop one by one

  double m_bitRate; //bit rate of MAC
  double m_transmitDistance;
    //distCST_ from ns2. this should be NOT be manual and instead be calc within channel.
  Time m_maxPropDelay;
  Time m_RTSTxTime;
  Time m_CTSTxTime;

  Time m_maxDataTxTime;


  std::queue<Ptr<Packet> > PktQ;
  std::vector<Address> NeighborList;

  Timer m_waitCTSTimer;
  Timer m_backoffTimer;
  Timer m_remoteTimer;
  Time m_remoteExpireTime;

  //packet_t UpperLayerPktType;


  Ptr<Packet> MakeND(); //broadcast
  Ptr<Packet> MakeRTS(Address Recver);
  Ptr<Packet> MakeCTS(Address RTS_Sender);

  void ProcessND(Ptr<Packet> pkt);
  void ProcessRTS(Ptr<Packet> pkt);

  void SendRTS(Time DeltaTime);
  void SendPkt(Ptr<Packet> pkt);
  void SendDataPkt();

  void ProcessDataSendTimer(Ptr<Packet> pkt);
  void ProcessDataBackoffTimer();
  void ProcessRemoteTimer();
  void NDTimerExpire();//periodically send out Neighbor discovery packet for 4 times.

  void StatusProcess();
  void BackoffTimerExpire();

  bool CarrierDected();
  void DoBackoff();
  void DoRemote(Time DeltaTime);

private:
  int m_famaNDCounter;
  Ptr<UniformRandomVariable> m_rand;
};

}  // namespace ns3

#endif /* AQUA_SIM_FAMA_H */
