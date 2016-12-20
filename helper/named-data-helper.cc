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


#include "ns3/log.h"
#include "ns3/assert.h"

#include "ns3/aqua-sim-net-device.h"
#include "ns3/aqua-sim-address.h"
#include "ns3/application.h"

#include "named-data-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NamedDataHelper");

NamedDataHelper::NamedDataHelper()
{
  m_channel.clear();
  /*
   * Protocol prefix setting
   */
  m_phy.SetTypeId("ns3::AquaSimPhyCmn");
  m_phy.Set("CPThresh", DoubleValue(10));
  m_phy.Set("CSThresh", DoubleValue(0));
  m_phy.Set("RXThresh", DoubleValue(0));
  m_phy.Set("PT", DoubleValue(0.2818));
  m_phy.Set("Frequency", DoubleValue(25));
  m_phy.Set("K", DoubleValue(2.0));
  m_mac.SetTypeId("ns3::AquaSimBroadcastMac");  //does not matter. only using for busy terminal problem fix.
  m_energyM.SetTypeId("ns3::AquaSimEnergyModel");
  m_sync.SetTypeId("ns3::AquaSimSync");
  m_localization.SetTypeId("ns3::AquaSimRBLocalization");
  m_attacker = false;

  m_nd.SetTypeId("ns3::NamedData");
  m_pit.SetTypeId("ns3::Pit");
  m_fib.SetTypeId("ns3::Fib");
  m_cs.SetTypeId("ns3::CSFifo");
}

void
NamedDataHelper::SetChannel(Ptr<AquaSimChannel> channel)
{
  NS_ASSERT_MSG(channel, "provided channel pointer is null");
  m_channel.push_back(channel);
}

Ptr<AquaSimChannel>
NamedDataHelper::GetChannel(int channelId)
{
  return m_channel.at(channelId);
}

void
NamedDataHelper::SetAttacker(bool attacker)
{
  m_attacker = attacker;
}

void
NamedDataHelper::SetPhy (std::string type,
                       std::string n0, const AttributeValue &v0,
                       std::string n1, const AttributeValue &v1,
                       std::string n2, const AttributeValue &v2,
                       std::string n3, const AttributeValue &v3,
                       std::string n4, const AttributeValue &v4,
                       std::string n5, const AttributeValue &v5,
                       std::string n6, const AttributeValue &v6,
                       std::string n7, const AttributeValue &v7)
{
  ObjectFactory factory;
  factory.SetTypeId (type);
  factory.Set (n0,v0);
  factory.Set (n1,v1);
  factory.Set (n2,v2);
  factory.Set (n3,v3);
  factory.Set (n4,v4);
  factory.Set (n5,v5);
  factory.Set (n6,v6);
  factory.Set (n7,v7);
  m_phy = factory;
}

void
NamedDataHelper::SetEnergyModel (std::string type,
                                              std::string n0, const AttributeValue &v0,
                                              std::string n1, const AttributeValue &v1,
                                              std::string n2, const AttributeValue &v2,
                                              std::string n3, const AttributeValue &v3,
                                              std::string n4, const AttributeValue &v4,
                                              std::string n5, const AttributeValue &v5,
                                              std::string n6, const AttributeValue &v6,
                                              std::string n7, const AttributeValue &v7)
{
  ObjectFactory factory;
  factory.SetTypeId (type);
  factory.Set (n0,v0);
  factory.Set (n1,v1);
  factory.Set (n2,v2);
  factory.Set (n3,v3);
  factory.Set (n4,v4);
  factory.Set (n5,v5);
  factory.Set (n6,v6);
  factory.Set (n7,v7);
  m_energyM = factory;
}

