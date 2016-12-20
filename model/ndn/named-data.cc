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

#include "named-data.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/aqua-sim-address.h"
#include "ns3/aqua-sim-header.h"
#include "ns3/aqua-sim-header-mac.h"
#include "named-data-header.h"
#include "name-discovery.h"
#include <string.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NamedData");
NS_OBJECT_ENSURE_REGISTERED (NamedData);

TypeId
NamedData::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NamedData")
    .SetParent<Object> ()
    .AddConstructor<NamedData> ()
    .AddAttribute ("HasCache", "If device has content storage cache. Default is false.",
      BooleanValue(false),
      MakeBooleanAccessor (&NamedData::m_hasCache),
      MakeBooleanChecker ())
    ;
  return tid;
}

NamedData::NamedData() : m_hasCache(false)
{
}

void
NamedData::SetFib(Ptr<Fib> fib)
{
  NS_LOG_FUNCTION(this);
  NS_ASSERT(fib);
  m_fib = fib;
}

void
NamedData::SetPit(Ptr<Pit> pit)
{
  NS_LOG_FUNCTION(this);
  NS_ASSERT(pit);
  m_pit = pit;
}

void
NamedData::SetContentStorage(Ptr<ContentStorage> cs)
{
  NS_LOG_FUNCTION(this);
  NS_ASSERT(cs);
  m_cs = cs;
  if (m_cs->GetCacheType()!=NO_CACHE) m_hasCache=true;
}

void
NamedData::SetNetDevice(Ptr<AquaSimNetDevice> device)
{
  NS_LOG_FUNCTION(this);
  NS_ASSERT(device);
  m_device = device;
}

bool
NamedData::Recv(Ptr<Packet> packet)
{
  NS_LOG_FUNCTION(this);

  AquaSimHeader ash;
  MacHeader mach;
  NamedDataHeader ndh;
  packet->RemoveHeader(ash);
  packet->RemoveHeader(mach);
  packet->PeekHeader(ndh);
  ash.SetNumForwards(ash.GetNumForwards()+1);
  packet->AddHeader(mach);
  packet->AddHeader(ash);

  if (ash.GetSAddr()==AquaSimAddress::ConvertFrom(m_device->GetAddress()) &&
        ash.GetNumForwards() > 1) {
    NS_LOG_DEBUG(this << "Loop detected, dropping packet.");
    return false;
  }

  if (!RecvCheck(packet,ndh.GetPType())) {
    return false;
  }

  switch (ndh.GetPType()) {
    case (NamedDataHeader::NDN_INTEREST):
    {
      NS_LOG_INFO("Interest Packet Recv");
      uint8_t* interest = GetInterestPktStr(packet);
      if (m_hasCache) {
        uint8_t* potentialData = m_cs->GetEntry(interest);
        if (potentialData != NULL) {
          NS_LOG_INFO(this << "Found corresponding data to satisfy interest.");
          SendPkt(CreateData(interest,potentialData,strlen((char*)interest),strlen((char*)potentialData)));
          return true;
        }
      }
      std::list<AquaSimAddress> addressList = m_fib->InterestRecv(interest);
      if (!addressList.empty()) {
        if (m_pit->AddEntry(interest, ash.GetSAddr())) {
          SendMultiplePackets(packet, addressList);
        }
      }
      else {
        NS_LOG_INFO(this << " No known FIB paths for " << interest);
        return false;
      }
    }
    break;
    case (NamedDataHeader::NDN_DATA):
    {
      NS_LOG_INFO("Data Packet Recv");
      std::pair<uint8_t*,uint8_t*> payload = GetInterestAndDataStr(packet);
          //payload.first == interest, payload.second == data
      std::list<AquaSimAddress> addressList = m_pit->GetEntry(payload.first);
      if (!addressList.empty()) {
        if (m_hasCache) m_cs->AddEntry(payload.first, payload.second);
        SendMultiplePackets(packet, addressList);
        m_pit->RemoveEntry(payload.first);
      }
      else {
        NS_LOG_INFO(this << "No corresponding PIT entries for given data pkt.");
        return false;
      }
    }
    break;
    case (NamedDataHeader::NDN_DISCOVERY):
    {
      NS_LOG_INFO("Discovery Packet Recv");
      NameDiscovery nameDiscovery;
      std::pair<uint8_t*,AquaSimAddress> discovery = nameDiscovery.ProcessNameDiscovery(packet);
      nameDiscovery.ShortenNamePrefix(discovery.first, '/');
      m_fib->AddEntry(discovery.first, discovery.second);
    }
    break;
    default:
      NS_LOG_DEBUG(this << "Incompatible ND header packet type. Dropping packet.");
      return false;
  }

  return true;
}

Ptr<Packet>
NamedData::CreateInterest(uint8_t* name, uint32_t nameSize)
{
  NS_LOG_DEBUG(this << name);

  Ptr<Packet> pkt = Create<Packet>(name, nameSize);

  AquaSimHeader ash;
  MacHeader mach;
  NamedDataHeader ndh;
  mach.SetDemuxPType(MacHeader::UWPTYPE_NDN);
  ash.SetDirection(AquaSimHeader::DOWN);
  ash.SetErrorFlag(false);
  ash.SetTxTime(m_device->GetMac()->GetTxTime(pkt));
  ndh.SetPType(NamedDataHeader::NDN_INTEREST);

  pkt->AddHeader(ndh);
  pkt->AddHeader(mach);
  pkt->AddHeader(ash);
  return pkt;
}

