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

#ifndef AQUA_SIM_HASH_TABLE_H
#define AQUA_SIM_HASH_TABLE_H

//#include "config.h"
//#include "tclcl.h"

#include "ns3/object.h"

namespace ns3{

#define TABLE_SIZE 20

struct ValueRecord{
  int node;
  int num;
};

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Basic hash table
 */
class AquaSimHashTable : public Object {
public:
  AquaSimHashTable()
  {
    m_currentIndex = 0;
    for (int i = 0; i < TABLE_SIZE; i++)
    {
      m_table[i].node = -1;
      m_table[i].num = -1;
    }
  }

  static TypeId GetTypeId(void);
  int m_currentIndex;
  int GetCurrentIndex(void) { return m_currentIndex; }
  void PutInHash(int addr);
  int  GetNode(int);
  int  GetNumber(int);
  ValueRecord m_table[TABLE_SIZE];
};

} // namespace ns3

#endif /* AQUA_SIM_HASH_TABLE_H */