void
NamedDataHelper::SetSync (std::string type,
                                              std::string n0, const AttributeValue &v0,
                                              std::string n1, const AttributeValue &v1,
                                              std::string n2, const AttributeValue &v2,
                                              std::string n3, const AttributeValue &v3,
                                              std::string n4, const AttributeValue &v4,
                                              std::string n5, const AttributeValue &v5,
                                              std::string n6, const AttributeValue &v6,
                                              std::string n7, const AttributeValue &v7)
{
  ObjectFactory factory;
  factory.SetTypeId (type);
  factory.Set (n0,v0);
  factory.Set (n1,v1);
  factory.Set (n2,v2);
  factory.Set (n3,v3);
  factory.Set (n4,v4);
  factory.Set (n5,v5);
  factory.Set (n6,v6);
  factory.Set (n7,v7);
  m_sync = factory;
}

void
NamedDataHelper::SetLocalization (std::string type,
                                              std::string n0, const AttributeValue &v0,
                                              std::string n1, const AttributeValue &v1,
                                              std::string n2, const AttributeValue &v2,
                                              std::string n3, const AttributeValue &v3,
                                              std::string n4, const AttributeValue &v4,
                                              std::string n5, const AttributeValue &v5,
                                              std::string n6, const AttributeValue &v6,
                                              std::string n7, const AttributeValue &v7)
{
  ObjectFactory factory;
  factory.SetTypeId (type);
  factory.Set (n0,v0);
  factory.Set (n1,v1);
  factory.Set (n2,v2);
  factory.Set (n3,v3);
  factory.Set (n4,v4);
  factory.Set (n5,v5);
  factory.Set (n6,v6);
  factory.Set (n7,v7);
  m_localization = factory;
}

void
NamedDataHelper::SetAttackModel (std::string type,
                                              std::string n0, const AttributeValue &v0,
                                              std::string n1, const AttributeValue &v1,
                                              std::string n2, const AttributeValue &v2,
                                              std::string n3, const AttributeValue &v3,
                                              std::string n4, const AttributeValue &v4,
                                              std::string n5, const AttributeValue &v5,
                                              std::string n6, const AttributeValue &v6,
                                              std::string n7, const AttributeValue &v7)
{
  ObjectFactory factory;
  factory.SetTypeId (type);
  factory.Set (n0,v0);
  factory.Set (n1,v1);
  factory.Set (n2,v2);
  factory.Set (n3,v3);
  factory.Set (n4,v4);
  factory.Set (n5,v5);
  factory.Set (n6,v6);
  factory.Set (n7,v7);
  m_attackM = factory;
}
void
NamedDataHelper::SetNamedData (std::string type,
                                              std::string n0, const AttributeValue &v0,
                                              std::string n1, const AttributeValue &v1,
                                              std::string n2, const AttributeValue &v2,
                                              std::string n3, const AttributeValue &v3,
                                              std::string n4, const AttributeValue &v4,
                                              std::string n5, const AttributeValue &v5,
                                              std::string n6, const AttributeValue &v6,
                                              std::string n7, const AttributeValue &v7)
{
  ObjectFactory factory;
  factory.SetTypeId (type);
  factory.Set (n0,v0);
  factory.Set (n1,v1);
  factory.Set (n2,v2);
  factory.Set (n3,v3);
  factory.Set (n4,v4);
  factory.Set (n5,v5);
  factory.Set (n6,v6);
  factory.Set (n7,v7);
  m_nd = factory;
}

