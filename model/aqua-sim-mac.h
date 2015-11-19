 
//....



#ifndef AQUA_SIM_MAC_H
#define AQUA_SIM_MAC_H

#include "ns3/packet.h"	
#include "ns3/address.h"
#include "ns3/callback.h"

#include "aqua-sim-node.h"
#include "aqua-sim-routing.h"
//#include "aqua-sim-address.h"  
#include <string>

namespace ns3{

class AquaSimNode;
class AquaSimRouting;
class AquaSimPhy;
class Packet;

class AquaSimMac : public Object {
public:
  AquaSimMac(void);
  virtual ~AquaSimMac(void);

  static TypeId GetTypeId(void);

  //Ptr<AquaSimNode> Node() { return m_node; }
  Ptr<AquaSimPhy> Phy() { return m_phy; }
  Ptr<AquaSimRouting> Routing() { return m_rout;}

  virtual void SetNode(Ptr<AquaSimNode> node);
  virtual void SetPhy(Ptr<AquaSimPhy> phy);
  virtual void SetRouting(Ptr<AquaSimRouting> rout);

  //interfaces for derived MAC protocols
  // to process the incoming packet
  virtual  void RecvProcess(Ptr<Packet>);
  // to process the outgoing packet
  virtual  void TxProcess(Ptr<Packet>);

  //interfaces for derived base MAC classes
  virtual void HandleIncomingPkt(Ptr<Packet>);
  virtual void HandleOutgoingPkt(Ptr<Packet>);

	  //Do we need a to address in the cb?
  virtual void SetForwardUpCallback(Callback<void, Ptr<Packet>, Address> upCallback);
  //virtual void SetLinkUpCallback(Callback<void> linkUp);
  //virtual void SetLinkDownCallback(Callback<void> linkDown);
  virtual  void SendUp(Ptr<Packet>);
  virtual  void SendDown(Ptr<Packet>);

  void PowerOff(void);
  void PowerOn(void);
  void InterruptRecv(double);

  double GetTxTime(int pktLen, std::string * modName = NULL);
  double GetTxTime(Ptr<Packet> pkt, std::string * modName = NULL);
  int  GetSizeByTxTime(double txTime, std::string * modName = NULL); //get packet size by txtime
  // The sending process can stop receiving process and change the transmission
  // status of the node since underwatermac is half-duplex

  double GetPreamble(void);

  void Recv(Ptr<Packet>);	//TODO move to private once non-base MAC classes available

private:
  // to receive packet from upper layer and lower layer
  //we hide this interface and demand derived classes to
  //override RecvProcess and TxProcess

  /*
   * virtual void Recv(Ptr<Packet>);	//handler not imlemented... handler can be 0 unless needed in operation
  */

protected:
  Ptr<AquaSimNode> m_node;// the node this mac is attached
  Ptr<AquaSimPhy> m_phy;
  Ptr<AquaSimRouting> m_rout;

  Callback<void, Ptr<Packet>, Address> m_callback;  // for the upper layer protocol
};  //class AquaSimMac

}  // namespace ns3

#endif /* AQUA_SIM_MAC_H */

