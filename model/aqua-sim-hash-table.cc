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

//#include "tclcl.h"
#include "aqua-sim-hash-table.h"

namespace ns3{

NS_OBJECT_ENSURE_REGISTERED(AquaSimHashTable);


TypeId
AquaSimHashTable::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimHashTable")
    .SetParent<Object>()
    .AddConstructor<AquaSimHashTable>()
    ;
  return tid;
}

void
AquaSimHashTable::PutInHash(int addr)
{
  bool exist = false;
  int index = 0;
  for (int i = 0; i < m_currentIndex; i++){
    if (m_table[i].node == addr) {
      index = i;
      exist = true;
    }
  }

  if (exist) m_table[index].num++;
  else {
    m_table[m_currentIndex].node = addr;
    m_table[m_currentIndex].num = 1;
    m_currentIndex++;
  }
}

int
AquaSimHashTable::GetNode(int index)
{
  return m_table[index].node;
}

int
AquaSimHashTable::GetNumber(int index)
{
  return m_table[index].num;
}

}  //namespace ns3
