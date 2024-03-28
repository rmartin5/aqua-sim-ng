/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Author: Dmitrii Dugaev <ddugaev@gradcenter.cuny.edu>
 */

#include "aqua-sim-mac-libra.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-mac.h"
#include "aqua-sim-address.h"

#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/simulator.h"

#include <math.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimMacLibra");
NS_OBJECT_ENSURE_REGISTERED(AquaSimMacLibra);


/* ======================================================================
Adaptive forwarding (routing) MAC for underwater sensor networks
====================================================================== */

/*
 * This function calculates the number of relays that minimize the energy consumption
 *
 * Arguments
 * distance:            the distance between the source and destination (km)
 * packet_size:         the size of the packet (bytes)
 * link_speed:          the physical layer speed (bps)
 * p_rx:                the power consumption for receiving and processing (watts)
 * p_tx_max:            the maximum transmission power (watts)
 *
 * Return value
 * n:                   the optimal number of relays
 */

//int AquaSimMacLibra::CalculateHopCount(double distance, int packet_size, int link_speed, double p_rx, double p_tx_max)
uint8_t
AquaSimMacLibra::CalculateHopCount(double distance, int packet_size, double p_tx_max)
{
  double freq = 25.0; // Frequency, in kHz
  double k = 2.0; // Spreading factor
  int link_speed = 10000; // bps
  double p_rx = 0.82; // Watts
  double t_rx = packet_size * 8.0 / (link_speed*1.0); // Transmission delay
  double p_rx_min = 5.4764*pow(10,(-8.0)); // Minimal signal strength for successful reception
  double C1 = (0.011*pow(freq,2.0)/(1.0+pow(freq,2))+4.4*pow(freq,2.0)/(4100.0+freq)+2.75/100000.0*pow(freq,2.0)+0.0003)*(distance/1000.0);
  double C2 = p_rx_min*pow(distance,k)*8.0*packet_size/(1.0*link_speed);
  double C3 = t_rx * p_rx;
  double Em = 0.0; // The energy consumption with relays
  double Em_last = 0.0; // A temporary variable to store the previous calculated energy consumption
  int n = 0, loop_count = 10;

  Em_last = C2*pow(10.0,C1/(n+1.0))/pow((n+1.0),(k-1.0))+n*C3;
  while (n++ < loop_count) {
    Em = C2*pow(10.0,C1/(n+1.0))/pow((n+1.0),(k-1.0))+n*C3;
    if (Em < Em_last) {
      Em_last = Em;
    }
    else {
      n--;
      break;
    }
  }
//  std::cout << "OPTIMAL HOP COUNT: " << n << "\n";
//  std::cout << "DISTANCE: " << distance << "\n";
  // Fix the number of hops for the experiments
 return n;

// // Fix the number of hops for the experiments !!!
// return m_nintermediate_nodes;
}


AquaSimMacLibra::AquaSimMacLibra()
{
  m_rand = CreateObject<UniformRandomVariable> ();
  m_max_range = 150;
  m_max_tx_power = 20; // Watts
  m_isInRange = true;
  m_nintermediate_nodes = 0;
  // Flag to indicate whether the initial distances are initialized
  m_distances_initialized = false;
}

TypeId
AquaSimMacLibra::GetTypeId()
{
  static TypeId tid = TypeId("ns3::AquaSimMacLibra")
      .SetParent<AquaSimMac>()
      .AddConstructor<AquaSimMacLibra>()
      .AddAttribute("max_range", "Maximum transmission range",
        DoubleValue(150),
        MakeDoubleAccessor (&AquaSimMacLibra::m_max_range),
        MakeDoubleChecker<double> ())
	  .AddAttribute("max_tx_power", "Maximum transmission power",
		DoubleValue(20),
		MakeDoubleAccessor (&AquaSimMacLibra::m_max_tx_power),
		MakeDoubleChecker<double> ())
	  .AddAttribute("packet_size", "Packet size from upper layers",
		IntegerValue(50),
		MakeIntegerAccessor (&AquaSimMacLibra::m_packet_size),
		MakeIntegerChecker<int> ())
	// Pass the number of intermediate hops explicitly (for some experiments only !!!)
	  .AddAttribute("intermediate_nodes", "Number of intermediate nodes for each packet transmission",
		IntegerValue(0), // 0 - direct transmission to destination
		MakeIntegerAccessor (&AquaSimMacLibra::m_nintermediate_nodes),
		MakeIntegerChecker<int> ())

     ;
  return tid;
}

