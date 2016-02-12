/*
 * aqua-sim-mac-aloha.h
 *
 *  Created on: Feb 11, 2016
 *      Author: Robert Martin
 */

#ifndef AQUA_SIM_MAC_ALOHA_H
#define AQUA_SIM_MAC_ALOHA_H

#include "aqua-sim-header.h"
#include "aqua-sim-mac.h"

#include "ns3/event-id.h"
#include "ns3/random-variable-stream.h"
#include "ns3/timer.h"
#include "ns3/packet.h"

#include <queue>
#include <map>
#include <math.h>


#define CALLBACK_DELAY 0.001	//the interval between two consecutive sendings
#define MAXIMUMCOUNTER 4
#define Broadcast -1

namespace ns3 {

class AquaSimAloha;
class Address;

class AquaSimAlohaAckRetry : public Timer
{
public:
  AquaSimAlohaAckRetry(Ptr<AquaSimAloha> mac, Ptr<Packet> pkt)
    {
      m_mac = mac;
      m_pkt = pkt;
      m_id = m_idGenerator++;
    }

  Ptr<Packet> Pkt() {
    return m_pkt;
  }

  long Id() {
    return m_id;
  }

protected:
  Ptr<AquaSimAloha> m_mac;
  Ptr<Packet> m_pkt;
  long	m_id;
  static long m_idGenerator;
};


class AquaSimAloha: public AquaSimMac
{
public:
  AquaSimAloha();
  ~AquaSimAloha();
  static TypeId GetTypeId(void);

  void TxProcess(Ptr<Packet> pkt);
  void RecvProcess(Ptr<Packet> pkt);
  int m_boCounter;

  void	ProcessRetryTimer(AquaSimAlohaAckRetry* timer);
protected:

  enum {
	  PASSIVE,
	  BACKOFF,
	  SEND_DATA,
	  WAIT_ACK,
  }ALOHA_Status;

  double m_persistent;
  int	m_AckOn;
  double m_minBackoff;
  double m_maxBackoff;
  double m_waitACKTime;
  double m_maxACKRetryInterval;

  bool m_blocked;
  bool m_isAck;	//status handler

  //int m_seqN;

  double m_maxPropDelay;
  double m_dataTxTime;
  double m_AckTxTime;

  std::queue<Ptr<Packet> >	PktQ_;
  std::map<long, AquaSimAlohaAckRetry*> RetryTimerMap_;   //map timer id to the corresponding pointer

  EventId m_statusEvent;
  EventId m_forwardEvent;
  EventId m_callbackEvent;
  EventId m_waitACKTimer;

  Ptr<Packet> MakeACK(Address RTS_Sender);

  void	ReplyACK(Ptr<Packet> pkt);

  //void sendACK(double DeltaTime);

  void	SendPkt(Ptr<Packet> pkt);//why?
  void	SendDataPkt();
  //	void 	DropPacket(Ptr<Packet>);
  void	DoBackoff();
  //	void	processDataSendTimer(Event *e);
  //void	ProcessWaitACKTimer(Ptr<EventId> e);
  void	ProcessPassive();
  //	void	processBackoffTimer();


  void	RetryACK(Ptr<Packet> ack);

  void	StatusProcess(bool isAck);
  //void	BackoffProcess();
  //bool	CarrierDected();
private:
  Ptr<UniformRandomVariable> m_rand;

};  // class AquaSimAloha

} // namespace ns3

#endif /* AQUA_SIM_MAC_ALOHA_H */
