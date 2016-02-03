//...


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


//Aqua Sim Signal Cache

namespace ns3 {

enum PacketStatus{ RECEPTION, COLLISION, INVALID };

struct IncomingPacket{
  Ptr<Packet> packet;
  enum PacketStatus status;
  IncomingPacket* next;
  IncomingPacket(Ptr<Packet> p, PacketStatus s = INVALID) :
	  packet(p), status(s), next(NULL) {}
};

class AquaSimSignalCache;

struct PktSubmissionUnit{
  IncomingPacket* inPkt;
  Time endT;	//ending time of reception
  friend bool operator < (const PktSubmissionUnit &l, const PktSubmissionUnit &r) {
    //pkt with earlier ending time appears at the front
    return l.endT >= r.endT;
  }
	
  PktSubmissionUnit(IncomingPacket* inPkt_, Time endT_);
};

/**
* submit packets in signal cache to upper layer
*/
class PktSubmissionTimer : public Timer{	
private:
  std::priority_queue<PktSubmissionUnit> m_waitingList;
  Ptr<AquaSimSignalCache> m_sC;
public:
  PktSubmissionTimer(Ptr<AquaSimSignalCache> sC);
  virtual ~PktSubmissionTimer(void);
  static TypeId GetTypeId(void);

  virtual void Expire(void);
  void AddNewSubmission(IncomingPacket* inPkt);
};  // class PktSubmissionTimer

/**
* this SignalCache simulates the way that how modems handle
* signals without considering multi-path effect
*/
class AquaSimSignalCache : public Object { 
public:
  AquaSimSignalCache(void);
  virtual ~AquaSimSignalCache(void);
  static TypeId GetTypeId(void);

  virtual void AddNewPacket(Ptr<Packet>);
  virtual bool DeleteIncomingPacket(Ptr<Packet>);
  void InvalidateIncomingPacket(void);
  IncomingPacket* Lookup(Ptr<Packet>);
  PacketStatus status;
  PacketStatus Status(Ptr<Packet> p);
  void SubmitPkt(IncomingPacket* inPkt);

  void SetNoiseGen(Ptr<AquaSimNoiseGen> noise);

  friend class PktSubmissionTimer;

protected:
  virtual void UpdatePacketStatus(void);

public:
  int m_pktNum;	// number of active incoming packets
  double m_totalPS; // total power strength of active incoming packets

protected:
  IncomingPacket* m_head;
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
* this class considers multi-path effect, what's the difference?
*/
class AquaSimMultiPathSignalCache : public AquaSimSignalCache {
	
  //TODO - in future work.

};  //class AquaSimMultiPathSignalCache


} //namespace ns3

#endif /* AQUA_SIM_SIGNAL_CACHE_H */
