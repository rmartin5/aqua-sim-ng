/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Connecticut
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

#include "aqua-sim-traffic-gen.h"
#include "ns3/inet-socket-address.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet.h"
#include "ns3/address.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimTrafficGen");
NS_OBJECT_ENSURE_REGISTERED (AquaSimTrafficGen);

TypeId
AquaSimTrafficGen::GetTypeId()
{
  static TypeId tid = TypeId("ns3::AquaSimTrafficGen")
    .SetParent<Application>()
    .AddConstructor<AquaSimTrafficGen>()
    .AddAttribute ("Delay", "The delay interval between sending packets (seconds)",
                    UintegerValue (10),
                    MakeUintegerAccessor (&AquaSimTrafficGen::m_delayInt),
                    MakeUintegerChecker<uint32_t>())
    .AddAttribute ("PacketSize", "Size of packets sent",
                    UintegerValue (300),
                    MakeUintegerAccessor (&AquaSimTrafficGen::m_pktSize),
                    MakeUintegerChecker<uint32_t>())
    .AddAttribute ("Protocol", "The type of protocol to use.",
                    TypeIdValue (UdpSocketFactory::GetTypeId ()),
                    MakeTypeIdAccessor (&AquaSimTrafficGen::m_tid),
                    MakeTypeIdChecker ())
    .AddAttribute ("Remote", "The address of the destination.",
                    AddressValue(),
                    MakeAddressAccessor (&AquaSimTrafficGen::m_peer),
                    MakeAddressChecker ())
  ;
  return tid;
}

AquaSimTrafficGen::AquaSimTrafficGen ()
 : m_socket(0)
{
  NS_LOG_FUNCTION(this);
}
AquaSimTrafficGen::~AquaSimTrafficGen()
{
}

void
AquaSimTrafficGen::SetDelay (uint32_t delay)
{
  m_delayInt = delay;
}

void
AquaSimTrafficGen::SetSize (uint32_t size)
{
  m_pktSize = size;
}

void
AquaSimTrafficGen::DoDipsose()
{
  NS_LOG_FUNCTION(this);
  m_socket=0;
  Application::DoDispose();
}

void
AquaSimTrafficGen::StartApplication()
{
  NS_LOG_FUNCTION(this);
  if (!m_socket) {
    m_socket = Socket::CreateSocket (GetNode(), m_tid);
    if (Inet6SocketAddress::IsMatchingType (m_peer)) {
      if (m_socket->Bind6 () == -1)
      {
        NS_FATAL_ERROR ("Failed to bind socket");
      }
    }
    else if (InetSocketAddress::IsMatchingType (m_peer) || PacketSocketAddress::IsMatchingType (m_peer)) {
      if (m_socket->Bind () == -1)
      {
        NS_FATAL_ERROR ("Failed to bind socket");
      }
    }

    m_socket->Connect (m_peer);
    m_socket->SetAllowBroadcast (true);
    m_socket->ShutdownRecv ();
  }
  CancelEvents();
  m_sendEvent = Simulator::Schedule(Seconds(m_delayInt),&AquaSimTrafficGen::DoGenerate, this);
}

void
AquaSimTrafficGen::StopApplication()
{
  NS_LOG_FUNCTION(this);
  if (m_socket != 0) {
    m_socket->Close();
  }
  else {
    NS_LOG_WARN ("AquaSimTrafficGen::StopApplication found null socket");
  }
}

void
AquaSimTrafficGen::DoGenerate()
{
  NS_LOG_FUNCTION(this);
  m_sendEvent = Simulator::Schedule(Seconds(m_delayInt),&AquaSimTrafficGen::DoGenerate,this);
  SendPacket();
}

void
AquaSimTrafficGen::SendPacket()
{
  Ptr<Packet> packet = Create<Packet> (m_pktSize);
  m_socket->Send(packet);
}

void
AquaSimTrafficGen::CancelEvents()
{
  NS_LOG_FUNCTION(this);
  Simulator::Cancel(m_sendEvent);
}

} // namespace ns3