int64_t
AquaSimMacLibra::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_rand->SetStream(stream);
  return 1;
}

/*
this program is used to handle the received packet,
it should be virtual function, different class may have
different versions.
*/
bool
AquaSimMacLibra::RecvProcess (Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this);
  /*std::cout << "\nBMac @RecvProcess check:\n";
  pkt->Print(std::cout);
  std::cout << "\n";*/

  // Calculate initial distances
  if (!m_distances_initialized)
  {
  	InitializeDistances();
  	m_distances_initialized = true;
  }

  AquaSimHeader ash;
  MacHeader mach;
  MacLibraHeader mac_libra_h;

  pkt->RemoveHeader(ash);
  pkt->RemoveHeader(mach);
  pkt->RemoveHeader(mac_libra_h);

	AquaSimAddress dst = mac_libra_h.GetDstAddr();
	AquaSimAddress sender_addr = mac_libra_h.GetSenderAddr();

	if (ash.GetErrorFlag())
	{
		NS_LOG_DEBUG("LibraMac:RecvProcess: received corrupt packet.");
		pkt=0;
		return false;
	}

		NS_LOG_DEBUG("\n-------------------\nPTYPE: " << mac_libra_h.GetPType());
		NS_LOG_DEBUG("TS: " << Simulator::Now());
		NS_LOG_DEBUG("SRC: " << mac_libra_h.GetSrcAddr());
		NS_LOG_DEBUG("DST: " << mac_libra_h.GetDstAddr());
		NS_LOG_DEBUG("HOP COUNT: " << mac_libra_h.GetHopCount());


		// DATA PACKET TYPE
		////////////////////
		////////////////////

		// If the DATA packet comes from the network (i.e. PHY), then process it
		if (mac_libra_h.GetPType() == 0)
		{
			// TODO: maybe to include some overhearing here. I.e., update the local distances table from all received packets
			// The DATA packet must be destined to the node
			if (dst == AquaSimAddress::ConvertFrom(m_device->GetAddress()))
			{
				// If dst_addr is its own, send up
				// If not, forward packet further
				double reward = 0;
				if (mach.GetDA() == AquaSimAddress::ConvertFrom(m_device->GetAddress()))
				{
					// The data packet is for this node, send it up
					pkt->AddHeader(ash);  //leave MacHeader off since sending to upper layers
//					NS_LOG_DEBUG("SENDING UP HOP COUNT: " << mac_libra_h.GetHopCount());
					SendUp(pkt);

					// Generate a single direct reward message back to the sender
					// Set mac_libra_header parameters
					Ptr<Packet> reward_msg = Create<Packet>();
					MacLibraHeader reward_h;
					reward_h.SetPType(7); // 7 - DIRECT REWARD MESSAGE
					reward_h.SetId(0);
					reward_h.SetSrcAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
					reward_h.SetDstAddr(mac_libra_h.GetSrcAddr());

//					std::cout << "REWARD SRC ADDR: " << reward_h.GetSrcAddr() << "\n";
//					std::cout << "REWARD DST ADDR: " << reward_h.GetDstAddr() << "\n";

					// Set aqua-sim-header parameters for correct handling on Phy layer
					ash.SetSize(reward_h.GetSerializedSize() + mach.GetSerializedSize());
					// ash.SetTxTime(Phy()->CalcTxTime(ash.GetSize()) + Seconds(0.0014));
					ash.SetTxTime(Phy()->CalcTxTime(ash.GetSize()));
					// ash.SetTxTime(Phy()->CalcTxTime(ash.GetSize()));
					ash.SetErrorFlag(false);
					ash.SetDirection(AquaSimHeader::DOWN);

					// Set Tx power for the frame
//					reward_h.SetTxPower(CalculateTxPower(CalculateDistance(mac_libra_h.GetTxPower(),
//							mac_libra_h.GetRxPower()))); // Send direct reward message considering the distance

					reward_h.SetTxPower(CalculateTxPower(CalculateDistance(mac_libra_h.GetSrcAddr()))); // Send direct reward message considering the distance
					// Add some "guard distance", just to mitigate the initial error in distance calculation from Tx/Rx
					reward_h.SetNextHopDistance(CalculateDistance(mac_libra_h.GetSrcAddr()) + m_dist_error);

					// Set reward value
					reward = CalculateReward(mac_libra_h.GetOptimalDistance(), mac_libra_h.GetDirectDistance(), m_distances.find(mach.GetDA())->second);
//					std::cout << "DIRECT DISTANCE: " << mac_libra_h.GetDirectDistance() << "\n";
					// Since the node is the destination itself, the residual distance is zero
//					std::cout << "RESIDUAL DISTANCE: " << 0 << "\n";

					reward_h.SetReward(reward);

					reward_msg->AddHeader(reward_h);
					reward_msg->AddHeader(mach);
					reward_msg->AddHeader(ash);

				}
				else
				{
					// Otherwise, insert the reward and forward the packet further
					// Generate reward from the given optimal metric and delta-distance to destination
					// I.e. how much the total distance has been reduced comparing to the optimal one
					reward = CalculateReward(mac_libra_h.GetOptimalDistance(), mac_libra_h.GetDirectDistance(), m_distances.find(mach.GetDA())->second);

					// std::cout << "DIRECT DISTANCE: " << mac_libra_h.GetDirectDistance() << "\n";
					// std::cout << "RESIDUAL DISTANCE: " << m_distances.find(mach.GetDA())->second << "\n";

					// Increment hop count, set reward for the sender node
					mac_libra_h.IncrementHopCount();
					mac_libra_h.SetReward(reward);

					// Set aqua-sim-header parameters for correct handling on Phy layer
					// ash.SetTxTime(GetTxTime(pkt));
					ash.SetTxTime(Phy()->CalcTxTime(ash.GetSize()));
					ash.SetErrorFlag(false);
					ash.SetDirection(AquaSimHeader::DOWN);

					pkt->AddHeader(mac_libra_h);
					pkt->AddHeader(mach);
					pkt->AddHeader(ash);
					// Increment hop_count, set reward
					ForwardPacket(pkt, mac_libra_h.GetSrcAddr());
				}
				return true;
			}

			// Update the weight value by the reward, if the sender_addr is for this node
			if (sender_addr == AquaSimAddress::ConvertFrom(m_device->GetAddress()))
			{
				// Check the reward
				// If the reward = 0, then the packet contains no rewards, i.e. the packet has come from the initial source node
				// In that case, do not update the forwarding table with reward
				double reward = mac_libra_h.GetReward();
				if (reward != 0)
				{
					// Update forwarding table
					UpdateWeight(mach.GetSA(), mach.GetDA(), mac_libra_h.GetSrcAddr(), reward);
				}

			}

			return true;
		}

		// Direct reward
		if (mac_libra_h.GetPType() == 7) // 7 - DIRECT REWARD
		{
			// If this reward for this node, update weight
			// Otherwise, discard
			if (mac_libra_h.GetDstAddr() == AquaSimAddress::ConvertFrom(m_device->GetAddress()))
			{
				UpdateWeight(mach.GetSA(), mach.GetDA(), mac_libra_h.GetSrcAddr(), mac_libra_h.GetReward());
			}
			return true;
		}

		else
		{
//			std::cout << "Unknown packet type is received: " << mac_libra_h.GetPType() << "\n";
		}

