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

#include "aqua-sim-routing-ddos.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-address.h"

#include "ns3/packet.h"
#include "ns3/boolean.h"
#include "ns3/log.h"

#include <cstdio>
#include <algorithm>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimDDOS");
NS_OBJECT_ENSURE_REGISTERED(AquaSimDDOS);

AquaSimDDOS::AquaSimDDOS() :
  m_totalPktSent(0), m_totalPktRecv(0), //m_dataCacheSize(10),
  isAttacker(false), m_pushbackReduction(0.01), m_throttleReduction(0.015),
  m_statisticalIteration(0)
{
  m_pitEntryTimeout = Seconds(60);
  m_ddosCheckFrequency = Minutes(4);//Minutes(10);
  m_pushbackLength = Minutes(5);//Hours(12);
  m_throttleLength = Minutes(5);//Hours(12);
  m_statisticalFreq = Seconds(80);//Hours(24);
  //real system values may vary and be much larger (as seen in comments)
  m_ruleMiningFreq = 3;


  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();

  m_data.clear();
  m_knownDataPath.clear();
  m_baseInterest = "/videos/crab/v";
  m_interestVersionNum = 0;

  //for attacker(s)
  m_possibleBaseInt.push_back("/videos/crab/v");
  m_possibleBaseInt.push_back("/videos/shark/v");
  m_possibleBaseInt.push_back("/videos/whale/v");

  Time delay = (m_ddosCheckFrequency + m_ddosCheckFrequency * m_rand->GetValue()); //add a slight variation
  Simulator::Schedule(delay, &AquaSimDDOS::DdosAttackCheck, this);
  Simulator::Schedule(m_statisticalFreq,&AquaSimDDOS::MachineLearning,this);
  //used to activate/deactivate machine learning
}

TypeId
AquaSimDDOS::GetTypeId()
{
  static TypeId tid = TypeId("ns3::AquaSimDDOS")
    .SetParent<AquaSimRouting> ()
    .AddConstructor<AquaSimDDOS> ()
    .AddAttribute ("Attacker", "Node set as an attacker node. Default(false).",
      BooleanValue(false),
      MakeBooleanAccessor(&AquaSimDDOS::isAttacker),
      MakeBooleanChecker())
  ;
  return tid;
}

void
AquaSimDDOS::SendNAck(Ptr<Packet> p)
{
  Ptr<Packet> pkt = p->Copy();
  AquaSimHeader ash;
  DDOSHeader dh;
  pkt->RemoveHeader(ash);
  pkt->RemoveHeader(dh);
  ash.SetDAddr(ash.GetSAddr());
  ash.SetDirection(AquaSimHeader::DOWN);
  ash.SetTimeStamp(Simulator::Now());
  dh.SetPacketType(DDOSHeader::NACK);

  pkt->AddHeader(dh);
  pkt->AddHeader(ash);

  Simulator::Schedule(Seconds(0), &AquaSimRouting::SendDown,this,
                        pkt,AquaSimAddress::GetBroadcast(),Seconds(0));
}

void
AquaSimDDOS::RecvNAck(Ptr<Packet> p) //NOTE TODO: should be called by phy layer in case that BER/SINR is too high.
{
  AquaSimHeader ash;
  p->PeekHeader(ash);

  std::map<int, DdosTable>::iterator it;
  it = DdosDetectionTable.find(ash.GetSAddr().GetAsInt());
  if (it == DdosDetectionTable.end()) {
    NS_LOG_DEBUG("No id entry in DdosDetectionTable exists.");
    return;
  }

  (it->second).NAck++;

  p->AddHeader(ash);
  SendInterest(p);
}

