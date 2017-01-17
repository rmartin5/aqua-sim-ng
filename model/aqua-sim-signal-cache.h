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


#ifndef AQUA_SIM_SIGNAL_CACHE_H
#define AQUA_SIM_SIGNAL_CACHE_H

#include <queue>

#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"
#include "ns3/object.h"

#include "aqua-sim-phy.h"
#include "aqua-sim-energy-model.h"
#include "aqua-sim-noise-generator.h"
#include "aqua-sim-header.h"


//Aqua Sim Signal Cache

namespace ns3 {

struct IncomingPacket : Object {
  Ptr<Packet> packet;
  AquaSimPacketStamp::PacketStatus status;
  Ptr<IncomingPacket> next;
  IncomingPacket(AquaSimPacketStamp::PacketStatus s = AquaSimPacketStamp::INVALID) :
    packet(NULL), status(s), next(NULL) {}
  IncomingPacket(Ptr<Packet> p, AquaSimPacketStamp::PacketStatus s = AquaSimPacketStamp::INVALID) :
	  packet(p), status(s), next(NULL) {}
};

class AquaSimSignalCache;

struct PktSubmissionUnit{
  Ptr<IncomingPacket> inPkt;
  Time endT;	//ending time of reception
  friend bool operator < (const PktSubmissionUnit &l, const PktSubmissionUnit &r) {
    //pkt with earlier ending time appears at the front
    return l.endT >= r.endT;
  }

  PktSubmissionUnit(Ptr<IncomingPacket> inPkt_, Time endT_);
};

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Helper timer to submit packets in signal cache to upper layer
 */
class PktSubmissionTimer : public Timer{
private:
  //std::priority_queue<PktSubmissionUnit> m_waitingList; not necessary.
  Ptr<AquaSimSignalCache> m_sC;
public:
  PktSubmissionTimer(Ptr<AquaSimSignalCache> sC);
  virtual ~PktSubmissionTimer(void);
  static TypeId GetTypeId(void);

  virtual void Expire(Ptr<IncomingPacket> inPkt);
  void AddNewSubmission(Ptr<IncomingPacket> inPkt);
};  // class PktSubmissionTimer

/**
 * \brief Signal Cache class which simulates the way that modems handle signals without considering multi-path effect
 */
class AquaSimSignalCache : public Object {
public:
  AquaSimSignalCache(void);
  virtual ~AquaSimSignalCache(void);
  static TypeId GetTypeId(void);

  virtual void AddNewPacket(Ptr<Packet>);
  virtual bool DeleteIncomingPacket(Ptr<Packet>);
  void InvalidateIncomingPacket(void);
  Ptr<IncomingPacket> Lookup(Ptr<Packet>);
  AquaSimPacketStamp::PacketStatus status;
  AquaSimPacketStamp::PacketStatus Status(Ptr<Packet> p);
  void SubmitPkt(Ptr<IncomingPacket> inPkt);

  void SetNoiseGen(Ptr<AquaSimNoiseGen> noise);

  friend class PktSubmissionTimer;

protected:
  virtual void UpdatePacketStatus(void);
  void DoDispose();

public:
  int m_pktNum;	// number of active incoming packets
  double m_totalPS; // total power strength of active incoming packets

protected:
  Ptr<IncomingPacket> m_head;
  Ptr<AquaSimPhy> m_phy;
  PktSubmissionTimer* m_pktSubTimer;
  Ptr<AquaSimEnergyModel> m_em;
  Ptr<AquaSimNoiseGen> m_noise;

private:
  /**
  * hide this interface from other classes, UnderwaterPhy
  * is the only one can use this method
  */
  void AttachPhy(Ptr<AquaSimPhy> phy) {
	  m_phy = phy;
  }
  friend class AquaSimPhy;
};  //class AquaSimSignalCache



/**
 * \brief Multi-path signal cache. Planned to be implemented in future updates.
 */
class AquaSimMultiPathSignalCache : public AquaSimSignalCache {
  /**
  * this class considers multi-path effect, what's the difference?
  */
  //TODO - in future work.

};  //class AquaSimMultiPathSignalCache


} //namespace ns3

#endif /* AQUA_SIM_SIGNAL_CACHE_H */