//	printf("underwaterAquaSimLibraMac: this is neither broadcast nor my packet, just drop it\n");
	//pkt=0;
	return false;
}


void
AquaSimMacLibra::DropPacket(Ptr<Packet> pkt)
{
  //this is not necessary... only kept for current legacy issues
  pkt=0;
  return;
}


/*
this program is used to handle the transmitted packet,
it should be virtual function, different class may have
different versions.
*/
bool
AquaSimMacLibra::TxProcess(Ptr<Packet> pkt)
{
	NS_LOG_FUNCTION(this << pkt);

	// Calculate initial distances
	if (!m_distances_initialized)
	{
		InitializeDistances();
		m_distances_initialized = true;
	}

	AquaSimHeader ash;
	MacHeader mach;
	MacLibraHeader mac_libra_h;

	pkt->RemoveHeader(ash);

	AquaSimAddress dst_addr = ash.GetDAddr();

	// Populate the list of neighbors within max tx range
	if (m_inrange_addresses.empty() && m_isInRange)
	{
		DiscoverInRangeNodes();
	}

	// Drop packet if node has no neighbors
	if (!m_isInRange)
	{
		// std::cout << "THE NODE HAS NO NEIGHBORS!!! DROPPING PACKET!!!\n";
		return false;
	}

	// Check if the destination is within maximum transmission range or not
	// If the destination is outside the maximum transmission range, then randomly select any other destination, which is within it
	if (!IsWithinMaximumRange(dst_addr))
	{
		uint32_t address_index = m_rand->GetInteger(0, m_inrange_addresses.size() - 1);
		// std::cout << "SELECTED ADDRESS INDEX: " << address_index << "\n";
		// std::cout << "SELECTING THE OTHER DST: " << m_inrange_addresses.at(address_index) << "\n";
		dst_addr = m_inrange_addresses.at(address_index);
	}


	// mach.SetDA(ash.GetDAddr());
	ash.SetDAddr(dst_addr);
	mach.SetDA(dst_addr);
	mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));

	ash.SetSize(ash.GetSize());

	// ash.SetTxTime(GetTxTime(pkt));
	ash.SetTxTime(Phy()->CalcTxTime(ash.GetSize()));

	// Set initial reward and hop_count values
	mac_libra_h.SetHopCount(1);
	mac_libra_h.SetReward(0);
	// Set packet type (DATA type), ID
	mac_libra_h.SetPType(0);
	mac_libra_h.SetId(m_header_id);
	m_header_id++;

	pkt->AddHeader(mac_libra_h);
	pkt->AddHeader(mach);
	pkt->AddHeader(ash);

	ForwardPacket(pkt->Copy(), AquaSimAddress::ConvertFrom(m_device->GetAddress()));

  return true;  //may be bug due to Sleep/default cases
}

