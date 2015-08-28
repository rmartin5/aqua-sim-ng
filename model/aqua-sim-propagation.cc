/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 UWSN Lab at the University of Connecticut
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


#include "ns3/nstime.h"
#include "ns3/log.h"

#include "aqua-sim-propagation.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AquaSimPropagation");
NS_OBJECT_ENSURE_REGISTERED (AquaSimPropagation);

TypeId
AquaSimPropagation::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::AquaSimPropagation")
    .SetParent<Object>()
  ;
  return tid;
}

AquaSimPropagation::AquaSimPropagation ()
{
}

AquaSimPropagation::~AquaSimPropagation ()
{
}

}  // namespace n3
