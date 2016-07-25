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

#include "ns3/vector.h"
#include "ns3/log.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/integer.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/mobility-model.h"

#include "aqua-sim-net-device.h"
#include "aqua-sim-mac.h"
#include "aqua-sim-channel.h"
#include "aqua-sim-signal-cache.h"
#include "aqua-sim-address.h"

#include <utility>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimNetDevice");
NS_OBJECT_ENSURE_REGISTERED (AquaSimNetDevice);

AquaSimNetDevice::AquaSimNetDevice ()
  : NetDevice(),
    m_nextHop(1), //-10?
    m_setHopStatus(0),
    m_sinkStatus(0),
    m_statusChangeTime(0.0),
    m_failureStatus(false), //added by peng xie
    m_failurePro(0.0), //added by peng xie
    m_failureStatusPro(0.0), //added by peng xie and zheng
    m_cX(0.0),
    m_cY(0.0),
    m_cZ(0.0),
    m_carrierSense(false),
    m_carrierId(false),
    m_ifIndex(0),
    m_mtu(64000),
    m_distCST(3000),
    m_totalSentPkts(0)
{
  m_transStatus = NIDLE;
  m_configComplete = false;
  NS_LOG_FUNCTION(this);
}

AquaSimNetDevice::~AquaSimNetDevice ()
{
  NS_LOG_FUNCTION(this);
}

TypeId
AquaSimNetDevice::GetTypeId ()
{
  static TypeId tid = TypeId("ns3::AquaSimNetDevice")
    .SetParent<NetDevice>()
    .AddAttribute ("Phy", "The PHY layer attached to this device.",
      PointerValue (),
      MakePointerAccessor (&AquaSimNetDevice::m_phy),
      MakePointerChecker<AquaSimPhy>())
    .AddAttribute ("Mac", "The MAC layer attached to this device.",
      PointerValue (),
      MakePointerAccessor (&AquaSimNetDevice::m_mac),
      MakePointerChecker<AquaSimMac>())
    .AddAttribute ("Routing", "The Routing layer attached to this device.",
      PointerValue (),
      MakePointerAccessor (&AquaSimNetDevice::m_routing),
      MakePointerChecker<AquaSimRouting>())
    .AddAttribute ("Channel", "The Channel layer attached to this device.",
      PointerValue (),
      MakePointerAccessor (&AquaSimNetDevice::m_channel),
      MakePointerChecker<AquaSimChannel>())
  //.AddAttribute ("App", "The App layer attached to this device.",
    //PointerValue (&AquaSimNetDevie::m_app),
    //MakePointerAccessor (&AquaSimNetDevice::GetApp, &AquaSimNetDevice::SetApp),
    //MakePointerChecker<AquaSimApp>())
  //3 following commands are for VBF related protocols only
    .AddAttribute("SetCx", "Set x for VBF related protocols.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimNetDevice::m_cX),
      MakeDoubleChecker<double>())
   .AddAttribute("SetCy", "Set y for VBF related protocols.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimNetDevice::m_cY),
      MakeDoubleChecker<double>())
   .AddAttribute("SetCz", "Set z for VBF related protocols.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimNetDevice::m_cZ),
      MakeDoubleChecker<double>())
   .AddAttribute("SetFailureStatus", "Set node failure status. Default false.",
     BooleanValue(0),
     MakeBooleanAccessor(&AquaSimNetDevice::m_failureStatus),
     MakeBooleanChecker())
   .AddAttribute("SetFailureStatusPro", "Set node failure status pro.",
     DoubleValue(0),
     MakeDoubleAccessor(&AquaSimNetDevice::m_failureStatusPro),
     MakeDoubleChecker<double>())
   .AddAttribute("SetFailurePro", "Set node failure pro.",
     DoubleValue(0),
     MakeDoubleAccessor(&AquaSimNetDevice::m_failurePro),
     MakeDoubleChecker<double>())
   .AddAttribute("NextHop", "Set next hop. Default is 1.",
     IntegerValue(1),
     MakeIntegerAccessor(&AquaSimNetDevice::m_nextHop),
     MakeIntegerChecker<int>())
   .AddAttribute("SinkStatus", "Set the sink's status, int value.",
     IntegerValue(0),
     MakeIntegerAccessor(&AquaSimNetDevice::m_sinkStatus),
     MakeIntegerChecker<int> ())
  ;
  return tid;
}

void
AquaSimNetDevice::DoDispose (void)
{
  //TODO clear all variables and link to NetDevice to do the same...
  NS_LOG_FUNCTION(this);
  NetDevice::DoDispose ();
}

void
AquaSimNetDevice::DoInitialize (void)
{
  //m_phy->Initialize ();
  //m_mac->Initialize ();
  //m_app->Initialize ();
  //channel?
  NetDevice::DoInitialize ();
}

void
AquaSimNetDevice::CompleteConfig (void)
{
  if (m_mac == 0 || m_phy == 0 || /*m_app == 0 ||*/ m_node == 0 || m_phy || m_configComplete)
    {
      return;
    }
  //exec
  //TODO set app
  m_configComplete = true;
}

