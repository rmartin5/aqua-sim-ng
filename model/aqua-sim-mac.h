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


#ifndef AQUA_SIM_MAC_H
#define AQUA_SIM_MAC_H

#include "aqua-sim-net-device.h"
//#include "aqua-sim-phy.h"
//#include "aqua-sim-routing.h"
#include "aqua-sim-address.h"

#include <string>
#include <queue>

#include "ns3/object.h"
#include "ns3/address.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/callback.h"
#include "ns3/traced-callback.h"


namespace ns3{

class AquaSimNetDevice;
class AquaSimPhy;
class AquaSimRouting;

class AquaSimMac : public Object {
public:
  AquaSimMac(void);
  ~AquaSimMac(void);

  static TypeId GetTypeId(void);

  Ptr<AquaSimNetDevice> Device();
  Ptr<AquaSimPhy> Phy();
  Ptr<AquaSimRouting> Routing();

  virtual void SetDevice(Ptr<AquaSimNetDevice> device);
  //virtual void SetPhy(Ptr<AquaSimPhy> phy);
  //virtual void SetRouting(Ptr<AquaSimRouting> rout);

  virtual Address GetAddress(void) {return this->m_address; }
  virtual void SetAddress(AquaSimAddress addr);

  //interfaces for derived MAC protocols
  // to process the incoming packet
  virtual bool RecvProcess(Ptr<Packet> p)=0;
  // to process the outgoing packet
  virtual bool TxProcess(Ptr<Packet> p)=0;

  //interfaces for derived base MAC classes
  virtual void HandleIncomingPkt(Ptr<Packet> p);
  virtual void HandleOutgoingPkt(Ptr<Packet> p);

	  //Do we need a to address in the cb? TODO set up up callback
  virtual void SetForwardUpCallback( Callback<void, const AquaSimAddress&> upCallback );
  //virtual void SetLinkUpCallback(Callback<void> linkUp);
  //virtual void SetLinkDownCallback(Callback<void> linkDown);
  virtual bool SendUp(Ptr<Packet> p);
  virtual bool SendDown(Ptr<Packet> p, TransStatus afterTrans = NIDLE);

  void PowerOff(void);
  void PowerOn(void);
  void InterruptRecv(double txTime);

  Time GetTxTime(int pktLen, std::string * modName = NULL);
  Time GetTxTime(Ptr<Packet> pkt, std::string * modName = NULL);
  double  GetSizeByTxTime(double txTime, std::string * modName = NULL); //get packet size by txtime
  // The sending process can stop receiving process and change the transmission
  // status of the node since underwatermac is half-duplex

  double GetPreamble(void);

  virtual void SetTransDistance(double range); //to be overloaded for certain cases.

  typedef void (* RxCallback)(std::string path, Ptr<Packet> p);
  typedef void (* TxCallback)(std::string path, Ptr<Packet> p);
  void NotifyRx(std::string context, Ptr<Packet> p);
  void NotifyTx(std::string context, Ptr<Packet> p);

  bool SendQueueEmpty();
  std::pair<Ptr<Packet>,TransStatus> SendQueuePop();
private:
  // to receive packet from upper layer and lower layer
  //we hide this interface and demand derived classes to
  //override RecvProcess and TxProcess
  void Recv(Ptr<Packet> p);

  TracedCallback<Ptr<const Packet> > m_macRxTrace;
  TracedCallback<Ptr<const Packet> > m_macTxTrace;
  /*
   * virtual void Recv(Ptr<Packet>);	//handler not imlemented... handler can be 0 unless needed in operation
  */
protected:
  Ptr<AquaSimNetDevice> m_device;// the device this mac is attached
  //Ptr<AquaSimPhy> m_phy;
  //Ptr<AquaSimRouting> m_rout;
  AquaSimAddress m_address;

  std::queue<std::pair<Ptr<Packet>,TransStatus> > m_sendQueue;

  Callback<void,const AquaSimAddress&> m_callback;  // for the upper layer protocol
};  //class AquaSimMac

}  // namespace ns3

#endif /* AQUA_SIM_MAC_H */
