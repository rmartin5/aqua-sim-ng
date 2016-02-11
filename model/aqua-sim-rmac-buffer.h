/*
 * aqua-sim-rmac-buffer.h
 *
 *  Created on: Feb 11, 2016
 *      Author: Robert Martin
 */

#ifndef AQUA_SIM_RMAC_BUFFER_H_
#define AQUA_SIM_RMAC_BUFFER_H_

#include "ns3/packet.h"

#define MAXIMUM_BUFFER 1

namespace ns3{

struct buffer_cell{
  Ptr<Packet> packet;
  buffer_cell * next;
  double delay;
};


class TransmissionBuffer {
public:
  TransmissionBuffer(){
		head_=NULL;
		current_p=NULL;
		num_of_packet=0;
		lock=false;
		tail_=NULL;
		lock_p=NULL;
			};
  static TypeId GetTypeId (void);

  void AddNewPacket(Ptr<Packet> p);
  void LockBuffer();
  void UnlockBuffer();
  int  DeletePacket(Ptr<Packet> p);
  Ptr<Packet> dehead();
  Ptr<Packet> next();
  Ptr<Packet> head();
  bool  IsEnd();
  bool IsEmpty();
  bool IsFull();
  bool ToBeFull();
  bool IsLocked(){return lock;};
  buffer_cell * lookup(Ptr<Packet> p);
  int num_of_packet;// number of sending packets
  buffer_cell* head_;
  bool lock;
private:
     buffer_cell* current_p;
     buffer_cell* lock_p;
     buffer_cell* tail_;
};  // class TransmissionBuffer



}  // namespace ns3

#endif /* AQUA_SIM_RMAC_BUFFER_H_ */
