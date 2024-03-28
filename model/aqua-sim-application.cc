/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Based on OnOffAppplication.
// Sends out the packets to a randomly chosen destination, based on the given list of receive sockets
//

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "aqua-sim-application.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AquaSimApplication");

NS_OBJECT_ENSURE_REGISTERED (AquaSimApplication);

TypeId
AquaSimApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AquaSimApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<AquaSimApplication> ()
    .AddAttribute ("DataRate", "The data rate in on state.",
                   DataRateValue (DataRate ("500kb/s")),
                   MakeDataRateAccessor (&AquaSimApplication::m_cbrRate),
                   MakeDataRateChecker ())
    .AddAttribute ("PacketSize", "The size of packets sent in on state",
                   UintegerValue (512),
                   MakeUintegerAccessor (&AquaSimApplication::m_pktSize),
                   MakeUintegerChecker<uint32_t> (1))
//    .AddAttribute ("RemoteDestinationList", "List of the destination addresses",
//                   AddressValue (),
//                   MakeAddressAccessor (&AquaSimApplication::m_peer_list),
//                   MakeAddressChecker ())

    .AddAttribute ("NumberOfDestinations", "Number of destinations",
    			   UintegerValue (),
				   MakeUintegerAccessor (&AquaSimApplication::m_numberOfDestinations),
				   MakeUintegerChecker<uint32_t> ())

    .AddAttribute ("OnTime", "A RandomVariableStream used to pick the duration of the 'On' state.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                   MakePointerAccessor (&AquaSimApplication::m_onTime),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("OffTime", "A RandomVariableStream used to pick the duration of the 'Off' state.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                   MakePointerAccessor (&AquaSimApplication::m_offTime),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("MaxBytes", 
                   "The total number of bytes to send. Once these bytes are sent, "
                   "no packet is sent again, even in on state. The value zero means "
                   "that there is no limit.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&AquaSimApplication::m_maxBytes),
                   MakeUintegerChecker<uint64_t> ())
    .AddAttribute ("Protocol", "The type of protocol to use. This should be "
                   "a subclass of ns3::SocketFactory",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&AquaSimApplication::m_tid),
                   // This should check for SocketFactory as a parent
                   MakeTypeIdChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&AquaSimApplication::m_txTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}


AquaSimApplication::AquaSimApplication ()
  : m_socket_init (false),
    m_connected (false),
    m_residualBits (0),
    m_lastStartTime (Seconds (0)),
    m_totBytes (0)
{
  NS_LOG_FUNCTION (this);
  m_rand = CreateObject<UniformRandomVariable> ();
}

AquaSimApplication::~AquaSimApplication()
{
  NS_LOG_FUNCTION (this);
}

void 
AquaSimApplication::SetMaxBytes (uint64_t maxBytes)
{
  NS_LOG_FUNCTION (this << maxBytes);
  m_maxBytes = maxBytes;
}

Ptr<Socket>
AquaSimApplication::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  // Return the first socket for now // TODO: fix it
  return m_socket_list.at(0);
}

int64_t 
AquaSimApplication::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_onTime->SetStream (stream);
  m_offTime->SetStream (stream + 1);
  return 2;
}

void
AquaSimApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  // TODO: dispose the vectors as well
//  m_socket_dst_map = 0;
  // chain up
  Application::DoDispose ();
}

// Application Methods
void AquaSimApplication::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);

  // Create the socket if not already
  if (!m_socket_init)
    {
	  m_socket_init = true;
	  memset (m_address, 0, 2);
//	  address = Address (11, m_address, 11);
//	  address.CopyTo(m_address);

	  // Create a socket for each destination in list
	  for (uint16_t i = 1; i <= m_numberOfDestinations; i++)
	  {

		  if (i != (GetNode()->GetId() + 1))
		  {
			  Ptr<Socket> socket = Socket::CreateSocket (GetNode (), m_tid);

			  m_address[0] = (i >> 8) & 0xff;
			  m_address[1] = (i >> 0) & 0xff;

			  Address address = Address (2, m_address, 2);

			  PacketSocketAddress packet_socket;
			  packet_socket.SetAllDevices();
			  packet_socket.SetPhysicalAddress(address);
			  packet_socket.SetProtocol(0);

		      if (Inet6SocketAddress::IsMatchingType (Address(packet_socket)))
		        {
		          if (socket->Bind6 () == -1)
		            {
		              NS_FATAL_ERROR ("Failed to bind socket");
		            }
		        }
		      else if (InetSocketAddress::IsMatchingType (Address(packet_socket)) ||
		               PacketSocketAddress::IsMatchingType (Address(packet_socket)))
		        {
		          if (socket->Bind () == -1)
		            {
		        	  std::cout << "Failed to bind socket\n";
		              NS_FATAL_ERROR ("Failed to bind socket");
		            }
		        }
		      socket->Connect (Address(packet_socket));
		      socket->SetAllowBroadcast (true);
		      socket->ShutdownRecv ();

		      /// ???
		      socket->SetConnectCallback (
		        MakeCallback (&AquaSimApplication::ConnectionSucceeded, this),
		        MakeCallback (&AquaSimApplication::ConnectionFailed, this));
		      /// ???

		      // Save socket to list
		      m_socket_list.push_back(socket);
		      // Save address to list
		      m_peer_list.push_back(Address(packet_socket));
		  }
	  }
    }

  m_cbrRateFailSafe = m_cbrRate;

  // Insure no pending event
  CancelEvents ();
  // If we are not yet connected, there is nothing to do here
  // The ConnectionComplete upcall will start timers at that time
  //if (!m_connected) return;
  ScheduleStartEvent ();
}

