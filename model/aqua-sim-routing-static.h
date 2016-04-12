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

#ifndef AQUA_SIM_ROUTING_STATIC_H
#define AQUA_SIM_ROUTING_STATIC_H

#include "aqua-sim-routing.h"
#include "aqua-sim-address.h"
#include <map>

namespace ns3 {

/*header length of Static routing*/
#define SR_HDR_LEN (3*sizeof(AquaSimAddress)+sizeof(int))

class AquaSimStaticRouting: public AquaSimRouting {
public:
	AquaSimStaticRouting();
  AquaSimStaticRouting(char *routeFile);
  virtual ~AquaSimStaticRouting();
  static TypeId GetTypeId(void);

	virtual bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);

  void SetRouteTable(char *routeFile);

protected:
	bool m_hasSetRouteFile;
	bool m_hasSetNode;
	char m_routeFile[100];

	void ReadRouteTable(char *filename);
	AquaSimAddress FindNextHop(const Ptr<Packet> p);


private:
	std::map<AquaSimAddress, AquaSimAddress> m_rTable;

};  // class AquaSimStaticRouting

}  //namespace ns3

#endif  /* AQUA_SIM_ROUTING_STATIC_H */