void
AquaSimDDOS::SendInterest(Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this << "@time" << Simulator::Now().ToDouble(Time::S));

  Ptr<Packet> pkt = p->Copy();
  AquaSimHeader ash;
  DDOSHeader dh;
  pkt->RemoveHeader(ash);
  pkt->RemoveHeader(dh);
  ash.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetDirection(AquaSimHeader::DOWN);
  ash.SetTimeStamp(Simulator::Now());
  dh.SetPacketType(DDOSHeader::Interest);
  dh.SetRowIndex(GetNetDevice()->GetIfIndex());

  pkt->AddHeader(dh);
  pkt->AddHeader(ash);

  m_totalPktSent++;
  trafficPktsTrace++;
  trafficBytesTrace += p->GetSize();
  Simulator::Schedule(Seconds(0), &AquaSimRouting::SendDown,this,
                        pkt,AquaSimAddress::GetBroadcast(),Seconds(0));
}

void
AquaSimDDOS::SendData(Ptr<Packet> p)
{
  //Seperate from SendInterest for any future updates/dependencies.

  NS_LOG_FUNCTION(this << "@time" << Simulator::Now().ToDouble(Time::S));
  AquaSimHeader ash;
  DDOSHeader dh;
  p->RemoveHeader(ash);
  p->RemoveHeader(dh);
  ash.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetDirection(AquaSimHeader::DOWN);
  ash.SetTimeStamp(Simulator::Now());
  dh.SetPacketType(DDOSHeader::Data);

  p->AddHeader(dh);
  p->AddHeader(ash);

  //fill payload at end with zeros to reach expected data packet size of 1 kb
  p->AddAtEnd( Create<Packet>(128-p->GetSize()) );

  Simulator::Schedule(Seconds(0), &AquaSimRouting::SendDown,this,
                        p->Copy(),AquaSimAddress::GetBroadcast(),Seconds(0));

  m_totalPktSent++;
  trafficPktsTrace++;
  trafficBytesTrace += p->GetSize();
}

void
  AquaSimDDOS::SendAlert(int attackerAddr)
{
  NS_LOG_FUNCTION(this << GetNetDevice()->GetAddress() << "@time" << Simulator::Now().ToDouble(Time::S));
  Ptr<Packet> pkt = Create<Packet>();

  AquaSimHeader ash;
  DDOSHeader dh;

  ash.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
  ash.SetDAddr(AquaSimAddress (attackerAddr));
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetDirection(AquaSimHeader::DOWN);
  ash.SetTimeStamp(Simulator::Now());
  ash.SetErrorFlag(false);
  dh.SetPacketType(DDOSHeader::Alert);

  pkt->AddHeader(dh);
  pkt->AddHeader(ash);

  Simulator::Schedule(Seconds(0), &AquaSimRouting::SendDown,this,
                        pkt,AquaSimAddress::GetBroadcast(),Seconds(0));
  //m_totalPktSent++; not counting alert messages in this case.
}

