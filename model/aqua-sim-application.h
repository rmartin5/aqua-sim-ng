/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Based on OnOffAppplication.
// Sends out the packets to a randomly chosen destination, based on the given list of receive sockets
//

#ifndef AQUA_SIM_APPLICATION_H
#define AQUA_SIM_APPLICATION_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

class Address;
class RandomVariableStream;
class Socket;


class AquaSimApplication : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  AquaSimApplication ();

  virtual ~AquaSimApplication();

  /**
   * \brief Set the total number of bytes to send.
   *
   * Once these bytes are sent, no packet is sent again, even in on state.
   * The value zero means that there is no limit.
   *
   * \param maxBytes the total number of bytes to send
   */
  void SetMaxBytes (uint64_t maxBytes);

  /**
   * \brief Return a pointer to associated socket.
   * \return pointer to associated socket
   */
  Ptr<Socket> GetSocket (void) const;

 /**
  * \brief Assign a fixed random variable stream number to the random variables
  * used by this model.
  *
  * \param stream first stream index to use
  * \return the number of stream indices assigned by this model
  */
  int64_t AssignStreams (int64_t stream);

protected:
  virtual void DoDispose (void);
private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  //helpers
  /**
   * \brief Cancel all pending events.
   */
  void CancelEvents ();

  // Event handlers
  /**
   * \brief Start an On period
   */
  void StartSending ();
  /**
   * \brief Start an Off period
   */
  void StopSending ();
  /**
   * \brief Send a packet
   */
  void SendPacket ();

  // Flag to initialize socket-dst map
  bool m_socket_init;
  // List of sockets for each destination
  std::vector<Ptr<Socket>> m_socket_list;
  // List of destination addresses
  std::vector<Address> m_peer_list;

  bool            m_connected;    //!< True if connected
  Ptr<RandomVariableStream>  m_onTime;       //!< rng for On Time
  Ptr<RandomVariableStream>  m_offTime;      //!< rng for Off Time
  DataRate        m_cbrRate;      //!< Rate that data is generated
  DataRate        m_cbrRateFailSafe;      //!< Rate that data is generated (check copy)
  uint32_t        m_pktSize;      //!< Size of packets
  uint32_t        m_residualBits; //!< Number of generated, but not sent, bits
  Time            m_lastStartTime; //!< Time last packet sent
  uint64_t        m_maxBytes;     //!< Limit total number of bytes sent
  uint64_t        m_totBytes;     //!< Total bytes sent so far
  EventId         m_startStopEvent;     //!< Event id for next start or stop event
  EventId         m_sendEvent;    //!< Event id of pending "send packet" event
  TypeId          m_tid;          //!< Type of the socket used

  // For random destination selection
  Ptr<UniformRandomVariable> m_rand;
  uint32_t m_numberOfDestinations = 0;

  uint8_t m_address[2]; //!< address value

  /// Traced Callback: transmitted packets.
  TracedCallback<Ptr<const Packet> > m_txTrace;

private:
  /**
   * \brief Schedule the next packet transmission
   */
  void ScheduleNextTx ();
  /**
   * \brief Schedule the next On period start
   */
  void ScheduleStartEvent ();
  /**
   * \brief Schedule the next Off period start
   */
  void ScheduleStopEvent ();
  /**
   * \brief Handle a Connection Succeed event
   * \param socket the connected socket
   */
  void ConnectionSucceeded (Ptr<Socket> socket);
  /**
   * \brief Handle a Connection Failed event
   * \param socket the not connected socket
   */
  void ConnectionFailed (Ptr<Socket> socket);
};

} // namespace ns3

#endif /* AQUA_SIM_APPLICATION_H */
