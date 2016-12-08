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

#ifndef AQUA_SIM_TRACE_READER_H
#define AQUA_SIM_TRACE_READER_H

#include "aqua-sim-channel.h"
#include <string>

namespace ns3 {

struct TraceEntry {
  double time;
  double temp;
  double salinity;
  double noise;
  void Reset() {time=temp=salinity=noise=0.0;}
};

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Channel trace driver. Used for reading in real-system channel traces.
 *    Channel must be using Range Propagation and Contant Noise Generator
 *    Expected input file is structured as following:
 *      -Each line is a single recorded entry.
 *      -Line layout: <Timestamp Temperature Salinity Noise>
 *      -Note: Delimiter is a space ' '
 *      -Expected metrics: <Seconds Celsius PPT dB>, respectivitly
 *
 */
class AquaSimTraceReader
{
public:
  AquaSimTraceReader();
  static TypeId GetTypeId (void);
  bool ReadFile (const std::string& fileName);
  void SetChannel(Ptr<AquaSimChannel> channel);

protected:
  void Initialize();
  void ScheduleComponents(TraceEntry entry);
  void SetComponents(TraceEntry entry);

private:
  Ptr<AquaSimChannel> m_channel;

};  //class AquaSimTraceReader

} // namespace ns3

#endif /* AQUA_SIM_TRACE_READER_H */
