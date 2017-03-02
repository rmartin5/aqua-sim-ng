/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 University of Washington
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
 */

#ifndef AQUA_SIM_EXAMPLE_H
#define AQUA_SIM_EXAMPLE_H

#include "ns3/network-module.h"
#include "ns3/stats-module.h"
#include "ns3/core-module.h"
#include "ns3/aqua-sim-ng-module.h"

using namespace ns3;

/**
 *
 */
class Experiment
{
public:
  /**
   * Run an experiment across a range of congestion window values.
   *
   * \param uan The Uan stack helper to configure nodes in the model.
   * \return The data set of CW values and measured throughput
   */
  Gnuplot2dDataset Run (AquaSimHelper &ash);
  /**
   * Receive all available packets from a socket.
   *
   * \param socket The receive socket.
   */
  void ReceivePkt (Ptr<Socket> socket);
  void IncreaseTraffic(uint32_t dr);
  void ResetData ();

  uint32_t m_numNodes;                //!< Number of transmitting nodes.
  uint32_t m_dataRate;                //!< DataRate in bps.
  double m_depth;                     //!< Depth of transmitting and sink nodes.
  double m_boundary;                  //!< Size of boundary in meters.
  uint32_t m_packetSize;              //!< Generated packet size in bytes.
  uint32_t m_bytesTotal;              //!< Total bytes received.
  uint32_t m_drMin;                   //!< Min DataRate to simulate.
  uint32_t m_drMax;                   //!< Max DataRate to simulate.
  uint32_t m_drStep;                  //!< DataRate step size, default 10.
  uint32_t m_avgs;                    //!< Number of topologies to test for each cw point.

  Time m_simTime;                     //!< Simulation run time, default 1000 s.

  std::string m_gnudatfile;           //!< Name for GNU Plot output, default uan-cw-example.gpl.
  std::string m_asciitracefile;       //!< Name for ascii trace file, default uan-cw-example.asc.
  std::string m_bhCfgFile;            //!< (Unused)

  Gnuplot2dDataset m_data;            //!< Container for the simulation data.
  std::vector<double> m_throughputs;  //!< Throughput for each run.

  /** Default constructor. */
  Experiment ();
};

#endif /* UAN_CW_EXAMPLE_H */