bool
AquaSimMacLibra::SendDownFrame (Ptr<Packet> pkt)
{
	NS_LOG_FUNCTION(this << pkt);
	AquaSimHeader ash;
	MacHeader mach;
	MacLibraHeader mac_libra_h;

	pkt->RemoveHeader(ash);
	pkt->RemoveHeader(mach);
	pkt->RemoveHeader(mac_libra_h);

	// Set mac type to mac header
	mach.SetDemuxPType(MacHeader::UWPTYPE_MAC_LIBRA);

	ash.SetTimeStamp(Simulator::Now());
	ash.SetDirection(AquaSimHeader::DOWN);
	pkt->AddHeader(mac_libra_h);
	pkt->AddHeader(mach);
	pkt->AddHeader(ash);

  // Check net_device status
  // If it is IDLE, send packet down to PHY immediately
  // otherwise, do random backoff again
  if (m_device->GetTransmissionStatus() == TransStatus::NIDLE)
  {
    // Call parent method to send packet down to PHY
    SendDown(pkt->Copy());
    // Go to next packet in queue
    SendPacket();
	// Simulator::Schedule(Seconds(GetBackoff()), &AquaSimMacLibra::SendPacket, this);
  }
  else
  {
	//   std::cout << Simulator::Now().GetSeconds() << "\n";
    // Do another backoff
    Simulator::Schedule(Seconds(GetBackoff()), &AquaSimMacLibra::SendDownFrame, this, pkt->Copy());
  }
  return true;
}

// Return a random number in-between given range
double AquaSimMacLibra::GetBackoff()
{
//   return m_rand->GetValue(0.1, 1.0);
  return m_rand->GetValue(1.0, 5.0);
}

