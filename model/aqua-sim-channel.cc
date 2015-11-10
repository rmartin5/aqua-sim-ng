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
#include "aqua-sim-header.h"

#include "ns3/log.h"

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
    //.SetParent<Channel> ()
    //.AddConstructor<AquaSimChannel> ()
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
AquaSimChannel::GetDevice (uint32_t i)
{
  return m_deviceList[i];
}

uint32_t 
AquaSimChannel::GetId (void) const
{
  NS_LOG_WARN("AquaSimChannel::GetId not implemented");
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
    NS_LOG_WARN("AquaSimChannel::RemoveDevice: deviceList is empty");
  else
  {
    std::vector<Ptr<AquaSimNetDevice> >::iterator it = m_deviceList.begin();
    for( int i = 0; i < m_deviceList.size(); ++i)
      {
	++it;
        if(m_deviceList[i] == device)
          {
            m_deviceList.erase(it);
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
  NS_LOG_FUNCTION(this);
  NS_LOG_DEBUG("Packet:" << p);

  Ptr<AquaSimNode> sender = (Ptr<AquaSimNode>)(tifp->Node());
  Ptr<AquaSimNode>recver = NULL;
  std::vector<Ptr<AquaSimPhy> > rifp;	//must support multiple recv phy in future
  Ptr<Packet> pCopy = NULL;
  Time pDelay = Seconds (0.0);

  /*
  if(!m_sorted){
    SortLists();
  }
  */

  std::vector<PktRecvUnit>* recvUnits = m_prop->ReceivedCopies(sender, p, m_deviceList);

  for(std::vector<int>::size_type i=0; i < recvUnits->size(); i++) {
    if (sender == (*recvUnits)[i].recver)
      continue;
    recver = (*recvUnits)[i].recver;
    pDelay = (*recvUnits)[i].pDelay;
    //rifp = recver->ifhead().lh_first;

    AquaSimHeader asHeader;
    p->PeekHeader(asHeader);

    asHeader.SetPr((*recvUnits)[i].pR);
    asHeader.SetNoise(m_noiseGen->Noise(Simulator::Now() + pDelay, recver->X(), recver->Y(), recver->Z()));

    p->AddHeader(asHeader);

    /**
     * Send to each interface a copy, and we will filter the packet
     * in physical layer according to freq and modulation
     */
    pCopy = p->Copy();
    Simulator::Schedule(pDelay, recver, &pCopy);

    /* FIXME in future support multiple phy with below code.
     *
     * for (std::vector<Ptr<AquaSimPhy> >::iterator it = rifp.begin(); it!=rifp.end(); it++) {
     *	 pCopy = p->Copy();
     *   Simulator::Schedule(pDelay, it, &pCopy);
     * }
     *
     */

  }
  pCopy->Unref();
  p->Unref();
  //p = 0; //smart pointer will unref automatically once out of scope
  delete recvUnits;
}

double
AquaSimChannel::GetPropDelay (Ptr<AquaSimNode> tnode, Ptr<AquaSimNode> rnode)
{
  return m_prop->PDelay(tnode, rnode).GetSeconds();
}
   	
double
AquaSimChannel::Distance(Ptr<AquaSimNode> tnode, Ptr<AquaSimNode> rnode)
{
  return tnode->DistanceFrom(rnode);
}

} // namespace ns3
