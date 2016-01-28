 
//....



#ifndef AQUA_SIM_MAC_H
#define AQUA_SIM_MAC_H


#include <string>

#include "ns3/object.h"
#include "ns3/address.h"
#include "ns3/packet.h"	
#include "ns3/nstime.h"
#include "ns3/callback.h"
#include "aqua-sim-net-device.h"
#include "aqua-sim-phy.h"
#include "aqua-sim-routing.h"

//#include "aqua-sim-address.h"  

namespace ns3{

class AquaSimRouting;

class AquaSimMac : public Object {
public:
  AquaSimMac(void);
  ~AquaSimMac(void);

  static TypeId GetTypeId(void);

  Ptr<AquaSimNetDevice> Device() {return m_device; }
  Ptr<AquaSimPhy> Phy() { return m_phy; }
  Ptr<AquaSimRouting> Routing() { return m_rout;}

  virtual void SetDevice(Ptr<AquaSimNetDevice> device);
  virtual void SetPhy(Ptr<AquaSimPhy> phy);
  virtual void SetRouting(Ptr<AquaSimRouting> rout);

  /*
   * TODO address needs to be integrated into MAC layer
   */
  virtual Address GetAddress(void) {return m_address; }
  virtual void SetAddress(Address addr);

  //interfaces for derived MAC protocols
  // to process the incoming packet
  virtual void RecvProcess(Ptr<Packet> p);
  // to process the outgoing packet
  virtual void TxProcess(Ptr<Packet> p);

  //interfaces for derived base MAC classes
  virtual void HandleIncomingPkt(Ptr<Packet> p);
  virtual void HandleOutgoingPkt(Ptr<Packet> p);

	  //Do we need a to address in the cb? TODO set up up callback
  virtual void SetForwardUpCallback( Callback<void, const Address&> upCallback );
  //virtual void SetLinkUpCallback(Callback<void> linkUp);
  //virtual void SetLinkDownCallback(Callback<void> linkDown);
  virtual bool SendUp(Ptr<Packet> p);
  virtual bool SendDown(Ptr<Packet> p);

  void PowerOff(void);
  void PowerOn(void);
  void InterruptRecv(double txTime);

  Time GetTxTime(int pktLen, std::string * modName = NULL);
  Time GetTxTime(Ptr<Packet> pkt, std::string * modName = NULL);
  int  GetSizeByTxTime(double txTime, std::string * modName = NULL); //get packet size by txtime
  // The sending process can stop receiving process and change the transmission
  // status of the node since underwatermac is half-duplex

  double GetPreamble(void);

  bool Recv(Ptr<Packet> p);	//TODO move to private once non-base MAC classes available

private:
  // to receive packet from upper layer and lower layer
  //we hide this interface and demand derived classes to
  //override RecvProcess and TxProcess

  /*
   * virtual void Recv(Ptr<Packet>);	//handler not imlemented... handler can be 0 unless needed in operation
  */

protected:
  Ptr<AquaSimNetDevice> m_device;// the device this mac is attached
  Ptr<AquaSimPhy> m_phy;
  Ptr<AquaSimRouting> m_rout;
  Address m_address;

  Callback<void,const Address&> m_callback;  // for the upper layer protocol
};  //class AquaSimMac

}  // namespace ns3

#endif /* AQUA_SIM_MAC_H */

