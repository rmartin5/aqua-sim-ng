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

void
NamedData::Recv(Ptr<Packet> packet)
{
  //TODO XXX
  /*
    Should also separate payload from packet and handle as a string or two strings if data packet.
    Distinush between expected packet types (interest, data, discovery)
    call functions accordingly
    if (hasCache) then try finding interest within cache, and handle from there.
    etc...
  */
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
  //TODO XXX
  //NOTE: check for busy terminal problem.
  //may need a transission notice like in AquaSimMac.
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

uint8_t*
NamedData::GetInterestStr(Ptr<Packet> dataPkt)
{
  uint32_t size = dataPkt->GetSize ();
  uint8_t *data = new uint8_t[size];
  if (dataPkt->CopyData (data, size) == 0)
  {
    NS_LOG_WARN(this << "Packet buffer is empty.");
  }
  return data;
}