bool
AquaSimDDOS::Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION(this << GetNetDevice()->GetAddress());

  /* for debugging purposes
  packet->Print(std::cout);
  std::cout << "\n";
  */

  AquaSimHeader ash;
  DDOSHeader dh;
  //TODO need to find a better way to handle application initiated packet.
  std::string interest;
  if (packet->GetSize() == 32)  //no headers
  {
    NS_LOG_DEBUG("RECV: new packet.");

    UpdateInterest();

    //might be better off creating unique application layer for this but quick work around is
    //  creating a new packet with interest within payload
    packet = Create<Packet> ((uint8_t*) m_interestMsg.str().c_str(), m_interestMsg.str().length());
    interest = m_interestMsg.str().c_str();

    dh.SetPacketType(DDOSHeader::Interest);
    dh.SetRowIndex(GetNetDevice()->GetIfIndex());
    ash.SetDirection(AquaSimHeader::DOWN);
    ash.SetNextHop(AquaSimAddress::GetBroadcast());
    ash.SetNumForwards(1);
    ash.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    ash.SetDAddr(AquaSimAddress::ConvertFrom(dest));
    ash.SetErrorFlag(false);
    ash.SetUId(packet->GetUid());

    packet->AddHeader(dh);
    packet->AddHeader(ash);

    //fill payload at end with zeros to reach expected interest packet size of 320 bits
    packet->AddAtEnd( Create<Packet>(40-packet->GetSize()) );
  }
  else
  {
    NS_LOG_DEBUG("RECV: updating packet.");
    if(isAttacker)
      {
        NS_LOG_DEBUG("Attacker received a packet, dropping.");
        return false;
      }
    packet->RemoveHeader(ash);
    packet->RemoveHeader(dh);
    ash.SetNumForwards(ash.GetNumForwards() + 1);

    uint8_t *payload = new uint8_t[packet->GetSize()];
    packet->CopyData (payload, packet->GetSize());
    interest = std::string((char*)payload);

    packet->AddHeader(dh);
    packet->AddHeader(ash);
  }

  std::map<int, DdosTable>::iterator it;
  it = DdosDetectionTable.find(ash.GetSAddr().GetAsInt());
  if (it == DdosDetectionTable.end()) {
    NS_LOG_DEBUG("No id entry in DdosDetectionTable exists, creating new.");
    DdosDetectionTable.insert( std::pair<int, DdosTable>(ash.GetSAddr().GetAsInt(),
                                          DdosTable(ash.GetSAddr().GetAsInt())));
  }

  if(!NodeContainsDataPath(interest) && dh.GetPacketType() != DDOSHeader::Alert) {
    NS_LOG_INFO("No Data Path found on node(" << GetNetDevice()->GetAddress() <<
                  ") from node " << ash.GetSAddr() << " for interest:" << interest);
    return false;
  }

  //TODO adding a restriction from potential attacker instead of bool may be better
  //    for future testing. ie. only allow so many interest pkts per a hour or
  //    only so many PIT usage per certain nodeID.

  if ((it->second).pushback.IsRunning() || (it->second).throttle.IsRunning()) {
    NS_LOG_INFO("Potential attack ignored from node:" << ash.GetSAddr());
    return false;
  }

  std::map<std::string, PIT_info>::iterator it_pit;
  it_pit = PIT.find(interest);
  bool entryFound = (it_pit != PIT.end());

  if (dh.GetPacketType() == DDOSHeader::Interest) {
    NS_LOG_DEBUG("Interest Pkt recved on node " << GetNetDevice()->GetAddress());
    (it->second).pktRecv++;
    m_totalPktRecv++;

    //remove flooding upstream effect by mimicing naming hierarchy as seen in NDN.
    // A more intuitive approach is required if all nodes are mobile in network.
    if (GetNetDevice()->GetIfIndex() < dh.GetRowIndex())
    {
      NS_LOG_DEBUG("Suppressing back stream effect of interest packet " <<
        interest << " on node " << GetNetDevice()->GetAddress());
      return false;
    }

    if (NodeContainsRelatedData(interest)) {
      NS_LOG_DEBUG("Related Data found for interest:" << interest);
      //since broadcast we don't need set interfaces/nodeIDs
      SendData(packet);
      return true;
    }

    if (!m_data.empty())
    {
      //is a data source and should not be retransmitting interests
      NS_LOG_DEBUG("Data Source(" << GetNetDevice()->GetAddress() <<
                    ") does not contain related data and therefore is dropping packet.");
      return false;
    }

    if (entryFound) {
      NS_LOG_DEBUG("PIT entry found for interest:" << interest << " | Dropping pkt.");
      //interest entry found in PIT
      (it_pit->second).nodeID.insert(ash.GetSAddr().GetAsInt());
      (it->second).interestEntries++;
      //packet dropped
      return true;
    }
    else {
      NS_LOG_DEBUG("Creating new entry in PIT for interest:" << interest);
      //interest entry not found in PIT, creating new one.
      //std::set will help restrict duplicate nodeIDs.
      std::set<int> nodeIDs;
      nodeIDs.insert(ash.GetSAddr().GetAsInt());
      PIT.insert( std::pair<std::string,PIT_info>(interest,PIT_info(nodeIDs)) );
      std::map<std::string, PIT_info>::iterator it_pit = PIT.find(interest); //to get newly inserted iterator value.

      /* slightly strange work around for timer container */
      it_pit->second.entryTimeout = Timer(Timer::CANCEL_ON_DESTROY);
      it_pit->second.entryTimeout.SetFunction(&AquaSimDDOS::RemoveEntry, this);
      it_pit->second.entryTimeout.SetArguments(interest,true);
      it_pit->second.entryTimeout.Schedule(m_pitEntryTimeout);

      (it->second).interestEntries++;
      SendInterest(packet);

      return true;
    }
  }
  else if (dh.GetPacketType() == DDOSHeader::Data) {
    NS_LOG_DEBUG("Data Pkt recved on node " << GetNetDevice()->GetAddress());
    if (entryFound) {
      if(GetNetDevice()->GetSinkStatus()) {
        //TODO callback needs to be integrated, count # of pkts recv
        NS_LOG_INFO("Sink(" << GetNetDevice()->GetAddress() << ") recved interest:"
            << interest << " at time " << Simulator::Now().ToDouble(Time::S));
      }
      else {
        SendData(packet);
      }
      RemoveEntry(interest,false);
      return true;
    }
    NS_LOG_DEBUG("Data Pkt: node(" << GetNetDevice()->GetAddress() <<
      ") does not reside on data requestor path or duplicate recv. Dropping Pkt.");
    //else: this node does not recide on the returning path
    //      towards the original data requestor. drop packet.
  }
  else if (dh.GetPacketType() == DDOSHeader::Alert) {
    NS_LOG_DEBUG("Alert Pkt recved on node " << GetNetDevice()->GetAddress());
    (it->second).timeoutThreshold = std::max((it->second).timeoutThreshold-m_pushbackReduction, 0.0);
    (it->second).maxThreshold = std::max((it->second).maxThreshold-m_throttleReduction, 0.0);
    return true;
  }
  else {
    NS_LOG_DEBUG("Recv: Something went wrong, DDOS packet type not setup correctly.");
  }

  return false;
}