void
AquaSimNetDevice::ConnectLayers()
{
  if (m_phy != 0 && m_mac != 0)
    {
      //m_phy->SetMac(m_mac);
      //m_mac->SetPhy(m_phy);
      NS_LOG_DEBUG("Phy/Mac layers set");
    }

  if (m_mac != 0 && m_routing != 0)
    {
      //m_mac->SetRouting(m_routing);
      m_routing->SetMac(m_mac);
      m_mac->SetTransDistance(m_distCST);
      m_routing->SetTransDistance(m_distCST);
      NS_LOG_DEBUG("Routing/Mac layers set");
    }
}

//voidconnectlayers and implementall waBelow stuff
void
AquaSimNetDevice::SetPhy (Ptr<AquaSimPhy> phy)
{
  //currently only supporting single layer per net device
  if (m_phy == 0)
    {
      NS_LOG_FUNCTION(this);
      m_phy = phy;
      m_phy->SetNetDevice (Ptr<AquaSimNetDevice> (this));

      CompleteConfig ();
    }
  else
    NS_LOG_DEBUG("NetDevice could not set phy layer (" << m_phy << ")");
}

void
AquaSimNetDevice::SetMac (Ptr<AquaSimMac> mac, Ptr<AquaSimSync> sync, Ptr<AquaSimLocalization> loc)
{
  //currently only supporting single layer per net device
  if (m_mac == 0)
    {
      NS_LOG_FUNCTION(this);
      m_mac = mac;
      m_mac->SetDevice (Ptr<AquaSimNetDevice> (this));

      if (sync != NULL)
      {
        sync = Create<AquaSimSync>();
        m_macSync = sync;
        sync->SetDevice(Ptr<AquaSimNetDevice>(this));
      }

      if (loc != NULL)
      {
        loc = Create<AquaSimLocalization>();
        m_macLoc = loc;
        loc->SetDevice(Ptr<AquaSimNetDevice>(this));
      }

      CompleteConfig ();
    }
  else
    NS_LOG_DEBUG("NetDevice could not set mac layer (" << m_mac << ")");
}

void
AquaSimNetDevice::SetRouting(Ptr<AquaSimRouting> routing)
{
  //currently only supporting single layer per net device
  if (m_routing == 0)
    {
      NS_LOG_FUNCTION(this);
      m_routing = routing;
      m_routing->SetNetDevice(Ptr<AquaSimNetDevice> (this));

      CompleteConfig ();
    }
  else
    NS_LOG_DEBUG("NetDevice could not set routing layer (" << m_routing << ")");
}

void
AquaSimNetDevice::SetChannel (Ptr<AquaSimChannel> channel)
{
  //currently only supporting single channel per net device
  if (m_channel == 0)
    {
      NS_LOG_FUNCTION(this);
      m_channel = channel;
      m_channel->AddDevice(Ptr<AquaSimNetDevice> (this));

      if (m_phy != 0)
	{
	  m_phy->SetChannel(m_channel);
	  m_phy->GetSignalCache()->SetNoiseGen(m_channel->GetNoiseGen());
	}
      CompleteConfig ();
    }
  else
    NS_LOG_DEBUG("NetDevice could not set channel (" << m_channel << ")");
}
/*
void
AquaSimNetDevice::SetApp (Ptr<AquaSimApp> app)
{
  NS_LOG_FUNCTION(this);
  m_app = app;
  CompleteConfig ();
}
*/
void
AquaSimNetDevice::SetEnergyModel (Ptr<AquaSimEnergyModel> energyModel)
{
  if (m_energyModel == 0)
  {
    NS_LOG_FUNCTION(this);
    m_energyModel = energyModel;
    m_energyModel->SetDevice(Ptr<AquaSimNetDevice> (this));
  }
}

double
AquaSimNetDevice::TransmitDistance()
{
  return m_distCST;
}

void
AquaSimNetDevice::SetTransmitDistance(double range)
{
  m_distCST = range;
}
void
AquaSimNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION(this);
  m_node = node;
}

Ptr<AquaSimPhy>
AquaSimNetDevice::GetPhy (void)
{
  return m_phy;
}

Ptr<AquaSimMac>
AquaSimNetDevice::GetMac (void)
{
  return m_mac;
}

Ptr<AquaSimSync>
AquaSimNetDevice::GetMacSync(void)
{
  return m_macSync;
}

Ptr<AquaSimLocalization>
AquaSimNetDevice::GetMacLoc(void)
{
  return m_macLoc;
}

Ptr<AquaSimRouting>
AquaSimNetDevice::GetRouting (void)
{
  return m_routing;
}

Ptr<AquaSimChannel>
AquaSimNetDevice::DoGetChannel (void) const
{
  return m_channel;
}

Ptr<Channel>
AquaSimNetDevice::GetChannel (void) const
{
  return m_channel;
}

Ptr<Node>
AquaSimNetDevice::GetNode (void) const
{
  return m_node;
}