// Try to send packet to PHY layer, after backoff
void AquaSimMacLibra::SendPacket()
{
	NS_LOG_FUNCTION(this);

  // If queue is empty -> nothing to send -> return
  if (m_send_buffer.size() == 0)
  {
    return;
  }

  // Get packet from queue
  Ptr<Packet> pkt = m_send_buffer.front();
  m_send_buffer.pop();

  SendDownFrame(pkt->Copy());
}

// Forward packet / frame coming from the application or the network (DATA type)
bool
AquaSimMacLibra::ForwardPacket(Ptr<Packet> p, AquaSimAddress sender_addr)
{
	AquaSimHeader ash;
	MacHeader mach;
	MacLibraHeader mac_libra_h;

	p->RemoveHeader(ash);
	p->RemoveHeader(mach);
	p->RemoveHeader(mac_libra_h);

	// Store destination address as integer
	AquaSimAddress dst_addr = mach.GetDA();

	// Reset the direction if the packet is forwarded by intermediate node
	ash.SetDirection(AquaSimHeader::DOWN);

	// If it is a source of initial packet, set the optimal_distance and max_hops_number values
	if (mach.GetSA() == AquaSimAddress::ConvertFrom(m_device->GetAddress()))
	{
		// Set the direct distance from this node to the destination
		mac_libra_h.SetDirectDistance(m_distances.find(dst_addr)->second);
		int max_hops_number = CalculateHopCount(mac_libra_h.GetDirectDistance(), m_packet_size, m_max_tx_power);
		uint32_t optimal_distance = mac_libra_h.GetDirectDistance() / (max_hops_number + 1);

//		std::cout << "MAX HOPS NUMBER: " << max_hops_number << "\n";
//		std::cout << "OPTIMAL DISTANCE: " << optimal_distance << "\n";

		mac_libra_h.SetOptimalDistance(optimal_distance);
		mac_libra_h.SetMaxHopsNumber(max_hops_number);
	}

	// Update the forwarding table according to the known distances
	std::map<AquaSimAddress, AquaSimAddress> src_dst_map;
	src_dst_map.insert(std::make_pair(mach.GetSA(), dst_addr));

	if (m_forwarding_table.count(src_dst_map) == 0)
	{
		// Create new entry with initial weight
		std::map<AquaSimAddress, double> m; // {next_hop : weight}

		// For each possible destination / distance - calculate the initial weight based on the optimal distance metric
//			std::cout << "DISTANCES SIZE: " << m_distances.size() << "\n";
		for (auto const& x : m_distances)
		{
			// Calculate initial weight (reward)
			m.insert(std::make_pair(x.first, CalculateReward(mac_libra_h.GetOptimalDistance(), x.second, 0)));
//				std::cout << "Creating new table entry with REWARD: " << reward / 2 << "\n";
			NS_LOG_DEBUG("Creating new table entry with WEIGHT: " << CalculateReward(mac_libra_h.GetOptimalDistance(), x.second, 0));
		}

		m_forwarding_table.insert(std::make_pair(src_dst_map, m));
//			std::cout << "TABLE SIZE: " << m_forwarding_table.size() << "\n";
	}

	// Select next_hop neighbor and send down the packet
	// If the hop_count exceeds the threshold, then send the packet directly to the destination
	AquaSimAddress next_hop_addr;
	if (mac_libra_h.GetHopCount() > mac_libra_h.GetMaxHopsNumber())
	{
//			std::cout << "HOP COUNT EXCEEDED THRESHOLD. Sending to DST.\n";
		next_hop_addr = dst_addr;
	}
	else
	{
		next_hop_addr = SelectNextHop(mach.GetSA(), dst_addr);
	}

	// Set next hop parameters - update src/dst fields
	mac_libra_h.SetSrcAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
//		std::cout << "Next hop addr: " << next_hop_addr << "\n";
	mac_libra_h.SetDstAddr(next_hop_addr);

	// Set the direct distance from this node to the destination
	mac_libra_h.SetDirectDistance(m_distances.find(dst_addr)->second);
//		std::cout << "DIRECT DISTANCE FROM LIST: " << m_distances.find(dst_addr)->second << "\n";

	// Set sender address for the reward
	mac_libra_h.SetSenderAddr(sender_addr);

	// Set tx time and size
	// ash.SetSize(mac_libra_h.GetSerializedSize() + mach.GetSerializedSize() + p->GetSize());
	ash.SetSize(p->GetSize());
	// DO NOT CONSIDER HEADER OVERHEAD IN THE TESTS
	ash.SetTxTime(Phy()->CalcTxTime(p->GetSize()));

	ash.SetErrorFlag(false);
	ash.SetDirection(AquaSimHeader::DOWN);

	// Set tx power for the given packet. The tx_power must be enough to reach the initial sender node.
	// Insert the next_hop distance to each sent packet
	if (AquaSimAddress::ConvertFrom(m_device->GetAddress()) == sender_addr)
	{
		// Set tx_power sufficient to reach the destination
		mac_libra_h.SetTxPower(CalculateTxPower(m_distances.find(next_hop_addr)->second));
		// Add some "guard distance", just to mitigate the initial error in distance calculation from Tx/Rx
		mac_libra_h.SetNextHopDistance(m_distances.find(next_hop_addr)->second + m_dist_error);
	}
	else
	{
		// Else, consider the tx_power to reach the sender node as well
		if ((m_distances.find(next_hop_addr)->second) <= (m_distances.find(sender_addr)->second))
		{
			// Set tx_power to reach the sender
			mac_libra_h.SetTxPower(CalculateTxPower(m_distances.find(sender_addr)->second));
			// Add some "guard distance", just to mitigate the initial error in distance calculation from Tx/Rx
			mac_libra_h.SetNextHopDistance(m_distances.find(sender_addr)->second + m_dist_error);
		}
		else
		{
			// Set tx_power sufficient to reach the destination
			mac_libra_h.SetTxPower(CalculateTxPower(m_distances.find(next_hop_addr)->second));
			// Add some "guard distance", just to mitigate the initial error in distance calculation from Tx/Rx
			mac_libra_h.SetNextHopDistance(m_distances.find(next_hop_addr)->second + m_dist_error);
		}
	}

	p->AddHeader(mac_libra_h);
	p->AddHeader(mach);
	p->AddHeader(ash);

	// Send Frame
	// std::cout << ash.GetTxTime() << "\n";

	// Start sending, if queue is empty
	if (m_send_buffer.size() == 0)
	{
		// Push packet to MAC send queue
		m_send_buffer.push(p->Copy());
		SendPacket();
	}
	else
	{
		m_send_buffer.push(p->Copy());
	}

	return true;
}

