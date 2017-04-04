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
#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/log.h"

#include <vector>
#include <cstdio>
#include <algorithm>
#include <math.h>

using namespace ns3;

#define Malloc(type,n) (type *)malloc((n)*sizeof(type))

NS_LOG_COMPONENT_DEFINE("AquaSimDDOS");
NS_OBJECT_ENSURE_REGISTERED(AquaSimDDOS);

AquaSimDDOS::AquaSimDDOS() :
  m_totalPktSent(0), m_totalPktRecv(0), //m_dataCacheSize(10),
  isAttacker(false), m_pushbackReduction(0.01), m_throttleReduction(0.015),
  m_statisticalIteration(0)
{
  m_pitEntryTimeout = Seconds(120);
  m_ddosCheckFrequency = Minutes(2);//Minutes(10);
  m_pushbackLength = Minutes(3);//Hours(12);
  m_throttleLength = Minutes(3);//Hours(12);
  m_analysisFreq = Minutes(30);//Hours(24);
  //real system values may vary and be much larger (as seen in comments)
  m_ruleMiningFreq = 3;
  m_svmLearningFreq = Minutes(20); // not currently used.

  Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();

  m_data.clear();
  m_knownDataPath.clear();
  m_baseInterest = "/videos/crab/v";
  m_interestVersionNum = 0;

  //for attacker(s)
  m_possibleBaseInt.push_back("/videos/crab/v");
  m_possibleBaseInt.push_back("/videos/shark/v");
  m_possibleBaseInt.push_back("/videos/whale/v");

  /*current rule sets:
      mobility & pushback & timeout
      mobility & throttle & timeout
      pushback & timeout
      throttle & timeout
  */
  double rulesets[] = {0.5, 0.25, 0.5,
                    0.5, 0.25, 0.5,
                    0.5, 0.5,
                    0.5, 0.5};
  m_rules = std::vector<double>(rulesets, rulesets + sizeof(rulesets) / sizeof(double) );

  Time delay = (m_ddosCheckFrequency + m_ddosCheckFrequency * m_rand->GetValue()); //add a slight variation
  Simulator::Schedule(delay, &AquaSimDDOS::DdosAttackCheck, this);

  //NOTE: currently disabled:
  Simulator::Schedule(m_analysisFreq,&AquaSimDDOS::Analysis,this);
  //used to activate/deactivate statistical/machine learning

  sinkCounter=attackCounter=0;

  #if LIBSVM
  //default svm settings
  m_param.svm_type = C_SVC;
  m_param.kernel_type = LINEAR;
  m_param.degree = 3;	/* for poly */
	m_param.gamma = 0.01;	/* for poly/rbf/sigmoid */
	m_param.coef0 = 0;	/* for poly/sigmoid */
  m_param.nu = 0.01;
  m_param.cache_size = 100;
  m_param.C = 0.5;
  m_param.eps = 0.01;
  m_param.p = 0.1;
  m_param.shrinking = 0;
  m_param.probability = 0;
  m_param.nr_weight = 0;
  m_param.weight_label = NULL;
  m_param.weight = NULL;
  #endif

  m_modelTrained=false;
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
    .AddAttribute ("MinSupport", "Minimum support to adjust rules for Rules Mining.",
      DoubleValue(0.3),
      MakeDoubleAccessor(&AquaSimDDOS::m_minSupport),
      MakeDoubleChecker<double>())
    .AddAttribute ("MinConfidence", "Minimum confidence to adjust rules for Rules Mining.",
      DoubleValue(0.3),
      MakeDoubleAccessor(&AquaSimDDOS::m_minConfidence),
      MakeDoubleChecker<double>())
    .AddAttribute ("MinCompTrans", "Minimum compromised transactions needed to adjust rules for Rules Mining.",
      IntegerValue(20),
      MakeIntegerAccessor(&AquaSimDDOS::m_minCompTrans),
      MakeIntegerChecker<int>())
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
  NS_LOG_FUNCTION(this << p << "@time" << Simulator::Now().ToDouble(Time::S));

  Ptr<Packet> pkt = p->Copy();
  AquaSimHeader ash;
  DDOSHeader dh;
  pkt->RemoveHeader(ash);
  pkt->RemoveHeader(dh);
  ash.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetDirection(AquaSimHeader::DOWN);
  ash.SetTimeStamp(Simulator::Now());
  ash.SetNumForwards(1);
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
  ash.SetNumForwards(1);
  dh.SetPacketType(DDOSHeader::Data);
  dh.SetRowIndex(GetNetDevice()->GetIfIndex());

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
  NS_LOG_DEBUG("SendAlert from node " << GetNetDevice()->GetAddress() << " at time " <<
        Simulator::Now().ToDouble(Time::S) << " for node " << attackerAddr);
  Ptr<Packet> pkt = Create<Packet>();

  AquaSimHeader ash;
  DDOSHeader dh;

  ash.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
  ash.SetDAddr(AquaSimAddress (attackerAddr));
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetDirection(AquaSimHeader::DOWN);
  ash.SetTimeStamp(Simulator::Now());
  ash.SetErrorFlag(false);
  ash.SetNumForwards(1);
  dh.SetPacketType(DDOSHeader::Alert);
  dh.SetRowIndex(255);  //arbitrary _large_ number

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

  packet->RemoveHeader(ash);
  if (ash.GetNumForwards() <= 0)  //no headers
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
    //packet->RemoveHeader(ash);
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

  if(!NodeContainsDataPath(interest) && dh.GetPacketType() != DDOSHeader::Alert)
  {
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
        //TODO callback needs to be integrated, count # of pkts
        NS_LOG_INFO("     Sink(" << GetNetDevice()->GetAddress() << ") recved interest:"
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
    //      towards the original data requestor. drop packet
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
    for (; it!=(it_pit->second).nodeID.end(); ++it) {
      (DdosDetectionTable.find(*it)->second).entriesTimeout++;
      if (AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt() == 2)
        std::cout << "timeout:" << *it << " for " << interest << " @ " << Simulator::Now().ToDouble(Time::S) <<"\n";
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

    if(timeoutRatio>1) timeoutRatio=1;

    NS_LOG_DEBUG("DdosAttackCheck valueDump:" << localNodeId <<
        " timeoutRatio:" << timeoutRatio << "(" << (localDdosTable).entriesTimeout << "/" << (localDdosTable).interestEntries <<
        ") channelMeasurement:" << channelMeasurement << "("<< (localDdosTable).NAck << "/" << m_totalPktSent <<
        ") transDominance:" << transDominance << "(" << (localDdosTable).pktRecv << "/" << m_totalPktRecv <<
        ") pitUsage:" << pitUsage << "(" << pitUsageCounter.find(localNodeId)->second << "/" << totalPitUsage <<
        ") timeoutThreshold:" << (localDdosTable).timeoutThreshold <<
        " maxThreshold:" << (localDdosTable).maxThreshold);

    bool potentialAttack = false;
    if (((1+channelMeasurement)*timeoutRatio >= (localDdosTable).timeoutThreshold)  &&
            (pitUsage >= (localDdosTable).maxThreshold) )
      {
        Pushback(localNodeId);
        potentialAttack=true;
      }
    if (((1+channelMeasurement)*timeoutRatio >= (localDdosTable).timeoutThreshold)  &&
            (transDominance >= (localDdosTable).maxThreshold) )
      {
        Throttle(localNodeId);
        potentialAttack=true;
        /* (idea here is if throttling isn't detected quickly it can lead to multiple
              throttles downstream, which will eventually reset once the entry node
              attackee fully throttles the attacker.) */
      }

    //collect for SVM
    SvmTable.push(SvmInput(timeoutRatio,std::max(transDominance,pitUsage),potentialAttack));
    //svm classification
    if (m_modelTrained) {
      m_space = Malloc(struct svm_node,3);
      m_space[0].index=0;
      m_space[0].value=timeoutRatio;
      m_space[1].index=1;
      m_space[1].value=std::max(transDominance,pitUsage);
      m_space[2].index=-1;

      /*
      std::cout << "Predicted(" << GetNetDevice()->GetAddress() << ") @" << Simulator::Now().ToDouble(Time::S) <<
        ":" << localNodeId << "," << svm_predict(m_model,m_space) << "\n";
      */
      free(m_space);
    }

    //collect data, over time, for machine learning case:
    MachineLearningStruct mlTable;
    mlTable.mobility = 0;  //not currently used
    mlTable.pushback = Normalize(pitUsage, (localDdosTable).maxThreshold);
    mlTable.throttle = Normalize(transDominance, (localDdosTable).maxThreshold);
    mlTable.timeout = Normalize(((1+channelMeasurement)*timeoutRatio), (localDdosTable).timeoutThreshold);

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
  if (v2==0) return 0;
  double v3 = v1 / v2;
  if (v3 >= 1) return 1;
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

void AquaSimDDOS::Analysis()
{
  if (isAttacker) return;
  NS_LOG_FUNCTION(this);

  std::vector<StatisticalTable> statScores = Statistical();
  //TODO do something w/ statScores.
  //This is used for statistical analyze printout and comparisons.
  std::cout << "Source(" << GetNetDevice()->GetAddress() << ") @" << Simulator::Now().ToDouble(Time::S) << ":";
  for (std::vector<StatisticalTable>::const_iterator i = statScores.begin(); i!=statScores.end(); i++)
    std::cout << (*i).nodeID << ',' << (*i).score << " | ";
  std::cout << "\n";

  // TODO should verify all entries in SvmTable are accurate either here (for each analysis call) or when added to SvmTable in DdosAttackCheck().
  // if m_svmLearningFreq, then SVM();
  // need a classification step (for each newly gathered entry (can occur in DdosAttackCheck())) whereas need a
  //    training step which can occur here every so often (such as occur when rules mining does or even less.)

  if(m_statisticalIteration >= m_ruleMiningFreq) {
    std::vector<StatisticalTable> ruleStatScores = RulesMining();
    std::cout << " rulesMining: ";
    for (std::vector<StatisticalTable>::const_iterator j = ruleStatScores.begin(); j!=ruleStatScores.end(); j++)
      std::cout << (*j).nodeID << ',' << (*j).score << " | ";
    std::cout << "\n";

    #if LIBSVM
    SVM();
    #endif
  }

  Simulator::Schedule(m_analysisFreq,&AquaSimDDOS::Analysis,this);
}

std::vector<StatisticalTable> AquaSimDDOS::Statistical()
{
  NS_LOG_FUNCTION(this);

  std::vector<StatisticalTable> scores;

  std::map<int,ml>::iterator it=StatLearningTable.begin();
  for (; it!=StatLearningTable.end(); it++) {
    int qSize = (it->second).size();
    if (qSize == 0) continue;
    MachineLearningStruct currentDist, previousDist;
    //for longterm history:
    std::pair<std::map<int,ml>::iterator,bool> rulesLearning;
    ml newML;
    rulesLearning = RuleMiningTable.insert(std::pair<int,ml>(it->first,newML));

    // finding node's (it->first) distribution
    while(!(it->second).empty()) {
      currentDist.mobility += (it->second).front().mobility;
      currentDist.pushback += (it->second).front().pushback;
      currentDist.throttle += (it->second).front().throttle;
      currentDist.timeout += (it->second).front().timeout;
      ((rulesLearning.first)->second).push((it->second).front());
      (it->second).pop();
    }

    currentDist.mobility /= qSize;
    currentDist.pushback /= qSize;
    currentDist.throttle /= qSize;
    currentDist.timeout /= qSize;

    //rounding
    currentDist.mobility = floor(currentDist.mobility * pow(10,4)) / pow(10,4);
    currentDist.pushback = floor(currentDist.pushback * pow(10,4)) / pow(10,4);
    currentDist.throttle = floor(currentDist.throttle * pow(10,4)) / pow(10,4);
    currentDist.timeout = floor(currentDist.timeout * pow(10,4)) / pow(10,4);

    //get previous statistical distrubtion
    std::map<int,MachineLearningStruct>::iterator prev_stat;
    prev_stat = PreviousStatDistribution.find(it->first);
    if (prev_stat == PreviousStatDistribution.end()) {
      PreviousStatDistribution.insert(std::pair<int,MachineLearningStruct>(it->first,currentDist));
      scores.push_back(StatisticalTable(it->first,1)); //report score of 1 for new entry
    }
    else {
      previousDist = prev_stat->second;
      MachineLearningStruct temp;
      //absolute subtration:
      temp.mobility = (currentDist.mobility <= previousDist.mobility) ? (previousDist.mobility - currentDist.mobility) : (currentDist.mobility - previousDist.mobility);
      temp.pushback = (currentDist.pushback <= previousDist.pushback) ? 0 : (currentDist.pushback - previousDist.pushback);
      temp.throttle = (currentDist.throttle <= previousDist.throttle) ? 0 : (currentDist.throttle - previousDist.throttle);
      temp.timeout = (currentDist.timeout <= previousDist.timeout) ? 0 : (currentDist.timeout - previousDist.timeout);
      //NOTE:timeout could be more sensitive to allow for better detection.

      //normalize summation to scale of 0,1 & report score.
      double tempScore = Normalize((temp.mobility + temp.pushback + temp.throttle + temp.timeout),4);
      scores.push_back(StatisticalTable(it->first,tempScore));
      previousDist = currentDist;
    }

  }
  m_statisticalIteration++;
  return scores;
}

void AquaSimDDOS::SVM()
{
  NS_LOG_FUNCTION(this);
  //NOTE: this is for training model, classification should be separate.
  //XXX should trained data set be presistent or removed from queue?
  if (SvmTable.size() < 20) return; //may need to change this around.

  m_prob.l = SvmTable.size();
  size_t elements, j;
  elements = m_prob.l * 3;
  j=0;

  m_prob.y = Malloc(double,m_prob.l);
  m_prob.x = Malloc(struct svm_node *,m_prob.l);
  m_space = Malloc(struct svm_node,elements);

  int i=0;
  while(!SvmTable.empty()) {
    //LIBSVM expected layout: <label> <index1>:<value1> <index2>:<value2> ...
    SvmInput entry=SvmTable.front();
    m_prob.x[i] = &m_space[j];
    m_prob.y[i] = (entry.compromised)?1:0;

    //boundry check.
    NS_ASSERT(entry.timeoutR >= 0 && entry.timeoutR <= 1 || entry.maxR >= 0 && entry.maxR <= 1);

    m_space[j].index = 0;
    m_space[j].value = entry.timeoutR;
    ++j;
    m_space[j].index = 1;
    m_space[j].value = entry.maxR;
    m_space[++j].index = -1;

    SvmTable.pop();
    i++;
  }

  const char *error_msg;
  error_msg = svm_check_parameter(&m_prob,&m_param);
  if(error_msg) {
    NS_LOG_ERROR("SVM Error:" << error_msg);
    return;
  }

  // -------------- cross validation ----------------------------
/*
  int k, total_correct = 0, nr_fold = 2;
  double *target = Malloc(double,m_prob.l);

  svm_cross_validation(&m_prob,&m_param,nr_fold,target);

  for(k=0;k<m_prob.l;k++)
    if(target[k] == m_prob.y[k])
      ++total_correct;
  printf("Cross Validation Accuracy = %g%%\n",100.0*total_correct/m_prob.l);
  free(target);
*/

  // ------------- training and saving model -------------------
  m_model = svm_train(&m_prob,&m_param);
  if(svm_save_model("ddos_svm.model",m_model)) {
    NS_LOG_WARN("Not able to save SVM model to file");
    return;
  }
  m_modelTrained=true;

  free(m_prob.y);
  free(m_prob.x);
  free(m_space);
}

std::vector<StatisticalTable> AquaSimDDOS::RulesMining()
{
  NS_LOG_FUNCTION(this);

  std::vector<StatisticalTable> nodeScores;
  std::queue<std::vector<bool> > compromisedTransQueue;

  //for each nodeID
  std::map<int,ml>::iterator rm_it=RuleMiningTable.begin();
  for (; rm_it!=RuleMiningTable.end(); rm_it++) {
    int compromisedTransactions=0;
    int totalTransactions= (rm_it->second).size();
    //review entire MachineLearningStruct queue for current nodeID
    for (int j=0;j<totalTransactions;j++) {
      /* (1) finding amount of compromised vs uncompromised components for each transaction */
      int compromisedRules=0;
      int totalRules=7;
      MachineLearningStruct currentTrans = (rm_it->second).front();
      /* The rule sets we are looking at */
      std::vector<double> transaction = {currentTrans.mobility, currentTrans.pushback,currentTrans.timeout,
                                        currentTrans.mobility,currentTrans.throttle,currentTrans.timeout,
                                        currentTrans.pushback,currentTrans.timeout,
                                        currentTrans.throttle,currentTrans.timeout};
      std::vector<bool> binaryTransaction;
      for(unsigned i=0; i<m_rules.size(); i++) {
        if(transaction[i]>m_rules[i]) {
          compromisedRules++;
          binaryTransaction.push_back(true);
        } else  binaryTransaction.push_back(false);

      }
      /* (2) if compromised >= uncompromised rule sets then set transaction to compromised */
      if (compromisedRules>=totalRules/2) {
        compromisedTransactions++;
        compromisedTransQueue.push(binaryTransaction); //used for step 4
      }
    (rm_it->second).pop();
    }
    /* (3) compare compromised transactions vs uncompromised for all of node's transactions */
    if (compromisedTransactions>totalTransactions/2) {
      nodeScores.push_back(StatisticalTable(rm_it->first,1));  //1 = comrpromised, 0 = not.
    } else {
      nodeScores.push_back(StatisticalTable(rm_it->first,0));
    }
  }

  //(4) use above list of compromised transactions to adjust the set of rules for all neighboring nodes
  //NOTE abbreviations: pushback = Pb, timeout = To, throttle = Th, Mobility = Mb
  int MbPbTo_sum=0, MbThTo_sum=0, PbTo_sum=0, ThTo_sum=0;  //support summations
  int PbTo_conf=0, ThTo_conf=0, PbTo_mobility=0, ThTo_mobility=0; //confidence summations
  int totalQueueSize = compromisedTransQueue.size();
  std::vector<bool> compTransation;

  while(!compromisedTransQueue.empty()) {
    compTransation = compromisedTransQueue.front();

    //support frequencies
    if (compTransation[0] && compTransation[1] && compTransation[2]) MbPbTo_sum++;
    if (compTransation[3] && compTransation[4] && compTransation[5]) MbThTo_sum++;
    if (compTransation[6] && compTransation[7]) PbTo_sum++;
    if (compTransation[8] && compTransation[9]) ThTo_sum++;

    //confidence frequencies (between first two rules sets: mobility vs pushback/timeout and mobility vs throttle/timeout)
    if (compTransation[1] && compTransation[2]) PbTo_conf++;
    if (compTransation[4] && compTransation[5]) ThTo_conf++;
    if (compTransation[0]) PbTo_mobility++;
    if (compTransation[3]) ThTo_mobility++;

    compromisedTransQueue.pop();
  }

  /* Debug/printing purposes */
  if (1 && totalQueueSize>0) {
    std::cout << "Rules Mining on node(" << GetNetDevice()->GetAddress() << ") out of " << totalQueueSize << " compromised transactions.\n"
              << "RuleSet#1: Support(" << MbPbTo_sum<<"/"<<totalQueueSize << ") Confidence(" << PbTo_conf<<"/"<<PbTo_mobility << ")\n"
              << "RuleSet#2: Support(" << MbThTo_sum<<"/"<<totalQueueSize << ") Confidence(" << ThTo_conf<<"/"<<ThTo_mobility << ")\n"
              << "RuleSet#3: Support(" << PbTo_sum<<"/"<<totalQueueSize << ")\n"
              << "RuleSet#4: Support(" << ThTo_sum<<"/"<<totalQueueSize << ")\n";
  }

  /* Adjust the rules as necessary */
  if (totalQueueSize >= m_minCompTrans) {
    //short-circuit if zero denominator
    //TODO tweak rule adjustments below
    if ( MbPbTo_sum/totalQueueSize >= m_minSupport && PbTo_mobility>0 && PbTo_conf/PbTo_mobility >= m_minConfidence) {
      m_rules[0] += 0.02;
      m_rules[1] += 0.01;
      m_rules[2] += 0.02;
    }
    if ( MbThTo_sum/totalQueueSize >= m_minSupport && ThTo_mobility>0 && ThTo_conf/ThTo_mobility >= m_minConfidence) {
      m_rules[3] += 0.02;
      m_rules[4] += 0.01;
      m_rules[5] += 0.02;
    }
    if ( PbTo_sum/totalQueueSize >= m_minSupport) {
      m_rules[6] += 0.015;
      m_rules[7] += 0.015;
    }
    if ( ThTo_sum/totalQueueSize >= m_minSupport) {
      m_rules[8] += 0.015;
      m_rules[9] += 0.015;
    }
  }

  m_statisticalIteration=0;
  return nodeScores;
}

void
AquaSimDDOS::Pushback(int nodeID)
{
  DdosTable &ddosTable = DdosDetectionTable.find(nodeID)->second;
  ddosTable.maxThreshold = std::max(ddosTable.maxThreshold - m_pushbackReduction, 0.0);
  ddosTable.timeoutThreshold = std::max(ddosTable.timeoutThreshold - m_pushbackReduction, 0.0);
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
  ddosTable.maxThreshold = std::max(ddosTable.maxThreshold - m_throttleReduction, 0.0);
  ddosTable.timeoutThreshold = std::max(ddosTable.timeoutThreshold - m_throttleReduction, 0.0);
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
  Ptr<UniformRandomVariable> randVar = CreateObject<UniformRandomVariable> ();

  if(isAttacker)
  {
    m_interestVersionNum = randVar->GetValue(1500,3000);
    //assuming all requests are illegitimate although random.


         //NOTE for multiple focused attacks

    //int attNum = AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt() - 15;
    //m_baseInterest = m_possibleBaseInt[0];  //set just to crab.


        //NOTE for spread attack.
    //m_baseInterest = m_possibleBaseInt[randVar->GetInteger(0,2)];
    //randVar=0;
  }
  else
  {
    m_interestVersionNum++;
  }
  //NOTE:editing below depenent on topology type.
  m_baseInterest = m_possibleBaseInt[randVar->GetInteger(0,2)];
  randVar=0;

  m_interestMsg.str("");
  m_interestMsg << m_baseInterest << m_interestVersionNum << '\0';
}

void AquaSimDDOS::DoDispose()
{
  #if LIBSVM
  svm_free_and_destroy_model(&m_model);
  svm_destroy_param(&m_param);
  #endif
  m_rand=0;
  AquaSimRouting::DoDispose();
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
