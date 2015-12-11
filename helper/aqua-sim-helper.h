/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef AQUA_SIM_HELPER_H
#define AQUA_SIM_HELPER_H

#include <string>
#include "ns3/attribute.h"
#include "ns3/object-factory.h"
#include "ns3/aqua-sim-channel.h"
#include "ns3/aqua-sim-net-device.h"
#include "ns3/node.h"


namespace ns3 {

class AquaSimChannelHelper
{
public:
  AquaSimChannelHelper ();
  static AquaSimChannelHelper Default (void);

  void SetPropagation (std::string name,
			     std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
			     std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
			     std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
			     std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
			     std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
			     std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
			     std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
			     std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
  void SetNoiseGenerator (std::string name,
			     std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
			     std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
			     std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
			     std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
			     std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
			     std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
			     std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
			     std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());

  Ptr<AquaSimChannel> Create (void) const;
  //int64_t AssignStreams (Ptr<AquaSimChannel> c, int64_t stream);

private:
  ObjectFactory m_propagation;
  ObjectFactory m_noiseGen;
};  // class AquaSimChannelHelper

class AquaSimHelper
{
public:
  AquaSimHelper ();
  static AquaSimHelper Default(void);

  void SetChannel(Ptr<AquaSimChannel> channel);
  void SetPhy (std::string name,
			     std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
			     std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
			     std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
			     std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
			     std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
			     std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
			     std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
			     std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
  void SetMac (std::string name,
			     std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
			     std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
			     std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
			     std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
			     std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
			     std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
			     std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
			     std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
  void SetRouting (std::string name,
			     std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
			     std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
			     std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
			     std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
			     std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
			     std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
			     std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
			     std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());

    Ptr<AquaSimNetDevice> Create (Ptr<Node> node, Ptr<AquaSimNetDevice> device) const;

private:

  Ptr<AquaSimChannel> m_channel;
  ObjectFactory m_phy;
  ObjectFactory m_mac;
  ObjectFactory m_routing;

};  //class AquaSimHelper

/*
 * Reference uan-mac and uan-helper.cc for examples of aggregation system (query interface design pattern)
 */

}

#endif /* AQUA_SIM_HELPER_H */