bool
AquaSimDDOS::NodeContainsRelatedData(std::string interest)
{
  std::size_t intDivider = interest.find_last_of("/");
  std::string interestStr = interest.substr(intDivider+1);

  for (uint i=0; i < m_data.size(); i++)
  {
    if (interestStr == m_data[i])
    {
      return true;
    }
  }
  return false;
}

bool
AquaSimDDOS::NodeContainsDataPath(std::string interest)
{
  std::size_t pathDivider = interest.find_last_of("/");
  std::string path = interest.substr(0,pathDivider+1);
  NS_LOG_DEBUG("DataPath for interst:" << interest << " is:" << path);

  for (uint i=0; i < m_knownDataPath.size(); i++)
  {
    if (path == m_knownDataPath[i])
    {
      return true;
    }
  }
  return false;
}

void
AquaSimDDOS::RemoveEntry(std::string interest, bool isTimeout)
{
  NS_LOG_FUNCTION(this << GetNetDevice()->GetAddress() << interest << isTimeout);

  std::map<std::string, PIT_info>::iterator it_pit;
  it_pit = PIT.find(interest);
  if (it_pit == PIT.end()) {
      NS_LOG_WARN("RemoveEntry failed. no entry found for:" << interest);
      return;
  }

  if(isTimeout) {
    NS_LOG_DEBUG("Timeout of interest " << interest << " on node " << GetNetDevice()->GetAddress());
    std::set<int>::iterator it = (it_pit->second).nodeID.begin();
    for (; it!=(it_pit->second).nodeID.end(); it++) {
      (DdosDetectionTable.find(*it)->second).entriesTimeout++;
    }
  }

  (it_pit->second).entryTimeout.Cancel();
  PIT.erase(it_pit);
}