// Select next hop based on softmax selection
AquaSimAddress
AquaSimMacLibra::SelectNextHop(AquaSimAddress src_addr, AquaSimAddress dst_addr)
{
	// Forwarding table has the following format: {dst_adddr: {next_hop1 : w1, ... next_hopN : wN}}
	// For the given dst_addr, select a next_hop_addr with max weight (greedy method for now)
	double max_value = 0;
	AquaSimAddress next_hop_addr = 0;

	std::map<AquaSimAddress, AquaSimAddress> src_dst_map;
	src_dst_map.insert(std::make_pair(src_addr, dst_addr));

	// Store the next-hop address and the corresponding selection probability, based on current weights
	std::vector<AquaSimAddress> next_hop_addresses;
	std::vector<double> selection_probabilities;
	
	// Calculate selection probabilities
	// Find weight sum
	double weight_sum = 0;
	for (auto const& x : m_forwarding_table.find(src_dst_map)->second)
	{
		weight_sum += exp(x.second);
	}

	// Store selection probabilities
	// int i = 0;
	for (auto const& x : m_forwarding_table.find(src_dst_map)->second)
	{
		selection_probabilities.push_back(exp(x.second) / weight_sum);
		next_hop_addresses.push_back(x.first);
		// std::cout << "Next hop: " << x.first << "\n";
		// std::cout << "Selection probability: " << selection_probabilities.at(i) << "\n";
		// i++;
	}

    // Select next-hop neighbor based on the softmax probabilities
	uint32_t next_hop_index;
	double point = m_rand->GetValue();
    double cur_cutoff = 0;

    for (uint32_t i=0; i<selection_probabilities.size(); i++) {
		// std::cout << "cur_cutoff: " << cur_cutoff << "\n";
		// std::cout << "selection prob: " << selection_probabilities[i] << "\n";
		// std::cout << "point: " << point << "\n";
        cur_cutoff += selection_probabilities[i];
        if (point < cur_cutoff)
		{
			next_hop_index = i;
			break;
		}
      next_hop_index = selection_probabilities.size()-1;
    }
    
	next_hop_addr = next_hop_addresses[next_hop_index];

// 	// Old greedy method:
// 	// Iterate through the weights to find the max value
// 	for (auto const& x : m_forwarding_table.find(src_dst_map)->second)
// 	{
// 		if (max_value <= x.second)
// 		{
// 			max_value = x.second;
// 			next_hop_addr = x.first;
// 		}
// //		std::cout << x.first << " " << x.second << "\n";
// 	}

//	std::cout << "MAX VALUE: " << max_value << "\n";
	NS_LOG_DEBUG("MAX VALUE: " << max_value);

//	std::cout << "NEXT HOP ADDR: " << next_hop_addr << "\n";
	NS_LOG_DEBUG("NEXT HOP ADDR: " << next_hop_addr);

	return next_hop_addr;
}

