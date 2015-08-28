
//....

#ifndef AQUA_SIM_SINK_H
#define AQUA_SIM_SINK_H

#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/address.h"		
#include "ns3/mobility-model.h"
#include "ns3/random-variable-stream.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"

#include "aqua-sim-hash-table.h"
#include "aqua-sim-node.h"
#include "aqua-sim-phy.h"
//#include "vbf/vectorbasedforward.h"

#include <set>
using namespace std;

// Aqua Sim Sink

namespace ns3{

struct SenseAreaElem{
	double senseX;
	double senseY;
	double senseZ;
	double senseR;
	SenseAreaElem(double x, double y, double z, double r) {
		senseX = x;
		senseY = y;
		senseZ = z;
		senseR = r;
	}

	friend bool operator<(const SenseAreaElem&  e1, const SenseAreaElem& e2);
};

class SenseArea{
private:
	set<SenseAreaElem> AreaSet;
public:
	bool IsInSenseArea(double nx, double ny, double nz);
	void Insert(double x, double y, double z, double r);
}; // class SenseArea


// Class SinkAgent as source and sink for directed diffusion

class AquaSimHashTable;
class AquaSimPhy;

class AquaSimSink : public Object {

public:
	AquaSimSink(void);
	virtual ~AquaSimSink(void);
	static TypeId GetTypeId(void);

	virtual void Timeout(int);

	void Report(void);
	void Recv(Ptr<Packet>);  //handler not implemented
	void Reset(void);
	void SetAddr(Address address);
	int GetPktCount(void);
	void IncrPktCount(void);
	Ptr<Packet> CreatePacket(void);

protected:
	bool m_AppDup;
	bool m_periodic;
	static int m_pktId;
	//bool m_alwaysMaxRate;
	int m_pktCount;
	//  unsigned int m_dataType;
	int m_numRecv;
	int m_numSend;
	// int m_recvPerSec; //? what's this for

	/*used ti indicate if the sink is active, send out interest first or
	passive, it gets the data ready and then sends out the interest. 1 is passive
	and 0 is active.*/

	int m_passive;

	double m_targetX;
	double m_targetY;
	double m_targetZ;
	double m_range;

	int m_activeSense;
	SenseArea SenseAreaSet;
	double m_senseInterval;

	Address m_targetId;
	Address m_here;	//address of this sink

	char   m_fName[80];

	//the monitoring area. nodes within this area will send
	double m_senseX;
	double m_senseY;
	double m_senseZ;
	double m_senseR;

	Ptr<AquaSimNode> m_node;

	double m_cumDelay;
	double m_lastArrivalTime;

	Ptr<AquaSimHashTable>  DataTable;

	bool IsDeviation(void);
	void Terminate(void);
	void BcastInterest(void);
	void SourceDeny(uint32_t, double, double, double);
	void DataReady(void);
	void Start(void);
	void GenerateInterval(void);
	void ExponentialStart(void);
	void Stop(void);
	virtual void SendPkt(void);

	int m_running;
	int m_random;   //1 is random; 2 is exponential distribution
	int m_maxPkts;

	double m_interval; // interval to send data pkt
	double m_exploreInterval;
	double m_dataInterval;
	double  m_dataRate;

	int m_packetSize;  // # of bytes in the packet
	int m_exploreRate;
	int m_dataCounter;
	int  m_exploreCounter;
	int m_exploreStatus;

	Ptr<UniformRandomVariable> m_uniformRand;

	//int simple_report_rate;
	//  int data_counter;

	// Timer handlers
	Timer m_sinkTimer;
	Timer m_periodicTimer;
	Timer m_reportTimer;

	Ptr<AquaSimPhy> m_phy;

};  // class AquaSimSink

}  // namespace ns3

#endif /* AQUA_SIM_SINK_H */
