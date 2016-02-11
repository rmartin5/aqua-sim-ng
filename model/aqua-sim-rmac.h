/*
 * aqua-sim-rmac.h
 *
 *  Created on: Feb 9, 2016
 *      Author: Robert Martin
 */

#ifndef AQUA_SIM_RMAC_H
#define AQUA_SIM_RMAC_H

#include "aqua-sim-mac.h"
#include "aqua-sim-rmac-buffer.h"
#include "ns3/address.h"
#include "ns3/event-id.h"


#define TABLE_SIZE 20 // the size of delay table
#define MAXIMUMBACKOFF 4 // the maximum times of backoffs
#define BACKOFF 1 //deleted later, used by TxProcess

#define AS_ND 1
#define AS_ACK_ND 2

#define PHASEONE 1
#define PHASETWO 2
#define PHASETHREE 3

namespace ns3{

class Packet;
class AquaSimNetDevice;

enum RmacPacketType{
  P_DATA,
  P_REV,
  P_ACKREV,
  P_ND,
  P_SACKND,
  P_ACKDATA,
  P_SYN
};


enum MAC_STATUS{
  RMAC_IDLE,
  RMAC_REV,
  RMAC_ACKREV,
  RMAC_RECV,
  RMAC_WAIT_ACKREV,
  RMAC_WAIT_ACKDATA,
  RMAC_FORBIDDED,
  RMAC_TRANSMISSION,
};


struct forbidden_time_record{
  int node_addr;// the address of the node
  double start_time;// the time to receive the ND packet from the node
  double next_time; // the  start time of next availabe period
  double duration; // the sending time of ND in local clock

};

struct time_record{
  int node_addr;// the address of the node
  double arrival_time;// the time to receive the ND packet from the node
  double sending_time; // the sending time of ND in local clock
};


struct reservation_record{
  int  node_addr;    // the address of the node
  double required_time;    // the duration of required time slot
  double interval;
  int block_id;
//the  interval between the start time of the transmission and the
// start time of the cycle
};



struct ackdata_record{
  int node_addr;// the address of the sender
  int bitmap[MAXIMUM_BUFFER];  // the pointer to the bitmap
  int block_num;// the block id
};


struct period_record{
  int node_addr;// the address of the node
  double difference;// the difference with my period
  double duration; // duration of duty cycle
  double last_update_time; // the time last updated
};


struct latency_record{
  int node_addr;      // the address of the node
  double latency;    // the propagation latency with that node
  double sumLatency;// the sum of latency
  int num;         // number of ACKND packets
  double last_update_time; // the time of last update
};

//If supported, below headers should be handled within their own header source file.

/*
struct hdr_rmac_data{
        double duration;  // there is a bug, put this to fix it
          int pk_num;    // sequence number
         int sender_addr;     //original sender' address
       // int receiver_addr;  // the address of the intended receiver


	static int offset_;
  	inline static int& offset() { return offset_; }
  	inline static hdr_rmac_data* access(const Packet*  p) {
		return (hdr_rmac_data*) p->access(offset_);
	}
};




struct hdr_rev{
        // unsigned int type;     //packet type
         int pk_num;    // sequence number
        int sender_addr;     //original sender' address
  // int receiver_addr;  // the address of the intended receiver
        double duration;           // time interval for reservation

	static int offset_;
  	inline static int& offset() { return offset_; }
  	inline static hdr_rev* access(const Packet*  p) {
		return (hdr_rev*) p->access(offset_);
	}
};


struct hdr_ack_rev{
        // unsigned int type;     //packet type
        int pk_num;    // sequence number
        int sender_addr;     //original sender' address
        int receiver_addr;  // the address of the intended receiver
        double duration;           // time interval for reservation
        double st;                 // start time of reservation

	static int offset_;
  	inline static int& offset() { return offset_; }
  	inline static hdr_ack_rev* access(const Packet*  p) {
		return (hdr_ack_rev*) p->access(offset_);
	}
};




struct hdr_syn{
        //  unsigned int type;     //packet type
         int pk_num;    // sequence number
        int sender_addr;  //original sender' address
        double interval;    // interval to the begining of periodic operation
        double duration; // duration of duty cycle;

	static int offset_;
  	inline static int& offset() { return offset_; }
  	inline static hdr_syn* access(const Packet*  p) {
		return (hdr_syn*) p->access(offset_);
	}
};

struct hdr_ack_nd{
  //	unsigned int type;
         int pk_num;    // sequence number
        int sender_addr;  //original sender' address
         double ts;// sending time of the ND in sender's clock
  double arrival_time; //arrival time of ND in  the receiver's clock
       //  struct  time_record table[TABLE_SIZE]; // delay table

	static int offset_;
  	inline static int& offset() { return offset_; }

  	inline static hdr_ack_nd* access(const Packet*  p) {
		return (hdr_ack_nd*) p->access(offset_);
	}
};

*/

class AquaSimRMac: public AquaSimMac {

public:
  AquaSimRMac();
  virtual ~AquaSimRMac();
  static TypeId GetTypeId (void);

