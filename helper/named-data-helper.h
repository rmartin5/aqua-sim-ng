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
#ifndef NAMED_DATA_HELPER_H
#define NAMED_DATA_HELPER_H

#include <string>
#include "ns3/attribute.h"
#include "ns3/object-factory.h"
#include "ns3/aqua-sim-channel.h"

namespace ns3 {

class AquaSimNetDevice;
class Node;

class NamedDataHelper
{
public:
  NamedDataHelper ();

  void SetChannel(Ptr<AquaSimChannel> channel);
  Ptr<AquaSimChannel> GetChannel(int channelId = 0);
  void SetAttacker(bool attacker);
  void SetPhy (std::string name,
			     std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
			     std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
			     std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
			     std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
			     std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
			     std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
			     std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
			     std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
  void SetEnergyModel (std::string name,
 			     std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
 			     std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
 			     std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
 			     std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
 			     std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
 			     std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
 			     std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
 			     std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
   void SetSync (std::string name,
 			     std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
 			     std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
 			     std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
 			     std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
 			     std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
 			     std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
 			     std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
 			     std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
   void SetLocalization (std::string name,
 			     std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
 			     std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
 			     std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
 			     std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
 			     std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
 			     std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
 			     std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
 			     std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
   void SetAttackModel (std::string name,
 			     std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
 			     std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
 			     std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
 			     std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
 			     std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
 			     std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
 			     std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
 			     std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
   void SetNamedData (std::string name,
 			     std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
 			     std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
 			     std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
 			     std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
 			     std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
 			     std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
 			     std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
 			     std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
   void SetPit (std::string name,
 			     std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
 			     std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
 			     std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
 			     std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
 			     std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
 			     std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
 			     std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
 			     std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
   void SetFib (std::string name,
 			     std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
 			     std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
 			     std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
 			     std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
 			     std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
 			     std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
 			     std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
 			     std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
   void SetCS (std::string name,
 			     std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
 			     std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
 			     std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
 			     std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
 			     std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
 			     std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
 			     std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
 			     std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
    Ptr<AquaSimNetDevice> Create (Ptr<Node> node, Ptr<AquaSimNetDevice> device);
private:
  std::vector<Ptr<AquaSimChannel> > m_channel;
  ObjectFactory m_phy;
  ObjectFactory m_mac;
  ObjectFactory m_energyM;
  ObjectFactory m_sync;
  ObjectFactory m_localization;
  ObjectFactory m_attackM;
  bool m_attacker;  //default is false

  ObjectFactory m_nd;
  ObjectFactory m_pit;
  ObjectFactory m_fib;
  ObjectFactory m_cs;
};  //class NamedDataHelper

}

#endif /* NAMED_DATA_HELPER_H */
