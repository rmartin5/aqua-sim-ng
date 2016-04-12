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

#include "aqua-sim-routing-static.h"
#include "aqua-sim-header.h"

#include "ns3/string.h"
#include "ns3/log.h"

#include <cstdio>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimStaticRouting");
NS_OBJECT_ENSURE_REGISTERED(AquaSimStaticRouting);

AquaSimStaticRouting::AquaSimStaticRouting() :
		m_hasSetRouteFile(false), m_hasSetNode(false)
{
}

AquaSimStaticRouting::AquaSimStaticRouting(char *routeFile) :
    m_hasSetNode(false)
{
  SetRouteTable(routeFile);
}

AquaSimStaticRouting::~AquaSimStaticRouting()
{
}

TypeId
AquaSimStaticRouting::GetTypeId()
{
  static TypeId tid = TypeId ("ns3::AquaSimStaticRouting")
    .SetParent<AquaSimRouting> ()
    .AddConstructor<AquaSimStaticRouting> ()
  ;
  return tid;
}

void
AquaSimStaticRouting::SetRouteTable(char *routeFile)
{
  m_hasSetRouteFile = false;
  strcpy(m_routeFile,routeFile);
  //if( m_hasSetNode ) {
    ReadRouteTable(m_routeFile);
  //}
}

/*
 * Load the static routing table in filename
 *
 * @param filename   the file containing routing table
 * */
void
AquaSimStaticRouting::ReadRouteTable(char *filename)
{
  NS_LOG_FUNCTION(this);

	FILE* stream = fopen(filename, "r");
	int current_node, dst_node, nxt_hop;

	if( stream == NULL ) {
		printf("ERROR: Cannot find routing table file!\nEXIT...\n");
		exit(0);
	}

	while( !feof(stream) ) {
		fscanf(stream, "%d:%d:%d", &current_node, &dst_node, &nxt_hop);

		if( m_myAddr == AquaSimAddress(current_node) ) {
			m_rTable[AquaSimAddress(dst_node)] = AquaSimAddress(nxt_hop);
		}
	}

	fclose(stream);
}


bool
AquaSimStaticRouting::Recv (Ptr<Packet> p, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION(this << p);
  AquaSimHeader ash;
  //struct hdr_ip* ih = HDR_IP(p);
  p->PeekHeader (ash);

  if (IsDeadLoop (p))
    {
      NS_LOG_INFO("Dropping packet " << p << " due to route loop");
      //drop(p, DROP_RTR_ROUTE_LOOP);
      p = 0;
      return false;
    }
  else if (AmISrc (p))
    {
      ash.SetSize(ash.GetSize() + SR_HDR_LEN); //add the overhead of static routing's header
    }
  else if (!AmINextHop (p))
    {
      NS_LOG_INFO("Dropping packet " << p << " due to duplicate");
      //drop(p, DROP_MAC_DUPLICATE);
      p = 0;
      return false;
    }

  //increase the number of forwards
  p->RemoveHeader (ash);
  uint8_t numForwards = ash.GetNumForwards () + 1;
  ash.SetNumForwards (numForwards);
  p->AddHeader (ash);

  if (AmIDst (p))
    {
      NS_LOG_INFO("I am destination. Sending up.");
      SendUp (p);
      return true;
    }

  //find the next hop and forward
  AquaSimAddress next_hop = FindNextHop (p);
  if (next_hop != AquaSimAddress::GetBroadcast ())
    {
      SendDown (p, next_hop, Seconds (0.0));
      return true;
    }
  else
    {
      //fail to find the route, drop it
      NS_LOG_INFO("Dropping packet " << p << " due to no route");
      //drop(p, DROP_RTR_NO_ROUTE);
      p = 0;
    }
  return false;
}

/*
 * @param p   a packet
 * @return    the next hop to route packet p
 * */
AquaSimAddress
AquaSimStaticRouting::FindNextHop(const Ptr<Packet> p)
{
  AquaSimHeader ash;
  p->PeekHeader(ash);
  std::map<AquaSimAddress, AquaSimAddress>::iterator it = m_rTable.find(ash.GetDAddr());
  return it == m_rTable.end() ? AquaSimAddress::GetBroadcast() : it->second;
}


}  // namespace ns3
