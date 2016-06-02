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

#ifndef AQUA_SIM_TMAC_H
#define AQUA_SIM_TMAC_H

#include "aqua-sim-mac.h"
#include "aqua-sim-rmac-buffer.h"
#include "aqua-sim-net-device.h"
#include "aqua-sim-address.h"

#include "ns3/random-variable-stream.h"
#include "ns3/event-id.h"
#include "ns3/packet.h"

#define T_TABLE_SIZE 10 // the size of delay table
#define MAXIMUMBACKOFF 4 // the maximum times of backoffs
#define BACKOFF 1 //deleted later, used by TxProcess


#define UW_ND 1
#define UW_ACK_ND 2


#define PHASEONE 1
#define PHASETWO 2
#define PHASETHREE 3

namespace ns3 {

enum TMacPacketType{
  PT_DATA,
  PT_RTS,
  PT_CTS,
  PT_ND,
  PT_SACKND,
  PT_ACKDATA,
  PT_SYN
};


enum TMAC_STATUS{
  TMAC_IDLE,
  TMAC_RTS,
  TMAC_CTS,
  TMAC_RECV,
  TMAC_TRANSMISSION,
  TMAC_SILENCE,
  TMAC_SLEEP
};





struct t_time_record{
  AquaSimAddress node_addr;// the address of the node
  double arrival_time;// the time to receive the ND packet from the node
  double sending_time; // the sending time of ND in local clock
};




struct t_period_record{
  AquaSimAddress node_addr;// the address of the node
  double difference;// the difference with my period
  double duration; // duration of duty cycle
  double last_update_time; // the time last updated
};



struct t_silence_record {
  AquaSimAddress node_addr;// the address of the node
  double start_time;// the start time of the silence
  double duration; // duration of duty cycle
  int  confirm_id; // silence is confirmed
};


struct t_latency_record{
  AquaSimAddress node_addr;      // the address of the node
  double latency;    // the propagation latency with that node
  double sumLatency;// the sum of latency
  int num;         // number of ACKND packets
  double last_update_time; // the time of last update
};


class AquaSimTMac: public AquaSimMac {

public:
  AquaSimTMac();
  ~AquaSimTMac();
  static TypeId GetTypeId (void);

  double m_ndWindow; // the window to send ND
  double m_ackNdWindow; // the winddow to send ACK_ND
  double m_phaseOneWindow;  // the time for latency detection
  double m_phaseTwoWindow;  // the time for SYN announcement
  double m_sif; // interval between two successive data packets
  double m_lastSilenceTime; // the time for the longest silence
  double m_lastRtsSilenceTime;
  double m_phaseTwoInterval; // interval between windows of phase two

  int m_phyOverhead; // the overhead caused by phy layer
  int m_arrivalTableIndex;

  int m_shortLatencyTableIndex;
  int m_periodTableIndex;
  int m_silenceTableIndex;

  AquaSimAddress m_dataSender; // address of the data sender
  int m_bitMap[MAXIMUM_BUFFER]; // in real world, this is supposed to use bit map to indicate the lost of packet
// these two variables are used to set next hop
// SetHopStatus=1 then set next hop using next_hop
// int setHopStatus;
//int next_hop;

  int m_numSend;
  int m_numData;
  int m_largePacketSize;
  int m_shortPacketSize;
  double m_transmissionRange;
  double m_duration; // duration of duty cycle
  double m_intervalPhase2Phase3;
  double m_nextPeriod; //the start_time of next duty cycle
  double m_periodInterval;
  double m_maxShortPacketTransmissionTime;
  double m_maxLargePacketTransmissionTime;
  double m_transmissionTimeError; //
  double m_maxPropagationTime;

  double m_taDuration;
  double m_contentionWindow;
  double m_minBackoffWindow;

// bool overhearData;
  int m_rtsTimeoutNum;
  AquaSimAddress m_transmissionAddr;
  double m_tBackoffWindow;
  AquaSimAddress m_rtsRecvAddr;
  int m_ctsNum;
  Ptr<UniformRandomVariable> m_rand;

