/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 UWSN Lab at the University of Connecticut
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

#include "aqua-sim-channel.h"
#include "aqua-sim-packetstamp.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (AquaSimChannel);

AquaSimChannel::AquaSimChannel ()
  : m_prop(NULL),
    m_noiseGen(NULL)
{
}

AquaSimChannel::~AquaSimChannel ()
{
  //clear all lists
  Channel::DoDispose ();
}

TypeId
AquaSimChannel::GetTypeId ()
{
  static TypeId tid= TypeId ("ns3::AquaSimChannel")
    .SetParent<Channel> ()
    .AddConstructor<AquaSimChannel> ()
    .AddAttribute ("SetProp", "A pointer to set the propagation model.",
       PointerValue (CreateObject<AquaSimSimplePropagation> ()),
       MakePointerAccessor (&AquaSimChannel::m_prop),
       MakePointerChecker<AquaSimPropagation> ())
    .AddAttribute ("SetNoise", "A pointer to set the noise generator.",
       PointerValue (CreateObject<AquaSimConstNoiseGen> ()),
       MakePointerAccessor (&AquaSimChannel::m_noiseGen),
       MakePointerChecker<AquaSimNoiseGen> ())
  ;
  return tid;
}

void
AquaSimChannel::SetNoiseGenerator (Ptr<AquaSimNoiseGen> noiseGen)
{
  NS_ASSERT (noiseGen);
  m_noiseGen = noiseGen;
}

void
AquaSimChannel::SetPropagation (Ptr<AquaSimSimplePropagation> prop)
{
  NS_ASSERT (prop);
  m_prop = prop;
}

Ptr<AquaSimNetDevice>
AquaSimChannel::GetDevice (uint32_t i) const
{
  return m_deviceList[i];
}

uint32_t 
AquaSimChannel::GetId (void) const
{
  NS_LOG_WARN(this << " not implemented");
  return 0;
}

uint32_t
AquaSimChannel::GetNDevices (void) const
{
  return m_deviceList.size();
}

void
AquaSimChannel::AddDevice (Ptr<AquaSimNetDevice> device)
{
  m_deviceList.push_back(device);
}

void
AquaSimChannel::RemoveDevice(Ptr<AquaSimNetDevice> device)
{
  if (m_deviceList.empty())
    NS_LOG_WARN(this << " deviceList is empty");
  else
  {
    std::vector<Ptr<AquaSimNetDevice> >::const_iterator it = m_deviceList.begin();
    for(; it != m_deviceList.end(); ++it)
      {
        if(m_deviceList[it] == device)
          {
             m_deviceList.erase(m_deviceList.begin() + it);
          }
      }
  }
}

void
AquaSimChannel::Recv(Ptr<Packet> p, Ptr<AquaSimPhy> phy)
{
  NS_ASSERT(p != NULL || phy != NULL);
  SendUp(p,phy);
}

void
AquaSimChannel::SendUp (Ptr<Packet> p, Ptr<AquaSimPhy> tifp)
{ 
  NS_LOG_FUNCTION(this << " Packet:" << p);

  Ptr<AquaSimNode> sender = (AquaSimNode*)(tifp->Node()), recver = NULL;
  std::vector<Ptr<AquaSimPhy> > rifp = NULL;
  Ptr<Packet> pCopy = NULL;
  Time pDelay = Seconds (0.0);

  /*
  if(!m_sorted){
    SortLists();
  }
  */

  std::vector<PktRecvUnit>* recvUnits = m_prop->ReceivedCopies(sender, p, m_deviceList);

  for(int i=0; i<recvUnits->size(); i++) {
    if (sender == (*recvUnits)[i].recver)
      continue;
    recver = (*recvUnits)[i].recver;
    pDelay = (*recvUnits)[i].pDelay;

    p->m_pStamp->Pr() = (*recvUnits)[i].pR;
    p->m_pStamp->Noise() = m_noiseGen->Noise(Simulator::Now() + pDelay, recver->X(), recver->Y(), recver->Z());
    /**
     * Send to each interface a copy, and we will filter the packet
     * in physical layer according to freq and modulation
     */
    for (std::vector<AquaSimPhy> it = rifp.begin(); it!=rifp.end(); it++) {
      pCopy = p->Copy();
      Simulator::Schedule(pDelay, it, pCopy);
    }
  }
  p = 0; //smart pointer will unref automatically once out of scope
  delete recvUnits;
}

double
AquaSimChannel::GetPropDelay (Ptr<MobilityModel> tnode, Ptr<MobilityModel> rnode)
{
  AquaSimSimplePropagation prop;
  return prop.PDelay(tnode,rnode).GetSeconds();
}
   	
double
AquaSimChannel::Distance(Ptr<MobilityModel> tnode, Ptr<MobilityModel> rnode)
{
  return tnode->GetDistanceFrom(rnode);
}

} // namespace ns3