Ptr<Packet>
NamedData::CreateData(uint8_t* name, uint8_t* data, uint32_t nameSize, uint32_t dataSize)
{
  NS_LOG_DEBUG(this << name);

  Ptr<Packet> pkt = Create<Packet>(name,nameSize);
  pkt->AddAtEnd(Create<Packet>((uint8_t*)DELIMITER,sizeof(DELIMITER)));
  pkt->AddAtEnd(Create<Packet>(data,dataSize));

  AquaSimHeader ash;
  MacHeader mach;
  NamedDataHeader ndh;
  mach.SetDemuxPType(MacHeader::UWPTYPE_NDN);
  ash.SetDirection(AquaSimHeader::DOWN);
  ash.SetErrorFlag(false);
  ash.SetTxTime(m_device->GetMac()->GetTxTime(pkt));
  ndh.SetPType(NamedDataHeader::NDN_DATA);

  pkt->AddHeader(ndh);
  pkt->AddHeader(mach);
  pkt->AddHeader(ash);
  return pkt;
}

Ptr<Packet>
NamedData::CreateNameDiscovery(uint8_t* name, uint32_t nameSize)
{
  NS_LOG_DEBUG(this << name);

  Ptr<Packet> pkt = Create<Packet>(name, nameSize);

  AquaSimHeader ash;
  MacHeader mach;
  NamedDataHeader ndh;
  mach.SetDemuxPType(MacHeader::UWPTYPE_NDN);
  ash.SetDirection(AquaSimHeader::DOWN);
  ash.SetErrorFlag(false);
  ash.SetTxTime(m_device->GetMac()->GetTxTime(pkt));
  ash.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
  ndh.SetPType(NamedDataHeader::NDN_DISCOVERY);

  pkt->AddHeader(ndh);
  pkt->AddHeader(mach);
  pkt->AddHeader(ash);
  return pkt;
}

void
NamedData::SendPkt(Ptr<Packet> packet)
{
  //Using MAC layer for busy terminal problem solution
  if (!m_device->GetMac()->SendDown(packet)){
    NS_LOG_DEBUG(this << "Something went wrong when sending packet. Is device sleeping?");
  }
}

uint8_t*
NamedData::GetDataStr(Ptr<Packet> dataPkt)
{
  uint32_t size = dataPkt->GetSize ();
  uint8_t *data = new uint8_t[size];
  dataPkt->CopyData (data, size);

  /*
   *  NOTE: If more than one DELIMITER is within packet buffer, then unpredicted results may occur.
   */
  char *token = strtok(reinterpret_cast<char*>(data),DELIMITER);
  token = strtok(NULL,DELIMITER); //Ignore interest
  if (token == NULL)
  {
    NS_LOG_WARN(this << "Cannot split payload:" << data << " with delimiter:" << DELIMITER << ". Returning NULL");
    return NULL;
  }
  return (uint8_t*)token;
}

std::pair<uint8_t*,uint8_t*>
NamedData::GetInterestAndDataStr(Ptr<Packet> dataPkt)
{
  uint32_t size = dataPkt->GetSize ();
  uint8_t *data = new uint8_t[size];
  dataPkt->CopyData (data, size);

  /*
   *  NOTE: If more than one DELIMITER is within packet buffer, then unpredicted results may occur.
   */
  char *token = strtok(reinterpret_cast<char*>(data),DELIMITER);
  char* interest = token;
  token = strtok(NULL,DELIMITER); //Ignore interest
  if (token == NULL)
  {
    NS_LOG_WARN(this << "Cannot split payload:" << data << " with delimiter:" << DELIMITER << ". Returning NULL");
    return std::make_pair((uint8_t*)NULL,(uint8_t*)NULL);
  }
  return std::make_pair((uint8_t*)interest,(uint8_t*)token);
}

uint8_t*
NamedData::GetInterestPktStr(Ptr<Packet> intPkt)
{
  uint32_t size = intPkt->GetSize ();
  uint8_t *data = new uint8_t[size];
  if (intPkt->CopyData (data, size) == 0)
  {
    NS_LOG_WARN(this << "Packet buffer is empty.");
  }
  return data;
}

void
NamedData::SendMultiplePackets(Ptr<Packet> packet, std::list<AquaSimAddress> addresses)
{
  AquaSimHeader ash;

  while (!addresses.empty()) {
      packet->RemoveHeader(ash);
      ash.SetDAddr(addresses.front());
      packet->AddHeader(ash);
      SendPkt(packet);
      addresses.pop_front();
  }
}

/*
 *  Assist in Pit/Fib targeted packet sending and multicasting. Ensure only targeted nodes recv packet.
 *
 *  Return true if should recv packet, false otherwise.
 */
bool
NamedData::RecvCheck(Ptr<Packet> packet, uint8_t ptype)
{
  AquaSimHeader ash;
  packet->PeekHeader(ash);
  return (ash.GetDAddr()==AquaSimAddress::ConvertFrom(m_device->GetAddress()) ||
            ash.GetDAddr()==AquaSimAddress::GetBroadcast() ||
            ptype==NamedDataHeader::NDN_DISCOVERY );
}
