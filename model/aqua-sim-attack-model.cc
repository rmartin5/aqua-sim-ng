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

#include "aqua-sim-attack-model.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-mac.h"

#include "ns3/log.h"
#include "ns3/vector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "ns3/simulator.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimAttackModel");
NS_OBJECT_ENSURE_REGISTERED (AquaSimAttackModel);
NS_OBJECT_ENSURE_REGISTERED (AquaSimAttackDos);
NS_OBJECT_ENSURE_REGISTERED (AquaSimAttackSinkhole);
NS_OBJECT_ENSURE_REGISTERED (AquaSimAttackSelective);
//NS_OBJECT_ENSURE_REGISTERED (AquaSimAttacSybil);

TypeId
AquaSimAttackModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AquaSimAttackModel")
    .SetParent<Object> ()
    ;
  return tid;
}

void
AquaSimAttackModel::SetDevice(Ptr<AquaSimNetDevice> device)
{
  NS_LOG_FUNCTION(this);
  m_device = device;
}

void
AquaSimAttackModel::SendDown(Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this);
  if(!m_device->GetMac()->SendDown(p))
    NS_LOG_DEBUG("Something went wrong when sending down to MAC");
}

/*
 *  Aqua Sim Attack DoS
 */
AquaSimAttackDos::AquaSimAttackDos() :
  m_sendFreq(10), m_packetSize(40),
  m_dest(AquaSimAddress::GetBroadcast())
{
  NS_LOG_FUNCTION(this);
  Simulator::Schedule(Seconds(m_sendFreq), &AquaSimAttackDos::SendPacket, this);
}

TypeId
AquaSimAttackDos::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AquaSimAttackDos")
    .SetParent<AquaSimAttackModel> ()
    .AddConstructor<AquaSimAttackDos> ()
    .AddAttribute ("SendFreq", "Frequency at which DoS packets are created and sent",
      DoubleValue(10),
      MakeDoubleAccessor (&AquaSimAttackDos::m_sendFreq),
      MakeDoubleChecker<double> ())
    .AddAttribute ("PacketSize", "Size of created packet's payload",
      IntegerValue(40),
      MakeIntegerAccessor (&AquaSimAttackDos::m_packetSize),
      MakeIntegerChecker<int> ())
    ;
  return tid;
}

void
AquaSimAttackDos::Recv(Ptr<Packet> p)
{
  NS_LOG_INFO("AttackDoS::Recv will ignore packet recv.");
}

/*
 * NOTE: Added headers should be updated dependent on the protocols currently in use.
 */
Ptr<Packet>
AquaSimAttackDos::CreatePkt()
{
  Ptr<Packet> pkt = Create<Packet>(m_packetSize);
  MacHeader mach;
  AquaSimHeader ash;

  mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  mach.SetDA(m_dest);
  //default mach DemuxPType == UWPTYPE_OTHER
  ash.SetTxTime(m_device->GetMac()->GetTxTime(ash.GetSize()) );
  ash.SetDirection(AquaSimHeader::DOWN);
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  ash.SetDAddr(m_dest);
  ash.SetTimeStamp(Simulator::Now());

  pkt->AddHeader(mach);
  pkt->AddHeader(ash);

  return pkt;
}

void
AquaSimAttackDos::SetPacketSize(int packetSize)
{
  m_packetSize = packetSize;
}

void
AquaSimAttackDos::SetSendFrequency(double sendFreq)
{
  m_sendFreq = sendFreq;
}

void
AquaSimAttackDos::SetDestAddress(AquaSimAddress dest)
{
  m_dest = dest;
}

void
AquaSimAttackDos::SendPacket()
{
  SendDown(CreatePkt());
  Simulator::Schedule(Seconds(m_sendFreq), &AquaSimAttackDos::SendPacket, this);
}


/*
 *  Aqua Sim Attack Sinkhole
 */
AquaSimAttackSinkhole::AquaSimAttackSinkhole() :
  m_dataRate(135), m_energy(50),
  m_depth(0), m_dropFrequency(1.0),
  m_pktDropped(0), m_totalPktRecv(0)
{
  NS_LOG_FUNCTION(this);
}