// Update weight in forwarding table
bool
AquaSimMacLibra::UpdateWeight(AquaSimAddress src_addr, AquaSimAddress dst_addr, AquaSimAddress next_hop_addr, double reward)
{
//	std::cout << "UPDATED REWARD: " << reward << "\n";
	NS_LOG_DEBUG("UPDATED REWARD: " << reward);

	std::map<AquaSimAddress, AquaSimAddress> src_dst_map;
	src_dst_map.insert(std::make_pair(src_addr, dst_addr));

	if (m_forwarding_table.count(src_dst_map) == 0)
	{
		// Create new entry with initial weight according to given reward
		std::map<AquaSimAddress, double> m; // {next_hop : weight}

		m.insert(std::make_pair(next_hop_addr, reward));
//		std::cout << "Creating new table entry with REWARD: " << reward << "\n";
//		NS_LOG_DEBUG("Creating new table entry with REWARD: " << reward / 2);

		m_forwarding_table.insert(std::make_pair(src_dst_map, m));
	}
	else
	{
		// Check if the next_hop_entry exist, if not - create one and return
		if (m_forwarding_table.find(src_dst_map)->second.count(next_hop_addr) == 0)
		{
			m_forwarding_table.find(src_dst_map)->second.insert(std::make_pair(next_hop_addr, reward));
			return 0;
		}

		double current_weight = m_forwarding_table.find(src_dst_map)->second.find(next_hop_addr)->second;
//		NS_LOG_DEBUG("CURRENT WEIGHT: " << current_weight << " Node Address: " << AquaSimAddress::ConvertFrom(m_device->GetAddress()));
//		NS_LOG_DEBUG("REWARD: " << reward);
//		NS_LOG_DEBUG("TABLE SIZE: " << m_forwarding_table.size());

		// Calculate sample average and update the weight
		m_forwarding_table.find(src_dst_map)->second.at(next_hop_addr) = (current_weight + reward) / 2;
	}
	return 0;
}

double
AquaSimMacLibra::CalculateReward(double optimal_distance, double direct_distance, double current_distance)
{
	// Calculate weight based on the difference between optimal_distance and D(Tx,Rx)
//	std::cout << "DISTANCE DIFFERENCE: " << (direct_distance - current_distance) << "\n";
	if ((direct_distance - current_distance) <= 0)
	{
		// Return minimum possible reward
		return m_min_reward;
	}

	// TODO: Think out how to give more weight to a closer node, then to a more distant one.
	if (optimal_distance <= (direct_distance - current_distance))
	{
		return 100 * (optimal_distance / (direct_distance - current_distance));
	}
	else
	{
		return 100 * ((direct_distance - current_distance) / optimal_distance);
	}
}