void
AquaSimDDOS::SetThresholds(double timeout, double max)
{
  m_timeoutThreshold = timeout;
  m_maxThreshold = max;

  //set for all DdosTables
  std::map<int,DdosTable>::iterator ddosTable=DdosDetectionTable.begin();
  for (; ddosTable!=DdosDetectionTable.end(); ddosTable++) {
    (ddosTable->second).timeoutThreshold = m_timeoutThreshold;
    (ddosTable->second).maxThreshold = m_maxThreshold;
  }
}

void
AquaSimDDOS::DdosAttackCheck()
{
  if (isAttacker) return;
  NS_LOG_FUNCTION(this);

  std::map<int,int> pitUsageCounter;
  int totalPitUsage = 0;

  std::map<int,DdosTable>::iterator node_it=DdosDetectionTable.begin();
  //review each neighboring nodes usage of the PIT
  for (; node_it!=DdosDetectionTable.end(); node_it++) {
    if (pitUsageCounter.count(node_it->first) == 0) {
      pitUsageCounter.insert( std::pair<int,int>(node_it->first,1));
    }
    else {
      pitUsageCounter.find(node_it->first)->second++;
    }
    totalPitUsage++;
  }

  std::map<int,DdosTable>::iterator it=DdosDetectionTable.begin();
  for (; it!=DdosDetectionTable.end(); it++) {
    int localNodeId = it->first;     // for readability
    DdosTable localDdosTable = it->second;

    if (localDdosTable.interestEntries == 0 || m_totalPktSent == 0 ||
                        m_totalPktRecv == 0 || totalPitUsage == 0)
      {
        continue;
      }
    if (localNodeId == AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt())
      {
        //don't review self
        continue;
      }


    double timeoutRatio = (double) (localDdosTable).entriesTimeout / (localDdosTable).interestEntries;
    double channelMeasurement = (double) (localDdosTable).NAck / m_totalPktSent;
    double transDominance = (double) (localDdosTable).pktRecv / m_totalPktRecv;
    double pitUsage = (double) pitUsageCounter.find(localNodeId)->second / totalPitUsage;

    NS_LOG_DEBUG("DdosAttackCheck valueDump:" << localNodeId <<
        " timeoutRatio:" << timeoutRatio << "(" << (localDdosTable).entriesTimeout << "/" << (localDdosTable).interestEntries <<
        ") channelMeasurement:" << channelMeasurement << "("<< (localDdosTable).NAck << "/" << m_totalPktSent <<
        ") transDominance:" << transDominance << "(" << (localDdosTable).pktRecv << "/" << m_totalPktRecv <<
        ") pitUsage:" << pitUsage << "(" << pitUsageCounter.find(localNodeId)->second << "/" << totalPitUsage <<
        ") timeoutThreshold:" << (localDdosTable).timeoutThreshold <<
        " maxThreshold:" << (localDdosTable).maxThreshold);

    if (((1+channelMeasurement)*timeoutRatio >= (localDdosTable).timeoutThreshold)  &&
            (pitUsage >= (localDdosTable).maxThreshold) )
      {
        Pushback(localNodeId);
      }
    if (((1+channelMeasurement)*timeoutRatio >= (localDdosTable).timeoutThreshold)  &&
            (transDominance >= (localDdosTable).maxThreshold) )
      {
        Throttle(localNodeId);
        /* (idea here is if throttling isn't detected quickly it can lead to multiple
              throttles downstream, which will eventually reset once the entry node
              attackee fully throttles the attacker.) */
      }

    //collect data, over time, for machine learning case:
    MachineLearningStruct mlTable;
    mlTable.mobility = 0;  //not currently used
    mlTable.pushback = Normalize(pitUsage, m_maxThreshold);
    mlTable.throttle = Normalize(transDominance, m_maxThreshold);
    mlTable.timeout = Normalize(((1+channelMeasurement)*timeoutRatio), m_timeoutThreshold);

    std::map<int,ml>::iterator ml_it;
    ml_it = StatLearningTable.find(localNodeId);
    if(ml_it == StatLearningTable.end())
      {
        ml newML;
        newML.push(mlTable);
        StatLearningTable.insert( std::pair<int,ml>(localNodeId,newML));
      }
    else
      {
        (ml_it->second).push(mlTable);
      }
  }
  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
  Time delay = m_ddosCheckFrequency + m_ddosCheckFrequency * m_rand->GetValue(); //add a slight variation
  Simulator::Schedule(delay, &AquaSimDDOS::DdosAttackCheck, this);
}

