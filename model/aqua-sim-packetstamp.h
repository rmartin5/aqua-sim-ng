
//...


#ifndef AQUA_SIM_PACKETSTAMP_H
#define AQUA_SIM_PACKETSTAMP_H

#include "aqua-sim-phy.h"
#include "aqua-sim-signal-cache.h"
#include <string>
#include <map>

namespace ns3{

enum AquaSimMacDemuxPktType{ UWPTYPE_LOC, UWPTYPE_SYNC, UWPTYPE_OTHER };

class AquaSimPhy;

class AquaSimPacketStamp {
public:
	AquaSimPacketStamp(void);
	static TypeId GetTypeId(void);

	double & TxRange(void) { return m_txRange; }
	double & Pt(void) { return m_pt; }
	double & Pr(void) { return m_pr; }
	double & Freq(void) { return m_freq; }
	double & Noise(void) { return m_noise; }
	std::string & ModName(void) { return m_modName; }

	/*
	* Demux feature needs to be implemented
	AquaSimMacDemuxPktType & MacDemuxPType(void) { return m_macDemuxPType; }
	*/

	bool CheckConflict(void);	//check if parameters conflict
	void Stamp(Packet *, double, double);
private:
	/**
	* only one between m_ptLevel and m_txRange can should be set
	* run CheckConflict() to avoid conflict
	*/
	double m_pt;			//transmission power, set according to Pt_level_
	double m_pr;			//rx power, set by channel/propagation module
	double m_txRange;	//transmission range
	double m_freq;		//central frequency
	double m_noise;		//background noise at the receiver side
	std::string m_modName;
	/*
	* Demux features needs to be implemented
	*AquaSimMacDemuxPktType m_macDemuxPType;
	*/
};

} //namespace ns3

#endif /* AQUA_SIM_PACKETSTAMP_H */
