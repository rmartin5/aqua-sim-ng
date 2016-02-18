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

#ifndef AQUA_SIM_CHANNEL_H
#define AQUA_SIM_CHANNEL_H

#include "ns3/channel.h"
#include "ns3/mobility-model.h"
#include "ns3/nstime.h"

#include "aqua-sim-net-device.h"
#include "aqua-sim-propagation.h"
#include "aqua-sim-noise-generator.h"

namespace ns3 {

class AquaSimPhy;
class Packet;

class AquaSimChannel : public Channel
{
	//friend class Topography;
public:
  AquaSimChannel (void);
  ~AquaSimChannel (void);
        //virtual int command(int argc, const char*const* argv);
  static TypeId GetTypeId (void);
        //double TransmitDistance(){return distCST;}; 
  
  void SetNoiseGenerator (Ptr<AquaSimNoiseGen> noiseGen);
  void SetPropagation (Ptr<AquaSimPropagation> prop);
  bool Recv(Ptr<Packet>, Ptr<AquaSimPhy>);

  void AddDevice (Ptr<AquaSimNetDevice> device);
  void RemoveDevice(Ptr<AquaSimNetDevice> device);

  //inherited
  virtual Ptr<NetDevice> GetDevice (uint32_t i) const;
  uint32_t GetId (void) const;
  virtual uint32_t GetNDevices (void) const;
  Ptr<AquaSimNoiseGen> GetNoiseGen();

private:
  bool SendUp (Ptr<Packet> p, Ptr<AquaSimPhy> tifp);
  Time GetPropDelay (Ptr<AquaSimNetDevice> tdevice, Ptr<AquaSimNetDevice> rdevice);
  Ptr<MobilityModel> GetMobilityModel(Ptr<AquaSimNetDevice> device);
   	
	/* For list-keeper, channel keeps list of mobilenodes 
	   listening on to it */
	//int numNodes_;
	//MobileNode *xListHead_;
	//bool sorted_;

        //void calculatePosition(Node* sender,Node* receiver, Packet* p);
  double Distance(Ptr<AquaSimNetDevice> tdevice, Ptr<AquaSimNetDevice> rdevice);
	//void addNodeToList(MobileNode *mn);
	//void removeNodeFromList(MobileNode *mn);
	//void sortLists(void);
	//void updateNodesList(class MobileNode *mn, double oldX);
	//MobileNode **getAffectedNodes(MobileNode *mn, double radius, int *numAffectedNodes);
	
protected:
        //static double m_distCST;      
        //void EstTransLocation(MobileNode* sender, MobileNode* recver); // to be added by Robert to estimate receiving position
  Ptr<AquaSimPropagation> m_prop;
  Ptr<AquaSimNoiseGen> m_noiseGen;
  std::vector<Ptr<AquaSimNetDevice> > m_deviceList; 
};  // class AquaSimChannel

} // namespace ns3

#endif /* AQUA_SIM_CHANNEL_H */
