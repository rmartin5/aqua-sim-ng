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


#include "aqua-sim-traffic-gen-helper.h"
#include "ns3/aqua-sim-traffic-gen.h"
#include "ns3/packet-socket-address.h"
#include "ns3/string.h"

namespace ns3 {

AquaSimTrafficGenHelper::AquaSimTrafficGenHelper (std::string protocol, Address remote)
{
  m_factory.SetTypeId("ns3::AquaSimTrafficGen");
  m_factory.Set("Protocol", StringValue(protocol));
  m_factory.Set("Remote", AddressValue(remote));
}

void
AquaSimTrafficGenHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set(name,value);
}

ApplicationContainer
AquaSimTrafficGenHelper::Install (NodeContainer nodes) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = nodes.Begin(); i != nodes.End(); ++i){
    apps.Add(InstallPriv(*i));
  }
  return apps;
}

ApplicationContainer
AquaSimTrafficGenHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

Ptr<Application>
AquaSimTrafficGenHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Application>();
  node->AddApplication(app);

  return app;
}

} // namespace ns3