TypeId
AquaSimAttackSinkhole::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AquaSimAttackSinkhole")
    .SetParent<AquaSimAttackModel> ()
    .AddConstructor<AquaSimAttackSinkhole> ()
    .AddAttribute ("DataRate", "False data rate advertised by attacker",
      DoubleValue(50),
      MakeDoubleAccessor (&AquaSimAttackSinkhole::m_dataRate),
      MakeDoubleChecker<double> ())
    .AddAttribute ("Energy", "False energy advertised by attacker",
      DoubleValue(50),
      MakeDoubleAccessor (&AquaSimAttackSinkhole::m_energy),
      MakeDoubleChecker<double> ())
    .AddAttribute ("Depth", "False depth advertised by attacker",
      DoubleValue(0),
      MakeDoubleAccessor (&AquaSimAttackSinkhole::m_depth),
      MakeDoubleChecker<double> ())
    .AddAttribute ("DropFreq", "Drop frequency of received packets (between 0 and 1)",
      DoubleValue(1.0),
      MakeDoubleAccessor (&AquaSimAttackSinkhole::m_pktDropped),
      MakeDoubleChecker<double> ())
    ;
  return tid;
}

void
AquaSimAttackSinkhole::Recv(Ptr<Packet> p)
{
  if ((m_pktDropped/++m_totalPktRecv) <= m_dropFrequency)
  {
    m_pktDropped++;
    return; //drop packet
  }
  else
  {
    m_device->GetMac()->RecvProcess(p);
  }
}

/*
 *  NOTE: This should be edited to match expected packet types
 */
Ptr<Packet>
AquaSimAttackSinkhole::CreatePkt()
{
  Ptr<Packet> pkt = Create<Packet>();
  MacHeader mach;
  AquaSimHeader ash;

  mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  mach.SetDA(AquaSimAddress::GetBroadcast());

  ash.SetTxTime(m_device->GetMac()->GetTxTime(ash.GetSize()) );
  ash.SetDirection(AquaSimHeader::DOWN);
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
  ash.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()) );
  ash.SetDAddr(AquaSimAddress::GetBroadcast());

  //Should include advertised rates: i.e. data rate / energy / depth

  pkt->AddHeader(mach);
  pkt->AddHeader(ash);

  return pkt;
}

void
AquaSimAttackSinkhole::SendAdvertisePacket()
{
  SendDown(CreatePkt());
}

void
AquaSimAttackSinkhole::SetDataRate(double rate)
{
  m_dataRate = rate;
}

void
AquaSimAttackSinkhole::SetEnergy(double energy)
{
  m_energy = energy;
}

void
AquaSimAttackSinkhole::SetDepth(double depth)
{
  m_depth = depth;
}

void
AquaSimAttackSinkhole::SetDropFrequency(double drop)
{
  m_dropFrequency = drop;
}


/*
 *  Aqua Sim Attack Selective Forward
 */
AquaSimAttackSelective::AquaSimAttackSelective() :
  m_blockSender(-1), m_dropFrequency(0.0),
  m_pktDropped(0), m_totalPktRecv(0)
{
  NS_LOG_FUNCTION(this);
}

TypeId
AquaSimAttackSelective::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AquaSimAttackSelective")
    .SetParent<AquaSimAttackModel> ()
    .AddConstructor<AquaSimAttackSelective> ()
    .AddAttribute ("BlockSender", "Block a specific sender. Will ignore all packets from given node",
      IntegerValue(-1),
      MakeIntegerAccessor (&AquaSimAttackSelective::m_blockSender),
      MakeIntegerChecker<int> ())
    .AddAttribute ("DropFreq", "Drop frequency of received packets (between 0 and 1)",
      DoubleValue(0.0),
      MakeDoubleAccessor (&AquaSimAttackSelective::m_pktDropped),
      MakeDoubleChecker<double> ())
    ;
  return tid;
}

void
AquaSimAttackSelective::Recv(Ptr<Packet> p)
{
  AquaSimHeader ash;
  p->PeekHeader(ash);
  if (m_blockSender == ash.GetSAddr().GetAsInt() ||
      (m_pktDropped/++m_totalPktRecv) <= m_dropFrequency)
  {
    m_pktDropped++;
    return; //drop packet
  }
  else
  {
    m_device->GetMac()->RecvProcess(p);
  }
}

void
AquaSimAttackSelective::SetDropFrequency(double drop)
{
  m_dropFrequency = drop;
}

void
AquaSimAttackSelective::BlockNode(int sender)
{
  m_blockSender = sender;
}

void
AquaSimAttackSelective::BlockNode(AquaSimAddress sender)
{
  m_blockSender = sender.GetAsInt();
}