void AquaSimApplication::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  CancelEvents ();
  if(m_socket_init != 0)
    {
	  for (auto it = m_socket_list.begin(); it != m_socket_list.end(); ++it)
	  {
		  (*it)->Close();
	  }
    }
  else
    {
      NS_LOG_WARN ("AquaSimApplication found null socket to close in StopApplication");
    }
}

void AquaSimApplication::CancelEvents ()
{
  NS_LOG_FUNCTION (this);

  if (m_sendEvent.IsRunning () && m_cbrRateFailSafe == m_cbrRate )
    { // Cancel the pending send packet event
      // Calculate residual bits since last packet sent
      Time delta (Simulator::Now () - m_lastStartTime);
      int64x64_t bits = delta.To (Time::S) * m_cbrRate.GetBitRate ();
      m_residualBits += bits.GetHigh ();
    }
  m_cbrRateFailSafe = m_cbrRate;
  Simulator::Cancel (m_sendEvent);
  Simulator::Cancel (m_startStopEvent);
}

// Event handlers
void AquaSimApplication::StartSending ()
{
  NS_LOG_FUNCTION (this);
  m_lastStartTime = Simulator::Now ();
  ScheduleNextTx ();  // Schedule the send packet event
  ScheduleStopEvent ();
}

void AquaSimApplication::StopSending ()
{
  NS_LOG_FUNCTION (this);
  CancelEvents ();

  ScheduleStartEvent ();
}

// Private helpers
void AquaSimApplication::ScheduleNextTx ()
{
  NS_LOG_FUNCTION (this);

  if (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    {
      uint32_t bits = m_pktSize * 8 - m_residualBits;
      NS_LOG_LOGIC ("bits = " << bits);
      Time nextTime (Seconds (bits /
                              static_cast<double>(m_cbrRate.GetBitRate ()))); // Time till next packet
      NS_LOG_LOGIC ("nextTime = " << nextTime);
      m_sendEvent = Simulator::Schedule (nextTime,
                                         &AquaSimApplication::SendPacket, this);
    }
  else
    { // All done, cancel any pending events
      StopApplication ();
    }
}

void AquaSimApplication::ScheduleStartEvent ()
{  // Schedules the event to start sending data (switch to the "On" state)
  NS_LOG_FUNCTION (this);

  Time offInterval = Seconds (m_offTime->GetValue ());
  NS_LOG_LOGIC ("start at " << offInterval);
  m_startStopEvent = Simulator::Schedule (offInterval, &AquaSimApplication::StartSending, this);
}

void AquaSimApplication::ScheduleStopEvent ()
{  // Schedules the event to stop sending data (switch to "Off" state)
  NS_LOG_FUNCTION (this);

  Time onInterval = Seconds (m_onTime->GetValue ());
  NS_LOG_LOGIC ("stop at " << onInterval);
  m_startStopEvent = Simulator::Schedule (onInterval, &AquaSimApplication::StopSending, this);
}


void AquaSimApplication::SendPacket ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());
  Ptr<Packet> packet = Create<Packet> (m_pktSize);
  m_txTrace (packet);

  // Randomly select a socket, and send packet
  int index = m_rand->GetValue(0, m_numberOfDestinations - 1);

  m_socket_list.at(index)->Send (packet);

//  std::cout << "ADDRESS to SEND: " << m_peer_list.at(index) << "\n";

  m_totBytes += m_pktSize;
  if (InetSocketAddress::IsMatchingType (m_peer_list.at(index)))
    {
//	  std::cout << "At time " << Simulator::Now ().GetSeconds ()
//                           << "s on-off application sent "
//                           <<  packet->GetSize () << " bytes to "
//                           << InetSocketAddress::ConvertFrom(m_peer_list.at(index)).GetIpv4 ()
//                           << " port " << InetSocketAddress::ConvertFrom (m_peer_list.at(index)).GetPort ()
//                           << " total Tx " << m_totBytes << " bytes\n";

      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                   << "s on-off application sent "
                   <<  packet->GetSize () << " bytes to "
                   << InetSocketAddress::ConvertFrom(m_peer_list.at(index)).GetIpv4 ()
                   << " port " << InetSocketAddress::ConvertFrom (m_peer_list.at(index)).GetPort ()
                   << " total Tx " << m_totBytes << " bytes");
    }
  else if (Inet6SocketAddress::IsMatchingType (m_peer_list.at(index)))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                   << "s on-off application sent "
                   <<  packet->GetSize () << " bytes to "
                   << Inet6SocketAddress::ConvertFrom(m_peer_list.at(index)).GetIpv6 ()
                   << " port " << Inet6SocketAddress::ConvertFrom (m_peer_list.at(index)).GetPort ()
                   << " total Tx " << m_totBytes << " bytes");
    }
  m_lastStartTime = Simulator::Now ();
  m_residualBits = 0;
  ScheduleNextTx ();
}


void AquaSimApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void AquaSimApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

} // Namespace ns3