  double m_NDwindow;// the window to send ND
  double m_ackNDwindow;// the window to send ACK_ND
  double m_phaseOneWindow; // the time for latency detection
  double m_phaseTwoWindow; // the time for SYN announcement
  double m_SIF;// interval between two successive data packets
  double m_ackRevInterval;
  double m_phaseTwoInterval;// interval between windows of phase two
  int m_phyOverhead;// the overhead caused by phy layer
  int m_arrivalTableIndex;
  // int m_largeLatencyTableIndex;
  int m_shortLatencyTableIndex;
  int m_periodTableIndex;
  int m_reservationTableIndex;
  int m_reservedTimeTableIndex;
  int m_ackDataTableIndex;
  int m_timer;// number of periodIntervals to backoff
  //int m_dataSender; // address of the data sender
  double m_NDBackoffWindow;
  int m_NDBackoffCounter;

  // in real world, this is supposed to use bit map
  // to indicate the lost of packet
  int m_bitMap[MAXIMUM_BUFFER];

  // these two variables are used to set next hop
  // SetHopStatus=1 then set next hop using next_hop
  // int m_setHopStatus;
  // int m_nextHop;

  bool m_carrierSense; // sense the channel in the ackrev time slot
  //   bool m_skip;
  int m_numSend;
  int m_numData;
  int m_numBlock;

  int m_largePacketSize;
  int m_shortPacketSize;
  double m_duration; // duration of duty cycle
  double m_intervalPhase2Phase3;
  double m_nextPeriod;//the start_time of next duty cycle
  double m_periodInterval;
  double m_maxShortPacketTransmissiontime;
  double m_maxLargePacketTransmissiontime;
  double m_transmissionTimeError; //guardian time
  double m_theta; //used for precision control
  double m_bitRate; //bit rate of MAC
  double m_encodingEfficiency; //ratio of encoding
    //two above should be static.

  int m_phaseOneCycle; // number of cycles in phase one
  int m_phaseTwoCycle; // number of cycles in phase two

  int m_phaseStatus;
    // used by the receiver to test if the data packet arrives on time
  bool m_recvBusy;
    // used to indicate the status of collecting reservations
  bool m_collectRev;

  enum MAC_STATUS m_macStatus;

  //Recv Handler
  int m_recvDataSender;
  double m_recvDuration;// duration of RECV
  int m_recvStatus;// 0 is open the recv window and 1 is close window

  //transmission handler
  Address m_transReceiver;

  double m_cycleStartTime; // the beginning time of this cycle;
  TransmissionBuffer m_txBuffer;
  struct time_record arrival_table[TABLE_SIZE];
  struct forbidden_time_record reserved_time_table[TABLE_SIZE];
  //struct latency_record large_latency_table[TABLE_SIZE];
  struct latency_record short_latency_table[TABLE_SIZE];
  struct period_record  period_table[TABLE_SIZE];
  struct reservation_record reservation_table[TABLE_SIZE];
  struct reservation_record next_available_table[TABLE_SIZE];
  struct ackdata_record ackdata_table[TABLE_SIZE];
  struct buffer_cell * ack_rev_pt;// pointer to the link of ack_rev

  void InitPhaseOne(double NDwindow, double ackNDwindow, double phaseOneWindow);

  void InitPhaseTwo();
  void InitPhaseThree();
  void StartPhaseTwo();

