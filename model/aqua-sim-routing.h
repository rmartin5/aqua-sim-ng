//...

#ifndef AQUA_SIM_ROUTING_H
#define AQUA_SIM_ROUTING_H

//#include <stdio.h>
//#include <agent.h>
//#include <cmu-trace.h>
//#include <classifier-port.h>

#include "aqua-sim-mac.h"
#include "aqua-sim-node.h"
#include "aqua-sim-packetstamp.h"

#include "ns3/address.h"
#include "ns3/pointer.h"
#include "ns3/object.h"
//#include "ns3/ipv4.h"
//#include "ns3/ipv4-routing-protocol.h"
//#include "ns3/ipv4-static-routing.h"

namespace ns3 {

class AquaSimPacketStamp;
class AquaSimNode;
class AquaSimMac;
class Packet;

class AquaSimRouting : public Object
{	
public:	
/*define common commands for underwater routing protocols*/
	static TypeId GetTypeId(void);
	AquaSimRouting(void);
	virtual ~AquaSimRouting(void);

	/*avoid instantiation since UnderwaterRouting's behavior is not defined*/
	virtual void Recv(Ptr<Packet>) = 0;	//handler not implemented
protected:
	/*send packet up to port-demux*/
	virtual void SendUp(Ptr<Packet> p);			//demux not implemented yet.
	/*send packet p to next_hop after delay*/
	virtual void SendDown(Ptr<Packet> p, Address &nextHop, Time delay);
	/*check if if a dead loop results in the incoming packet*/
	virtual bool IsDeadLoop(Ptr<Packet> p);
	/*check if this node is the next hop*/
	virtual bool AmINextHop(const Ptr<Packet> p);
	/*check if this node is the destination.*/
	virtual bool AmIDst(const Ptr<Packet> p);
	/*check if this node is the source node,
		* i.e., whose app layer generates this packet.*/
	virtual bool AmISrc(const Ptr<Packet> p);
protected:
	Address m_myAddr;  //the ip address of this node
	Ptr<AquaSimNode> m_node; //the host node
	//Ptr<Trace> m_traceTarget;       // Trace Target	TODO need to initiate tracing
	//NsObject *ll;			//pointer to link layer object
	//NsObject *port_dmux;

private:
	AquaSimPacketStamp* m_pStamp;
	Ptr<AquaSimMac> m_mac;

};  //AquaSimRouting class

}  //namespace ns3

#endif /* AQUA_SIM_ROUTING_H */
