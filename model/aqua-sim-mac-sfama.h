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

#ifndef AQUA_SIM_MAC_SFAMA_H
#define AQUA_SIM_MAC_SFAMA_H


#include "aqua-sim-mac.h"
#include "aqua-sim-channel.h"

#include "ns3/packet.h"
#include "ns3/timer.h"
#include "ns3/random-variable-stream.h"

#include <queue>

#define AquaSimSFAMA_DEBUG 0

namespace ns3 {

class AquaSimSFama;
class AquaSimAddress;

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Helper timer class for SFAMA
 */
class AquaSimSFama_Wait_Send_Timer: public Timer {
  friend class AquaSimSFama;
public:
	AquaSimSFama_Wait_Send_Timer(Ptr<AquaSimSFama> mac): Timer(Timer::CANCEL_ON_DESTROY) {
		m_mac = mac;
	}
  ~AquaSimSFama_Wait_Send_Timer()
  {
    m_mac=0;
    m_pkt=0;
  }


  /* Not necessary:
   *      (1)smart ptr will destroy when out of scope
   *      (2)timer set to CANCEL_ON_DESTROY
  */

  /*
  void Stop() {
		if( m_pkt != NULL ) {
			pkt=0;
			//pkt_ = NULL;
		}
		if( this->IsRunning() ) {
			Cancel();
		}
	}
  */

protected:
	Ptr<AquaSimSFama> m_mac;
	Ptr<Packet> m_pkt;
	void expire();
};

/**
 * \brief Helper timer class for SFAMA
 */
class AquaSimSFama_Wait_Reply_Timer: public Timer {
  friend class AquaSimSFama;
public:
	AquaSimSFama_Wait_Reply_Timer(Ptr<AquaSimSFama> mac): Timer(Timer::CANCEL_ON_DESTROY) {
		m_mac = mac;
	}
  ~AquaSimSFama_Wait_Reply_Timer()
  {
    m_mac=0;
  }

/*
	void Stop() {
		if( this->IsRunning() ) {
			Cancel();
		}
	}
*/
protected:
	Ptr<AquaSimSFama> m_mac;
	void expire();
};

/**
 * \brief Helper timer class for SFAMA
 */
class AquaSimSFama_Backoff_Timer: public Timer{
  friend class AquaSimSFama;
public:
	AquaSimSFama_Backoff_Timer(Ptr<AquaSimSFama> mac): Timer(Timer::CANCEL_ON_DESTROY) {
		m_mac = mac;
	}
  ~AquaSimSFama_Backoff_Timer()
  {
    m_mac=0;
  }

	/*void stop() {
		if( this->status() == TIMER_PENDING ) {
			cancel();
		}
	}*/

protected:
	Ptr<AquaSimSFama> m_mac;
	void expire();
};

/**
 * \brief Helper timer class for SFAMA
 */
class AquaSimSFama_DataSend_Timer: public Timer {
  friend class AquaSimSFama;
public:
	AquaSimSFama_DataSend_Timer(Ptr<AquaSimSFama> mac): Timer(Timer::CANCEL_ON_DESTROY) {
		m_mac = mac;
	}
  ~AquaSimSFama_DataSend_Timer()
  {
    m_mac=0;
  }
protected:
	Ptr<AquaSimSFama> m_mac;
	void expire();
};

enum AquaSimSFama_Status{
    IDLE_WAIT,  /*do nothing but just wait*/
    WAIT_SEND_RTS,
    WAIT_SEND_CTS,
    WAIT_RECV_CTS,
    WAIT_SEND_DATA,
    WAIT_RECV_DATA,
    WAIT_SEND_ACK,
    WAIT_RECV_ACK,
    BACKOFF,
	  BACKOFF_FAIR
};

/**
 * \brief Slotted FAMA protocol
 */
class AquaSimSFama: public AquaSimMac{
public:
	AquaSimSFama();
  virtual ~AquaSimSFama();
  static TypeId GetTypeId(void);
  int64_t AssignStreams (int64_t stream);

	// to process the incomming packet
	virtual  bool RecvProcess(Ptr<Packet>);
	// to process the outgoing packet
	virtual  bool TxProcess(Ptr<Packet>);


	enum AquaSimSFama_Status m_status;


	void CallBackProcess();
	void StatusProcess(int slotnum);

	void WaitSendTimerProcess(Ptr<Packet> pkt);
	void BackoffTimerProcess();
	void WaitReplyTimerProcess(bool directcall=false);
	void DataSendTimerProcess();

	void InitSlotLen();

private:
	//index_ is the mac address of this node
	double m_guardTime;  //need to be binded
	double m_slotLen;

	// Store data packet size value from the app
	double m_packet_size = 50;

	bool m_isInRound;
	bool m_isInBackoff;

	int m_maxBackoffSlots;

	int m_maxBurst; /*maximum number of packets in the train*/
	double m_dataSendingInterval;

	//wait to send pkt at the beginning of next slot
	AquaSimSFama_Wait_Send_Timer m_waitSendTimer;
	//wait for the corresponding reply. Timeout if fail to get
	AquaSimSFama_Wait_Reply_Timer m_waitReplyTimer;
	//if there is a collision or RTS/CTS not for this node, do backoff
	AquaSimSFama_Backoff_Timer    m_backoffTimer;
	AquaSimSFama_DataSend_Timer	m_datasendTimer;

	std::queue<Ptr<Packet> > m_sendingPktQ;
	std::queue<Ptr<Packet> > m_CachedPktQ;
	std::queue<Ptr<Packet> > m_BackupSendingPktQ;

	//packet_t UpperLayerPktType;
  Ptr<UniformRandomVariable> m_rand;

  int m_slotNumHandler;
protected:
  /// creating packets, with the appropriate headers, using the assigned parameter(s)
	Ptr<Packet> MakeRTS(AquaSimAddress recver, int slot_num);
	Ptr<Packet> MakeCTS(AquaSimAddress rts_sender, int slot_num);
	Ptr<Packet> FillDATA(Ptr<Packet> data_pkt);
	Ptr<Packet> MakeACK(AquaSimAddress data_sender);

  /// handle different packet types
	void ProcessRTS(Ptr<Packet> rts_pkt);
	void ProcessCTS(Ptr<Packet> cts_pkt);
	void ProcessDATA(Ptr<Packet> data_pkt);
	void ProcessACK(Ptr<Packet> ack_pkt);

	void DoBackoff(int backoff_slotnum);

	void SendPkt(Ptr<Packet> pkt);
	void SendDataPkt(Ptr<Packet> pkt);

	void SetStatus(enum AquaSimSFama_Status status);
	enum AquaSimSFama_Status GetStatus();

	void StopTimers();
	void ReleaseSentPkts();

	void PrepareSendingDATA();

	double GetPktTrainTxTime();

	void ScheduleRTS(AquaSimAddress recver, int slot_num);

	double GetTime2ComingSlot(double t);

	int RandBackoffSlots();
  void SlotInitHandler();

#ifdef AquaSimSFama_DEBUG
	void PrintQ(std::queue<Ptr<Packet> >& my_q);
	void PrintAllQ();
#endif

	friend class AquaSimSFama_Backoff_Timer;
	friend class AquaSimSFama_Wait_Send_Timer;
	friend class AquaSimSFama_Wait_Reply_Timer;
	friend class AquaSimSFama_DataSend_Timer;

  virtual void DoDispose();
};  // class AquaSimSFama

} // namespace ns3

#endif /* AQUA_SIM_MAC_SFAMA_H */