  void InitND(double NDwindow,double ackNDwindow,double paseOneWindow);// to detect latency
  void ShortNDHandler();
  void SendND(int);
  void TxND(Ptr<Packet> p, double window);
  void NDBackoffHandler(Ptr<Packet> p);
  // void ProcessPacket(Ptr<Packet>);
  void ProcessNDPacket(Ptr<Packet>);
  void ProcessDataPacket(Ptr<Packet>);
  // void ProcessLargeACKNDPacket(Ptr<Packet>);
  void ProcessShortACKNDPacket(Ptr<Packet>);
  void ProcessSYN(Ptr<Packet>);
  void ProcessSleep();
  void ProcessRevPacket(Ptr<Packet>);
  void ProcessACKRevPacket(Ptr<Packet>);
  void ProcessACKDataPacket(Ptr<Packet>);
  void ProcessReservedTimeTable();
  bool ProcessRetransmission();
  void ProcessListen();
  void ProcessCarrier();

  void ResetReservation();
  void Wakeup();
  void TxRev(Ptr<Packet> p);
  void TxACKRev(Ptr<Packet>);
  void TxACKData(Ptr<Packet> pkt);
  void ResetMacStatus();
  void ScheduleACKREV(int receiver,double duration,double offset);
  void ScheduleACKData(int dataSender);
  //void SendLargeAckND();
  void SendShortAckND();
  void StatusProcess(TransmissionStatus state);
  void SendSYN();
  void MakeReservation();
  void ArrangeReservation();
  void ResetReservationTable();
  void SetStartTime(buffer_cell* ackRevPt, double st,double nextPeriod);
  void ClearTxBuffer();
  void InsertReservedTimeTable(int senderAddr,double startTime,double dt);
  void SortPeriodTable(struct period_record* table);
  void InsertBackoff(int sender_addr);
  void CopyBitmap(Ptr<Packet> pkt,int dataSender);
  void UpdateACKDataTable(int dataSender,int bNum,int num);
  void ClearReservationTable(int index);
  Ptr<Packet> GenerateACKRev(int receiver, int intendedReceiver, double duration);
  Ptr<Packet> GenerateSYN();
  int SelectReservation();
  double CalculateOffset(double dt);
  double CalculateACKRevTime(double diff1,double l1,double diff2,double l2);
  double CalculateACKRevTime(double diff,double latency,double elapsedTime);
  double DetermineSendingTime(int receiverAddr);
  double CheckLatency(latency_record* table,int addr);
  double CheckDifference(period_record* table,int addr);


  bool IsRetransmission(int reservationIndex);
  bool NewData();// ture if there exist data needed to send, false otherwise
  bool IsACKREVWindowCovered(double current_time);
  bool IsSafe();

  //void MarkBitMap(int num);

  void InsertACKRevLink(Ptr<Packet> p, double d);
  void InsertACKRevLink(Ptr<Packet> p, double* d);
  void TxData(Address receiver);
  void ClearACKRevLink();
  void StartRECV(double dt,int id,int dataSender);
  void SetNextHop();
  void DeleteBufferCell(Ptr<Packet>);
  void DeleteRecord(int index);
  void CancelReservation();
  void CancelREVtimeout();
  void PrintTable();
  void ResumeTxProcess();
  void ClearChannel();

  //void InsertNextAvailableTable(int,double);
  //void ProcessNextAvailableTable();
  //double CheckNextAvailableTable(int);

  EventId m_largeNdEvent;
  EventId m_shortNdEvent;
  EventId m_statusEvent;
  // Event m_largeAckndEvent;
  EventId m_shortAckndEvent;
  EventId m_phaseOneEvent;
  EventId m_phaseTwoEvent;
  EventId m_phaseThreeEvent;
  EventId m_sleepEvent;
  EventId m_wakeupEvent;
  EventId m_timeoutEvent;
  EventId m_transmissionEvent;
  EventId m_macRecvEvent;
  EventId m_clearChannelEvent;
  EventId m_ackWindowEvent;
  EventId m_carrierSenseEvent;

  //Node* node(void) const {return node_;}
    // to process the incoming packet
  virtual void RecvProcess(Ptr<Packet> p);

    // to process the outgoing packet
  virtual void TxProcess(Ptr<Packet> p);

}; // class AquaSimRMac

} // namespace ns3

#endif /* AQUA_SIM_RMAC_H */