double
AquaSimMacLibra::CalculateDistance(double tx_power, double rx_power)
{
	// A very rough approximation of rayleigh model, used in the propagation module:
	// rx_power = tx_power / d^k * alpha^(d/1000), k = 2
	// This approximation works if freq=25kHz, i.e. alpha ~ 4
	NS_LOG_DEBUG("TX_POWER_CALC_DISTANCE: " << tx_power);
	NS_LOG_DEBUG("RX_POWER_CALC_DISTANCE: " << rx_power);
	return sqrt(tx_power / rx_power);
}

void
AquaSimMacLibra::InitializeDistances()
{
	for (uint32_t i=1; i <= m_device->GetChannel()->GetNDevices(); i++)
	{
		if (i != (m_device->GetNode()->GetId() + 1))
		{
//	    		std::cout << "ADDR: " << AquaSimAddress::ConvertFrom(m_device->GetChannel()->GetDevice(i-1)->GetAddress()) << " " << m_device->GetChannel()->GetDevice(i-1)->GetAddress() << "\n";
			m_distances.insert(std::make_pair(AquaSimAddress::ConvertFrom(m_device->GetChannel()->GetDevice(i-1)->GetAddress()),
					CalculateDistance(AquaSimAddress::ConvertFrom(m_device->GetChannel()->GetDevice(i-1)->GetAddress()))));
		}
	}
}

double
AquaSimMacLibra::CalculateDistance(AquaSimAddress dst_addr)
{
	double dist = 0;

	// Get the distance from the mobility model
    Ptr<Object> sObject = m_device->GetNode();
    Ptr<MobilityModel> senderModel = sObject->GetObject<MobilityModel> ();

    Ptr<Object> rObject = m_device->GetChannel()->GetDevice(dst_addr.GetAsInt() - 1)->GetNode();
    Ptr<MobilityModel> recvModel = rObject->GetObject<MobilityModel> ();

    dist = senderModel->GetDistanceFrom(recvModel);

    // std::cout << "MOBILITY DISTANCE: " << dist << "\n";

    return dist;
}

double
AquaSimMacLibra::CalculateTxPower(double d)
{
	  // Calculate Tx power given the distance and expected Rx_threshold
	  // The calculation is based on Rayleight model, used in the aqua-sim-propagation module:
	  // Rx = Tx / (d^k * alpha^(d/1000)), k = 2, alpha = 4.07831, (f = 25kHz)
	// double tx_power = pow(d, 2) * pow(4.07831, (d / 1000)) * (m_rx_threshold + 0.0003); // 0.0003 to adjust the model
	double tx_power = pow(d, 2) * pow(4.07831, (d / 1000)) * m_rx_threshold;
	NS_LOG_DEBUG("GIVEN DISTANCE: " << d);
	NS_LOG_DEBUG("CALCULATED TX POWER: " << tx_power);

	if (tx_power > m_max_tx_power)
	{
		return m_max_tx_power;
	}
	else
	{
		return tx_power;
	}
}

bool
AquaSimMacLibra::IsWithinMaximumRange(AquaSimAddress dst_addr)
{
	if (dst_addr == AquaSimAddress::ConvertFrom(m_device->GetAddress()))
	{
		return false;
	}
	if (CalculateDistance(dst_addr) > m_max_range)
	{
		// std::cout << "THE DST IS OUTSIDE TX RANGE: " << CalculateDistance(dst_addr) << "\n";
		return false;
	}
	else
	{
		return true;
	}	
}

void
AquaSimMacLibra::DiscoverInRangeNodes()
{
	m_isInRange = false;
	for (uint32_t i=1; i <= m_device->GetChannel()->GetNDevices(); i++)
	{
		if (i != (m_device->GetNode()->GetId() + 1))
		{
			AquaSimAddress address = AquaSimAddress::ConvertFrom(m_device->GetChannel()->GetDevice(i-1)->GetAddress());
			if (IsWithinMaximumRange(address))
			{
				// std::cout << "ADDRESS in RANGE: " << address << "\n";
				m_inrange_addresses.push_back(address);
				m_isInRange = true;
			}
		}
	}
}

void AquaSimMacLibra::DoDispose()
{
  NS_LOG_FUNCTION(this);
  AquaSimMac::DoDispose();
}
