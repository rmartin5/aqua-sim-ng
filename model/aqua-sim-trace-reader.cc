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

#include "aqua-sim-trace-reader.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("AquaSimTraceReader");
NS_OBJECT_ENSURE_REGISTERED (AquaSimTraceReader);

AquaSimTraceReader::AquaSimTraceReader()
{
}

AquaSimTraceReader::~AquaSimTraceReader()
{
  m_channel=0;
}


TypeId
AquaSimTraceReader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::AquaSimTraceReader")
  ;
  return tid;
}

bool
AquaSimTraceReader::ReadFile (const std::string& fileName)
{
  if (m_channel == nullptr) {
    NS_LOG_DEBUG("No channel provided.");
    return false;
  }

  std::ifstream reader;
  reader.open(fileName.c_str());
  if(!reader) {
    NS_LOG_DEBUG("Trace file(" << fileName << ") does exist.");
    return false;
  }

  TraceEntry entry;
  entry.Reset();
  while (reader >> entry.time >> entry.temp >> entry.salinity >> entry.noise) {
    ScheduleComponents(entry);
    entry.Reset();
  }
  return true;
}

void
AquaSimTraceReader::SetChannel(Ptr<AquaSimChannel> channel)
{
  m_channel = channel;
}

void
AquaSimTraceReader::Initialize()
{
  NS_ASSERT(m_channel);
  m_channel->m_prop->Initialize();
  m_channel->m_noiseGen->Initialize();
}

void
AquaSimTraceReader::ScheduleComponents(TraceEntry entry)
{
  Simulator::Schedule(Seconds(entry.time), &AquaSimTraceReader::SetComponents, this, entry);
}

void
AquaSimTraceReader::SetComponents(TraceEntry entry)
{

  m_channel->m_prop->SetTraceValues(entry.temp, entry.salinity, entry.noise);
  m_channel->m_noiseGen->SetNoise(entry.noise);
}