void
NamedDataHelper::SetPit (std::string type,
                                              std::string n0, const AttributeValue &v0,
                                              std::string n1, const AttributeValue &v1,
                                              std::string n2, const AttributeValue &v2,
                                              std::string n3, const AttributeValue &v3,
                                              std::string n4, const AttributeValue &v4,
                                              std::string n5, const AttributeValue &v5,
                                              std::string n6, const AttributeValue &v6,
                                              std::string n7, const AttributeValue &v7)
{
  ObjectFactory factory;
  factory.SetTypeId (type);
  factory.Set (n0,v0);
  factory.Set (n1,v1);
  factory.Set (n2,v2);
  factory.Set (n3,v3);
  factory.Set (n4,v4);
  factory.Set (n5,v5);
  factory.Set (n6,v6);
  factory.Set (n7,v7);
  m_pit = factory;
}
void
NamedDataHelper::SetFib (std::string type,
                                              std::string n0, const AttributeValue &v0,
                                              std::string n1, const AttributeValue &v1,
                                              std::string n2, const AttributeValue &v2,
                                              std::string n3, const AttributeValue &v3,
                                              std::string n4, const AttributeValue &v4,
                                              std::string n5, const AttributeValue &v5,
                                              std::string n6, const AttributeValue &v6,
                                              std::string n7, const AttributeValue &v7)
{
  ObjectFactory factory;
  factory.SetTypeId (type);
  factory.Set (n0,v0);
  factory.Set (n1,v1);
  factory.Set (n2,v2);
  factory.Set (n3,v3);
  factory.Set (n4,v4);
  factory.Set (n5,v5);
  factory.Set (n6,v6);
  factory.Set (n7,v7);
  m_fib = factory;
}

void
NamedDataHelper::SetCS (std::string type,
                                              std::string n0, const AttributeValue &v0,
                                              std::string n1, const AttributeValue &v1,
                                              std::string n2, const AttributeValue &v2,
                                              std::string n3, const AttributeValue &v3,
                                              std::string n4, const AttributeValue &v4,
                                              std::string n5, const AttributeValue &v5,
                                              std::string n6, const AttributeValue &v6,
                                              std::string n7, const AttributeValue &v7)
{
  ObjectFactory factory;
  factory.SetTypeId (type);
  factory.Set (n0,v0);
  factory.Set (n1,v1);
  factory.Set (n2,v2);
  factory.Set (n3,v3);
  factory.Set (n4,v4);
  factory.Set (n5,v5);
  factory.Set (n6,v6);
  factory.Set (n7,v7);
  m_cs = factory;
}

Ptr<AquaSimNetDevice>
NamedDataHelper::Create(Ptr<Node> node, Ptr<AquaSimNetDevice> device)
{
  Ptr<AquaSimPhy> phy = m_phy.Create<AquaSimPhy>();
  Ptr<AquaSimMac> mac = m_mac.Create<AquaSimMac>();
  Ptr<AquaSimEnergyModel> energyM = m_energyM.Create<AquaSimEnergyModel>();
  //Ptr<AquaSimSync> sync = m_sync.Create<AquaSimSync>();
  //Ptr<AquaSimLocalization> loc = m_localization.Create<AquaSimLocalization>();

  Ptr<NamedData> nd = m_nd.Create<NamedData>();
  Ptr<Pit> pit = m_pit.Create<Pit>();
  Ptr<Fib> fib = m_fib.Create<Fib>();
  Ptr<ContentStorage> cs = m_cs.Create<ContentStorage>();


  device->SetPhy(phy);
  device->SetMac(mac);
  //device->SetMac(mac,sync,loc);
  device->ConnectLayers();

  NS_ASSERT(!m_channel.empty());
  device->SetChannel(m_channel);

  device->SetEnergyModel(energyM);
  device->SetAddress(AquaSimAddress::Allocate());

  if(m_attacker)
  {
    Ptr<AquaSimAttackModel> attackM = m_attackM.Create<AquaSimAttackModel>();
    device->SetAttackModel(attackM);
  }

  //Named Data components.
  nd->SetFib(fib);
  nd->SetPit(pit);
  nd->SetContentStorage(cs);
  nd->SetNetDevice(device);
  device->SetNamedData(nd);

  node->AddDevice(device);

  NS_LOG_DEBUG(this << "Create Dump. Phy:" << device->GetPhy() << " Mac:"
	       << device->GetMac() << " Routing:" << device->GetRouting()
	       << " Channel:" << device->GetChannel() << "\n");

  return device;
}


}  //namespace ns3
