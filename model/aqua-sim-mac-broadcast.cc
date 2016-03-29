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

#include "aqua-sim-mac-broadcast.h"
#include "aqua-sim-header.h"
#include "aqua-sim-address.h"

#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/simulator.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimBroadcastMac");
NS_OBJECT_ENSURE_REGISTERED(AquaSimBroadcastMac);


/* ======================================================================
Broadcast MAC for  underwater sensor
====================================================================== */

AquaSimBroadcastMac::AquaSimBroadcastMac()
{
  m_backoffCounter=0;
}

TypeId
AquaSimBroadcastMac::GetTypeId()
{
  static TypeId tid = TypeId("ns3::AquaSimBroadcastMac")
      .SetParent<AquaSimMac>()
      .AddConstructor<AquaSimBroadcastMac>()
      .AddAttribute("PacketHeaderSize", "Size of packet header",
        IntegerValue(0),
        MakeIntegerAccessor (&AquaSimBroadcastMac::m_packetHeaderSize),
        MakeIntegerChecker<int> ())
      .AddAttribute("PacketSize", "Size of packet",
	IntegerValue(0),
	MakeIntegerAccessor (&AquaSimBroadcastMac::m_packetSize),
	MakeIntegerChecker<int> ())
    ;
  return tid;
}
/*
this program is used to handle the received packet,
it should be virtual function, different class may have
different versions.
*/

bool
AquaSimBroadcastMac::RecvProcess (Ptr<Packet> pkt)
{
  AquaSimHeader ash;
  pkt->PeekHeader(ash);
  AquaSimAddress dst = ash.GetDAddr();

  //get a packet from modem, remove the sync hdr from txtime first
  //cmh->txtime() -= getSyncHdrLen();

  if (ash.GetErrorFlag())
    {
      NS_LOG_DEBUG("BroadcastMac:RecvProcess: received corrupt packet.");
      pkt=0;
      return false;
    }

  if (dst == AquaSimAddress::GetBroadcast() || dst == AquaSimAddress::ConvertFrom(m_device->GetAddress()))
    {
      if (m_packetSize == 0)
	{
	  NS_LOG_INFO("Should be changing header size here.");
    ash.SetSize(ash.GetSize() - m_packetHeaderSize);
	}
      return SendUp(pkt);
    }

//	printf("underwaterAquaSimBroadcastMac: this is neither broadcast nor my packet, just drop it\n");
  pkt=0;
  return false;
}


void
AquaSimBroadcastMac::DropPacket(Ptr<Packet> pkt)
{
  //this is not necessary... only kept for current legacy issues
  pkt=0;
  return;
}


/*
this program is used to handle the transmitted packet,
it should be virtual function, different class may have
different versions.
*/
bool
AquaSimBroadcastMac::TxProcess(Ptr<Packet> pkt)
{
  AquaSimHeader ash;
  pkt->RemoveHeader(ash);

  ash.SetDAddr(AquaSimAddress::GetBroadcast());
  ash.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));

  if( m_packetSize != 0 )
    ash.SetSize(m_packetSize);
  else
    ash.SetSize(m_packetHeaderSize + ash.GetSize());

  ash.SetTxTime(GetTxTime(pkt));

  switch( m_device->TransmissionStatus() )
  {
  case SLEEP:
      PowerOn();
      break;
  case NIDLE:
      m_device->SetTransmissionStatus(SEND);
      ash.SetDirection(AquaSimHeader::DOWN);
      //ash->addr_type()=NS_AF_ILINK;
      //add the sync hdr
      pkt->AddHeader(ash);
      SendDown(pkt);
      m_backoffCounter=0;
      Simulator::Schedule(ash.GetTxTime(),&AquaSimBroadcastMac::StatusProcess,this);
      return true;
  case RECV:
    {
      Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
      double backoff=rand->GetValue()*BC_BACKOFF;
      pkt->AddHeader(ash);
      Simulator::Schedule(Seconds(backoff),&AquaSimBroadcastMac::BackoffHandler,this,pkt);
    }
      return true;
  case SEND:
      pkt=0;
      return false;
  default:
      /*
      * all cases have been processed above, so simply return
      */
    break;
  }
  return true;
}


void
AquaSimBroadcastMac::StatusProcess()
{
  if(SLEEP==m_device->TransmissionStatus())
    {
      //s.schedule(&callback_handler,&callback_event,BC_CALLBACK_DELAY);
      return;
    }
  m_device->SetTransmissionStatus(NIDLE);
  //s.schedule(&callback_handler,&callback_event,BC_CALLBACK_DELAY);
  return;
}

void
AquaSimBroadcastMac::BackoffHandler(Ptr<Packet> pkt)
{
  m_backoffCounter++;
  if (m_backoffCounter<BC_MAXIMUMCOUNTER)
    TxProcess(pkt);
  else
    {
      NS_LOG_INFO("BackoffHandler: too many backoffs");
      m_backoffCounter=0;
      DropPacket(pkt);
    }
}
