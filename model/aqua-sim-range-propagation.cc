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

#include "aqua-sim-range-propagation.h"
#include "aqua-sim-header.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/double.h"
#include "ns3/simulator.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("AquaSimRangePropagation");
NS_OBJECT_ENSURE_REGISTERED (AquaSimRangePropagation);


AquaSimRangePropagation::AquaSimRangePropagation()
{
}

TypeId
AquaSimRangePropagation::GetTypeId()
{
  static TypeId tid = TypeId("ns3::AquaSimRangePropagation")
    .SetParent<AquaSimPropagation> ()
    .AddConstructor<AquaSimRangePropagation> ()
    .AddAttribute("Bandwidth", "Bandwidth of propagation in Hz.",
      DoubleValue(4096),
      MakeDoubleAccessor(&AquaSimRangePropagation::m_bandwidth),
      MakeDoubleChecker<double>())
    .AddAttribute("Temperature", "Temperature of water (C).",
      DoubleValue(25),
      MakeDoubleAccessor(&AquaSimRangePropagation::m_temp),
      MakeDoubleChecker<double>())
    .AddAttribute("Salinty", "Salinty of water (ppt).",
      DoubleValue(35),
      MakeDoubleAccessor(&AquaSimRangePropagation::m_salinity),
      MakeDoubleChecker<double>())
    .AddAttribute("NoiseLvl", "Noise level in dB.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimRangePropagation::m_noiseLvl),
      MakeDoubleChecker<double>())
  ;
  return tid;
}

void
AquaSimRangePropagation::Initialize()
{
  m_bandwidth = 4096;
  m_temp = 25;
  m_salinity = 35;
  m_noiseLvl = 0;
}

/**
* only nodes within range will receive a copy
*/
std::vector<PktRecvUnit> *
AquaSimRangePropagation::ReceivedCopies (Ptr<AquaSimNetDevice> s,
               Ptr<Packet> p,
               std::vector<Ptr<AquaSimNetDevice> > dList)
{
  NS_LOG_FUNCTION(this << dList.size());
  NS_ASSERT(dList.size());

	std::vector<PktRecvUnit> * res = new std::vector<PktRecvUnit>;
	//find all nodes which will receive a copy
	PktRecvUnit pru;
	double dist = 0;

  AquaSimPacketStamp pstamp;
  p->PeekHeader(pstamp);

  Ptr<Object> sObject = s->GetNode();
  Ptr<MobilityModel> senderModel = sObject->GetObject<MobilityModel> ();

  unsigned i = 0;
  std::vector<Ptr<AquaSimNetDevice> >::iterator it = dList.begin();
  for(; it != dList.end(); it++, i++)
  {
    Ptr<Object> rObject = dList[i]->GetNode();
    Ptr<MobilityModel> recvModel = rObject->GetObject<MobilityModel> ();
    /*
    if (std::fabs(recvModel->GetPosition().x - senderModel->GetPosition().x) > pstamp.GetTxRange())
      break;
    */
    if ( (dist = senderModel->GetDistanceFrom(recvModel)) > pstamp.GetTxRange() && pstamp.GetTxRange() != -1)
      continue;

		pru.recver = dList[i];
		pru.pDelay = Time::FromDouble(dist / AcousticSpeed(std::fabs(recvModel->GetPosition().z - senderModel->GetPosition().z)),Time::S);
		pru.pR = RayleighAtt(dist, pstamp.GetFreq(), pstamp.GetPt());
		res->push_back(pru);

    NS_LOG_DEBUG("AquaSimRangePropagation::ReceivedCopies: Sender("
    << s->GetAddress() << ") Recv(" << (pru.recver)->GetAddress()
    << ") dist(" << dist << ") pDelay(" << pru.pDelay.GetMilliSeconds()
    << ") pR(" << pru.pR << ")" << " Pt(" << pstamp.GetPt() << ")" << senderModel->GetPosition() << " & " << recvModel->GetPosition());
	}
	return res;
}

/*
 * Gives the acoustic speed based on propagation conditions.
 * Model from Mackenzie, JASA, 1981.
 *
 *  input: node depth
 *  returns: m/s
 */
double
AquaSimRangePropagation::AcousticSpeed(double depth)
{
  double s = m_salinity - 35;
  double d = depth/2;

  return ( 1448.96 + 4.591 * m_temp - 0.05304 * pow(m_temp,2) +
    0.0002374 * pow(m_temp,3) + 1.34 * s + 0.0163 * d +
    0.0000001675 * pow(d,2) - 0.01025 * m_temp * s -
    0.0000000000007139 * m_temp * pow(d,3) );
}

/*
 *  Urick Acoustic Model
 *
 *  Input: sender's NetDevice
 *         receiver's NetDevice
 *  Returns:  SNR
 */
double
AquaSimRangePropagation::Urick(Ptr<AquaSimNetDevice> sender, Ptr<AquaSimNetDevice> recver)
{
  Ptr<Object> sObject = sender->GetNode();
  Ptr<MobilityModel> senderModel = sObject->GetObject<MobilityModel> ();
  Ptr<Object> rObject = recver->GetNode();
  Ptr<MobilityModel> recverModel = rObject->GetObject<MobilityModel> ();

  double distance = senderModel->GetDistanceFrom(recverModel);
  double carrierFreq = sender->GetPhy()->GetFrequency() / 1000;  //Hz
  double spread = sender->GetPhy()->GetEnergySpread();
  double tempFreq = 21.9 * pow(10, 6 - 1520/(m_temp+273));

  double transmissionLoss = 10 * spread * std::log(distance) +
    ( 0.0186 * (m_salinity * tempFreq * pow(carrierFreq,2)) /
    (pow(tempFreq,2) + pow(carrierFreq,2)) + 0.0268 *
    pow(carrierFreq,2) / tempFreq ) * 0.0010936 * distance;

  double totalNoise = m_noiseLvl + 10 * std::log(m_bandwidth);

  //SNR = Transmission Source Level (dB) - TL - NL
  return (sender->GetPhy()->GetPt() - transmissionLoss - totalNoise);
}

void
AquaSimRangePropagation::SetBandwidth(double bandwidth)
{
  m_bandwidth = bandwidth;
}

void
AquaSimRangePropagation::SetTemp(double temp)
{
  m_temp = temp;
}

void
AquaSimRangePropagation::SetSalinity(double salinity)
{
  m_salinity = salinity;
}

void
AquaSimRangePropagation::SetNoiseLvl(double noiseLvl)
{
  m_noiseLvl = noiseLvl;
}

void
AquaSimRangePropagation::SetTraceValues(double temp, double salinity, double noiseLvl)
{
  m_temp = temp;
  m_salinity = salinity;
  m_noiseLvl = noiseLvl;
  NS_LOG_DEBUG("TraceValues(" << Simulator::Now().GetSeconds() << "):" << m_temp << "," << m_salinity << "," << m_noiseLvl);
}
