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

#include "aqua-sim-routing-dummy.h"
#include "aqua-sim-address.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-header.h"
#include "ns3/log.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimRoutingDummy");
NS_OBJECT_ENSURE_REGISTERED(AquaSimRoutingDummy);


AquaSimRoutingDummy::AquaSimRoutingDummy()
{
}

TypeId
AquaSimRoutingDummy::GetTypeId()
{
  static TypeId tid = TypeId ("ns3::AquaSimRoutingDummy")
    .SetParent<AquaSimRouting> ()
    .AddConstructor<AquaSimRoutingDummy> ()
  ;
  return tid;
}


bool
AquaSimRoutingDummy::Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION(this);

  AquaSimHeader ash;
  AquaSimAddress myAddr = AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress());
  packet->RemoveHeader(ash);

  if (ash.GetNumForwards()==0)  //new packet
  {
    ash.SetDirection(AquaSimHeader::DOWN);
    ash.SetNextHop(AquaSimAddress::GetBroadcast());  // ash.SetNextHop(AquaSimAddress(myAddr.GetAsInt()-1)); //
    ash.SetNumForwards(0);
    ash.SetDAddr(AquaSimAddress::ConvertFrom(dest));
    ash.SetErrorFlag(false);
    ash.SetUId(packet->GetUid());
  }
  else
  {
    //packet->RemoveHeader(ash);
    //if (ash.GetSAddr().GetAsInt() > myAddr.GetAsInt()) return true; //remove backtracking of packets from traffic generator
    packet->AddHeader(ash);
    if (ash.GetDAddr() == myAddr)
    {
      DataForSink(packet);
      return true;
    }

    if (IsDeadLoop(packet))
    {
      NS_LOG_INFO("Deadloop detected. Dropping pkt.");
      return true;
    }
  }

  packet->AddHeader(ash);
  ash.SetSAddr(myAddr);
  ash.SetNumForwards(ash.GetNumForwards() + 1);
  packet->AddHeader(ash);


  MACsend(packet);
  return true;
}

void
AquaSimRoutingDummy::MACsend(Ptr<Packet> pkt, Time delay)
{
  NS_LOG_FUNCTION(this);
  AquaSimHeader ash;
  pkt->PeekHeader(ash);
  Simulator::Schedule(delay, &AquaSimRouting::SendDown,this,
                        pkt,ash.GetNextHop(),Seconds(0));
}

void
AquaSimRoutingDummy::DataForSink(Ptr<Packet> pkt)
{
	//  printf("DataforSink: the packet is send to demux\n");
	NS_LOG_FUNCTION(this << pkt << "Sending up to dmux.");
	if (!SendUp(pkt))
		NS_LOG_WARN("DataForSink: Something went wrong when passing packet up to dmux.");
}

void
AquaSimRoutingDummy::DoDispose()
{
  NS_LOG_FUNCTION(this);
  AquaSimRouting::DoDispose();
}