double
AquaSimDDOS::Normalize(double v1, double v2)
{
  double v3 = v1 / v2;
  if (v3 >= 1)
  {
    return 1;
  }
  return v3;
}


/* Notes on machine learning:
  can use a combination of Statical Distriubtion Model & Rules Mining Model.
    -Where SDM will serve has detecting an attacker over a short period of time
      by viewing the differences between last sample and current, as well as comparing
      this to the given rules.
    -RMM will happen slightly less then SDM, in which we will look back a larger history
      , lets say ~3x the normal frequency of SDM, and look for which rules should be edited.
      Such as reducing certain thresholds or looking for correlations between certain fields.
    -Both are similar to past work but the main difference is these learning models are more focused
      on long history data sets to analyze, instead of short bursts of data. So these may be
      helpful when it comes to small DDos attacks which may be hard to detect and targeted
      at straining the network instead of completely cutting off communication.

   Rules
  -Cannot have above a certain amount of PIT timeouts. Channel cond. is taken into account.
  -Main contributor to PIT usage OR total transmissions is a HINT to malicious behavior.
  -since both attack detections use this rule can just find both norm. values and find max()

  This should be edited and enhanced in future works.
*/
void
AquaSimDDOS::MachineLearning()
{
  if (isAttacker) return;
  NS_LOG_FUNCTION(this);

  std::map<int,ml>::iterator it=StatLearningTable.begin();
  for (; it!=StatLearningTable.end(); it++) {
    int qSize = (it->second).size();
    MachineLearningStruct tempTable;
    while(!(it->second).empty()) {
      tempTable.mobility += (it->second).front().mobility;
      tempTable.pushback += (it->second).front().pushback;
      tempTable.throttle += (it->second).front().throttle;
      tempTable.timeout += (it->second).front().timeout;
      (it->second).pop();
    }
    tempTable.mobility /= qSize;
    tempTable.pushback /= qSize;
    tempTable.throttle /= qSize;
    tempTable.timeout /= qSize;
    (it->second).push(tempTable);
      // for long term history.
    std::map<int,ml>::iterator ml_it;
    ml_it = RuleMiningTable.find(it->first);
    if(ml_it == RuleMiningTable.end()) {
      ml newML;
      newML.push(tempTable);
      RuleMiningTable.insert( std::pair<int,ml>(it->first,newML));
    }
    else {
      (ml_it->second).push(tempTable);
    }
  }

  if (m_statisticalIteration >= m_ruleMiningFreq) {
    std::map<int,ml>::iterator rm_it=RuleMiningTable.begin();
    for (; rm_it!=RuleMiningTable.end(); rm_it++) {
      //for each nodeID
      int queueSize = (rm_it->second).size();
      int attackPotential = 0;
      while (!(rm_it->second).empty()) {
        //analyze each entry within internal queue
        double analyze = (rm_it->second).front().mobility * 0.1 +
                         (rm_it->second).front().pushback * 0.3 +
                         (rm_it->second).front().throttle * 0.3 +
                         (rm_it->second).front().timeout * 0.3;
        if (analyze >= 0.6) attackPotential++;
        (rm_it->second).pop();
      }
      //adjust thresholdOffset if majority of queue entries have attacker characteristics (assigned to 1 for Machine Learning)
      if (attackPotential > queueSize/2) {
        (DdosDetectionTable.find(rm_it->first)->second).thresholdOffset += 0.1;
        //NOTE this may need to be tweaked.
      }
    }

    RuleMiningTable.clear();
  }
  m_statisticalIteration++;
  Simulator::Schedule(m_statisticalFreq,&AquaSimDDOS::MachineLearning,this);
}


