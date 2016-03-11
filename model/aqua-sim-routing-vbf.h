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

#ifndef AQUA_SIM_ROUTING_VBF_H
#define AQUA_SIM_ROUTING_VBF_H

#include "aqua-sim-address.h"
#include "aqua-sim-datastructure.h"
#include "ns3/vector.h"

#include <map>

namespace ns3 {

class VBHeader;

struct vbf_neighborhood{
  int number;
  Vector3D neighbor[MAX_NEIGHBOR];
};

typedef std::pair<AquaSimAddress, unsigned int> hash_entry;

class AquaSimPktHashTable {
public:
  std::map<hash_entry,vbf_neighborhood*> m_htable;
  //std::map<hash_t, hash_entry> m_htable;

  AquaSimPktHashTable() {
    m_windowSize=WINDOW_SIZE;
    //  lower_counter=0;
    // 50 items in the hash table, however, because it begins by 0, so, minus 1
    //Tcl_InitHashTable(&m_htable, 3);
  }

  int  m_windowSize;
  void Reset();
  void PutInHash(VBHeader *);
  void PutInHash(VBHeader *, Vector3D *);
  vbf_neighborhood* GetHash(AquaSimAddress senderAddr, unsigned int pkt_num);
//private:
//int lower_counter;
};

//TODO VBF class...


}  // namespace ns3

#endif /* AQUA_SIM_ROUTING_VBF_H */