  int m_phaseOneCycle; // number of cycles in phase one
  int m_phaseTwoCycle; // number of cycles in phase two
  int m_phaseStatus;

  enum TMAC_STATUS m_macStatus;

  double m_cycleStartTime; // the begining time of this cycle;
  TransmissionBuffer m_txbuffer;
  struct  t_time_record m_arrivalTable[T_TABLE_SIZE];

  struct t_latency_record m_shortLatencyTable[T_TABLE_SIZE];
  struct t_period_record m_periodTable[T_TABLE_SIZE];
  struct t_silence_record m_silenceTable[T_TABLE_SIZE];

  void InitPhaseOne(double /*ND window*/,double /*ack_nd window*/,double /* phaseOne window*/);

  void InitPhaseTwo();
  void InitPhaseThree();
  void StartPhaseTwo();


  void InitND(double /*ND window*/,double /*ack_nd window*/,double /* phase One window*/); // to detect latency

  void SendND(int);
  void TxND(Ptr<Packet>, double);

  void ProcessNDPacket(Ptr<Packet>);
  void ProcessDataPacket(Ptr<Packet>);
  void ProcessShortACKNDPacket(Ptr<Packet>);
  void ProcessSYN(Ptr<Packet>);
  void ProcessRTSPacket(Ptr<Packet>);
  void ProcessCTSPacket(Ptr<Packet>);
  void ProcessSleep();
  void ProcessSilence();

  void InitializeSilenceTable();
  void DeleteSilenceTable(int);
  void ConfirmSilenceTable(AquaSimAddress, double);
  void DeleteSilenceRecord(AquaSimAddress);
  void DataUpdateSilenceTable(AquaSimAddress);
  void InsertSilenceTable(AquaSimAddress, double);
  void CleanSilenceTable();


  Ptr<Packet> GenerateCTS(AquaSimAddress,double);
  void ProcessACKDataPacket(Ptr<Packet>);
  void ClearTxBuffer();
  void Wakeup();
  void ReStart();
  void SendACKPacket();
  void SetIdle();
  void SendRTS();
  void SendCTS();
  void TxCTS(Ptr<Packet>);

  void TxACKData(Ptr<Packet>);
  void ResetMacStatus();

  void SendShortAckND();
  void StatusProcess(TransStatus);
  void SendSYN();
  bool NewData(); // ture if there exist data needed to send, false otherwise

  void TxRTS(Ptr<Packet> pkt,AquaSimAddress receiver_addr);
  double CheckLatency(t_latency_record*,AquaSimAddress);
  double CheckDifference(t_period_record*,AquaSimAddress);

  void MarkBitMap(int);


  void TxData(AquaSimAddress);
  void PrintTable();
  void ResumeTxProcess();

  void RTSTimeoutHandler();
  void TBackoffHandler(Ptr<Packet> pkt);
  void TNDHandler();
  void TStatusHandler();
  void TStatusHandler_SetStatus(TransStatus status);
  void CTSHandler(Ptr<Packet> pkt);

  Ptr<Packet> GenerateSYN();

  //Event large_nd_event;
  EventId m_shortNdEvent;
  EventId m_statusEvent;

  EventId m_shortAckndEvent;
  EventId m_phaseoneEvent;
  EventId m_phasetwoEvent;
  EventId m_phasethreeEvent;

//EventId transmission_event;

  EventId m_silenceEvent;
  EventId m_ackEvent;
  EventId m_poweroffEvent;
  EventId m_wakeupEvent;
  EventId m_timeoutEvent;
  EventId m_rtsTimeoutEvent;
// Event rts_silence_event;

//     RTSSilenceHandler rts_silence_handler;
//Node* node(void) const {return node_;}
// to process the incomming packet
  virtual bool RecvProcess(Ptr<Packet>);


// to process the outgoing packet
  virtual bool TxProcess(Ptr<Packet>);

private:
  int m_tBackoffCounter;
  TransStatus m_localStatus;

};    // class AquaSimTMac
} // namespace ns3

#endif /* AQUA_SIM_TMAC_H */