void
AquaSimDDOS::Pushback(int nodeID)
{
  DdosTable &ddosTable = DdosDetectionTable.find(nodeID)->second;
  ddosTable.maxThreshold -= m_pushbackReduction;
  ddosTable.timeoutThreshold -= m_pushbackReduction;
  if(ddosTable.pushback.IsRunning())
    ddosTable.pushback.Cancel();
  ddosTable.pushback = Simulator::Schedule(m_pushbackLength, &AquaSimDDOS::ResetPushback, this,nodeID);

  //FilePrintout(true,AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt(),nodeID);
  SendAlert(nodeID);
}

void
AquaSimDDOS::Throttle(int nodeID)
{
  DdosTable &ddosTable = DdosDetectionTable.find(nodeID)->second;
  ddosTable.maxThreshold -= m_throttleReduction;
  ddosTable.timeoutThreshold -= m_throttleReduction;
  if(ddosTable.throttle.IsRunning())
    ddosTable.throttle.Cancel();
  ddosTable.throttle = Simulator::Schedule(m_throttleLength, &AquaSimDDOS::ResetThrottle, this,nodeID);

  //FilePrintout(false,AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt(),nodeID);
}

void
AquaSimDDOS::ResetPushback(int nodeID)
{
  NS_LOG_FUNCTION(this << "Pushback reset.");
  DdosTable &ddosTable = DdosDetectionTable.find(nodeID)->second;
  ddosTable.timeoutThreshold = m_timeoutThreshold;
  ddosTable.maxThreshold = m_maxThreshold - ddosTable.thresholdOffset;
}

void
AquaSimDDOS::ResetThrottle(int nodeID)
{
  NS_LOG_FUNCTION(this << "Throttle reset.");
  DdosTable &ddosTable = DdosDetectionTable.find(nodeID)->second;
  ddosTable.timeoutThreshold = m_timeoutThreshold;
  ddosTable.maxThreshold = m_maxThreshold - ddosTable.thresholdOffset;
}

void
AquaSimDDOS::UpdateInterest()
{
  if(isAttacker)
  {
    Ptr<UniformRandomVariable> randVar = CreateObject<UniformRandomVariable> ();
    m_interestVersionNum = randVar->GetValue(1500,3000);
    //assuming all requests are illegitimate although random.


         //NOTE for multiple focused attacks
    /*
    int attNum = AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt() - 15;
    m_baseInterest = m_possibleBaseInt[attNum];
    */

        //NOTE for spread attack.
    m_baseInterest = m_possibleBaseInt[randVar->GetInteger(0,2)];
  }
  else
  {
    m_interestVersionNum++;
  }

  m_interestMsg.str("");
  m_interestMsg << m_baseInterest << m_interestVersionNum << '\0';
}

/*
void
AquaSimDDOS::FilePrintout(bool pushback, int nodeID, int attackerID)
{
  //XXX should be removed and handled by FileHandler API from example/tracers.
  std::string defType;
  if (pushback) defType = "Pushback";
  else defType = "Throttle";

  FILE * pFile;
  pFile = fopen ("outputFile.txt", "a");
  if (pFile!=NULL)
  {
    fprintf (pFile, "%s:%i:%i\n",defType.c_str(),nodeID,attackerID);
    fclose (pFile);
  }
}
*/
