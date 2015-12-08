/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "aqua-sim-helper.h"
#include "ns3/log.h"
#include "ns3/assert.h"

namespace ns3 {


AquaSimChannelHelper::AquaSimChannelHelper()
{
}

AquaSimChannelHelper
AquaSimChannelHelper::Default (void)
{
  AquaSimChannelHelper channelHelper;
  channelHelper.SetPropagation ("ns3::AquaSimSimplePropagation");
  channelHelper.SetNoiseGenerator("ns3::AquaSimConstNoiseGen");
  return channelHelper;
}

void
AquaSimChannelHelper::SetPropagation (std::string type,
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
  m_propagation = factory;
}

void
AquaSimChannelHelper::SetNoiseGenerator (std::string type,
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
  m_noiseGen = factory;
}

Ptr<AquaSimChannel>
AquaSimChannelHelper::Create (void) const
{
  Ptr<AquaSimChannel> channel = CreateObject<AquaSimChannel> ();
  Ptr<AquaSimPropagation> prop = m_propagation.Create<AquaSimPropagation>();
  Ptr<AquaSimNoiseGen> noise = m_noiseGen.Create<AquaSimNoiseGen>();
  channel->SetPropagation(prop);
  channel->SetNoiseGenerator(noise);
  return channel;
}

/*
int64_t
AquaSimChannelHelper::AssignStreams (Ptr<AquaSimChannel> c, int64_t stream)
{
  return c->AssignStreams (stream);	//this needs to be implemented or removed...
}
*/

AquaSimHelper::AquaSimHelper() :
    m_channel(0)
{
  m_phy.SetTypeId("ns3::AquaSimPhyCmn");
  m_phy.Set("CPThresh", DoubleValue(10));
  m_phy.Set("CSThresh", DoubleValue(0));
  m_phy.Set("RXThresh", DoubleValue(0));
  m_phy.Set("PT", DoubleValue(0));
  m_phy.Set("Frequency", DoubleValue(25));
  m_phy.Set("K", DoubleValue(2.0));
  m_mac.SetTypeId("ns3::AquaSimMac");	//TODO update... this is base class.
  m_routing.SetTypeId("ns3::AquaSimRouting"); //TODO update... this is base class.
}

AquaSimHelper
AquaSimHelper::Default()
{
  AquaSimHelper asHelper;

  //TODO populate the default case once child classes are created for mac/routing

  return asHelper;
}

void
AquaSimHelper::SetChannel(Ptr<AquaSimChannel> channel)
{
  m_channel = channel;
}

void
AquaSimHelper::SetPhy (std::string type,
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
AquaSimHelper::SetMac (std::string type,
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
  m_mac = factory;
}

void
AquaSimHelper::SetRouting (std::string type,
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
  m_routing = factory;
}

Ptr<AquaSimNetDevice>
AquaSimHelper::Create(AquaSimNode * node, Ptr<AquaSimNetDevice> device) const
{
  Ptr<AquaSimPhy> phy = m_phy.Create<AquaSimPhy>();
  Ptr<AquaSimMac> mac = m_mac.Create<AquaSimMac>();
  Ptr<AquaSimRouting> routing = m_routing.Create<AquaSimRouting>();

  device->SetPhy(phy);
  device->SetMac(mac);
  device->SetRouting(routing);

  NS_ASSERT(m_channel);
  device->SetChannel(m_channel);

  node->AddDevice(device);

  return device;
}

}  //namespace ns3