/*
AquaSimNetDevice::GetApp (void)
{
  return m_app;
}
*/

bool
AquaSimNetDevice::IsMoving(void)
{
  NS_LOG_FUNCTION(this);

  Ptr<Object> object = GetNode();
  Ptr<MobilityModel> model = object->GetObject<MobilityModel>();

  if (model == 0){
      return false;
  }

  Vector vel = model->GetVelocity();
  if (vel.x==0 && vel.y==0 && vel.z==0) {
      return false;
  }

  return true;
}

int
AquaSimNetDevice::SetSinkStatus()
{
  m_sinkStatus = 1;
  return 0;
}


int
AquaSimNetDevice::ClearSinkStatus()
{
  m_sinkStatus = 0;
  return 0;
}

void
AquaSimNetDevice::GenerateFailure()
{
  double error_pro = m_uniformRand->GetValue();
  if (error_pro < m_failureStatusPro)
    m_failureStatus = true;
}

void
AquaSimNetDevice::AddLinkChangeCallback (Callback< void > callback)
{
  NS_LOG_WARN("Not implemented");
}

Address
AquaSimNetDevice::GetAddress (void) const
{
  NS_LOG_DEBUG(this);
  return m_mac->GetAddress();
}

Address
AquaSimNetDevice::GetBroadcast (void) const
{
  NS_LOG_WARN("Not implemented since UW is median is always broadcast (i.e. 255).");
  return Address();
}

uint32_t
AquaSimNetDevice::GetIfIndex (void) const
{
  return m_ifIndex;
}

uint16_t
AquaSimNetDevice::GetMtu (void) const
{
  return m_mtu;
}

Address
AquaSimNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_WARN("Not implemented");
  return Address();
}

Address
AquaSimNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_WARN("Not implemented");
  return Address();
}

bool
AquaSimNetDevice::IsBridge (void) const
{
  return false;
}

bool
AquaSimNetDevice::IsBroadcast (void) const
{
  return true;
}

bool
AquaSimNetDevice::IsLinkUp (void) const
{
  return (m_phy != 0);
}

bool
AquaSimNetDevice::IsMulticast (void) const
{
  return false;
}

bool
AquaSimNetDevice::IsPointToPoint (void) const
{
  return false;
}

bool
AquaSimNetDevice::NeedsArp (void) const
{
  return false;
}

bool
AquaSimNetDevice::Send (Ptr< Packet > packet, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION(this << packet << dest << protocolNumber);

  m_totalSentPkts++;  //debugging

  if(m_routing)
    {//Note : https://www.nsnam.org/docs/release/3.24/doxygen/uan-mac-cw_8cc_source.html#l00123
      if(m_mac)
      {
          //redudant
	      //m_routing->SetMyAddr(AquaSimAddress::ConvertFrom(m_mac->GetAddress()));
        return m_routing->Recv(packet, dest, protocolNumber);
        //return m_routing->SendDown(packet, AquaSimAddress::ConvertFrom(dest), Seconds(0));
      }
    }
  else if (m_mac)
    {
      return m_mac->RecvProcess(packet);
    }
  /*else if (m_phy)
    {
      NS_LOG_DEBUG("Phy SendDown hit");
      return m_phy->PktTransmit(packet);
    }
    */
  else NS_LOG_WARN("Routing/Mac layers are not attached to this device. Can not send.");
  return false;
}

bool
AquaSimNetDevice::SendFrom (Ptr< Packet > packet, const Address &source, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_WARN("SendFrom not supported");
  return false;
}

void
AquaSimNetDevice::SetAddress (Address address)
{
  m_mac->SetAddress(AquaSimAddress::ConvertFrom(address));
}

void
AquaSimNetDevice::SetIfIndex (uint32_t index)
{
  m_ifIndex = index;
}

bool
AquaSimNetDevice::SetMtu (uint16_t mtu)
{
  m_mtu = mtu;
  return true;
}

void
AquaSimNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG_WARN("PromiscRecvCB Not supported");
}

void
AquaSimNetDevice::SetReceiveCallback (ReceiveCallback cb)
{
  NS_LOG_WARN("RecvCallback not implemented");
  m_forwardUp = cb;
}

bool
AquaSimNetDevice::SupportsSendFrom (void) const
{
  return false;
}

int
AquaSimNetDevice::GetHopStatus()
{
  return m_setHopStatus;
}

int
AquaSimNetDevice::GetNextHop()
{
  return m_nextHop;
}


void
AquaSimNetDevice::SetTransmissionStatus(TransStatus status)
{
  if (GetTransmissionStatus() == SLEEP){
      m_mac->PowerOff();
      return;
  }

  m_transStatus = status;

 if (!m_mac->SendQueueEmpty()) {
     std::pair<Ptr<Packet>,TransStatus> sendPacket = m_mac->SendQueuePop();
     m_mac->SendDown(sendPacket.first,sendPacket.second);
 }
}

TransStatus
AquaSimNetDevice::GetTransmissionStatus(void)
{
  return m_transStatus;
}
