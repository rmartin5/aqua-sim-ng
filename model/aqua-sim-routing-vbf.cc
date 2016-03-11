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

#include "aqua-sim-routing-vbf.h"
#include "aqua-sim-header-routing.h"
#include "ns3/log.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimVBF");
//NS_OBJECT_ENSURE_REGISTERED(AquaSimPktHashTable);


void
AquaSimPktHashTable::Reset()
{
  m_htable.clear();
  /*
	vbf_neighborhood *hashPtr;
	Tcl_HashEntry *entryPtr;
	Tcl_HashSearch searchPtr;

	entryPtr = Tcl_FirstHashEntry(&m_htable, &searchPtr);
	while (entryPtr != NULL) {
		hashPtr = (vbf_neighborhood *)Tcl_GetHashValue(entryPtr);
		delete hashPtr;
		Tcl_DeleteHashEntry(entryPtr);
		entryPtr = Tcl_NextHashEntry(&searchPtr);
	}
  */
}

vbf_neighborhood*
AquaSimPktHashTable::GetHash(AquaSimAddress senderAddr, unsigned int pk_num)
{
  hash_entry entry = std::make_pair (senderAddr,pk_num);
  std::map<hash_entry,vbf_neighborhood*>::iterator it;

  it = m_htable.find(entry);

  if (it == m_htable.end())
    return NULL;

  return it->second;
  /*
  unsigned int key[3];

	key[0] = senderAddr;
	key[1] = 0; //sender_id.port_;
	key[2] = pk_num;

	Tcl_HashEntry *entryPtr = Tcl_FindHashEntry(&m_htable, (char *)key);

	if (entryPtr == NULL )
		return NULL;

	return (vbf_neighborhood *)Tcl_GetHashValue(entryPtr);
  */
}

void
AquaSimPktHashTable::PutInHash(VBHeader * vbh)
{
	//Tcl_HashEntry *entryPtr;
	// Pkt_Hash_Entry    *hashPtr;
	vbf_neighborhood* hashPtr;
  AquaSimAddress addr;
  unsigned int pkNum;
  //unsigned int key[3];
	bool newPtr = true;

	addr=vbh->GetSenderAddr();
	//key[1]=0; //(vbh->sender_id).port_;
	pkNum=vbh->GetPkNum();
  hash_entry entry = std::make_pair (addr,pkNum);
  std::map<hash_entry,vbf_neighborhood*>::iterator it;

	int k=pkNum-m_windowSize;
	if(k>0)
	{
		for (int i=0; i<k; i++)
		{
			pkNum=i;
      if(m_htable.count(entry)>0)
      {
        it = m_htable.find(entry);
        hashPtr = it->second;
        newPtr = false;
        m_htable.erase(it);
      }
		}
	}

	pkNum=vbh->GetPkNum();
  //entryPtr = Tcl_CreateHashEntry(&m_htable, (char *)key, &newPtr);
	if (!newPtr) {
		//hashPtr=GetHash(vbh->GetSenderAddr(),vbh->GetPkNum());
		int m=hashPtr->number;
		if (m<MAX_NEIGHBOR) {
			hashPtr->number++;
			hashPtr->neighbor[m].x=0;
			hashPtr->neighbor[m].y=0;
			hashPtr->neighbor[m].z=0;
		}
		return;
	}
	hashPtr=new vbf_neighborhood[1];
	hashPtr[0].number=1;
	hashPtr[0].neighbor[0].x=0;
	hashPtr[0].neighbor[0].y=0;
	hashPtr[0].neighbor[0].z=0;
  m_htable.insert(std::pair<hash_entry,vbf_neighborhood*>(entry,hashPtr));
	//Tcl_SetHashValue(entryPtr, hashPtr);
}

void
AquaSimPktHashTable::PutInHash(VBHeader * vbh, Vector3D* p)
{
	//Tcl_HashEntry *entryPtr;
	// Pkt_Hash_Entry    *hashPtr;
	vbf_neighborhood* hashPtr;
  AquaSimAddress addr;
  unsigned int pkNum;
	//unsigned int key[3];
	bool newPtr = true;

	addr=vbh->GetSenderAddr();
	//key[1]=0; //(vbh->sender_id).port_;
	pkNum=vbh->GetPkNum();
  hash_entry entry = std::make_pair (addr,pkNum);
  std::map<hash_entry,vbf_neighborhood*>::iterator it;

	int k=pkNum-m_windowSize;
	if(k>0)
	{
		for (int i=0; i<k; i++)
		{
			pkNum=i;
      if(m_htable.count(entry)>0)
      {
        it = m_htable.find(entry);
        hashPtr = it->second;
        newPtr = false;
        m_htable.erase(it);
      }
		}
	}

  pkNum=vbh->GetPkNum();
	//entryPtr = Tcl_CreateHashEntry(&m_htable, (char *)key, &newPtr);
	if (!newPtr)
	{
		//hashPtr=GetHash(vbh->GetSenderAddr(),vbh->GetPkNum());
		int m=hashPtr->number;
		// printf("hash_table: this is not old item, there are %d item inside\n",m);
		if (m<MAX_NEIGHBOR) {
			hashPtr->number++;
			hashPtr->neighbor[m].x=p->x;
			hashPtr->neighbor[m].y=p->y;
			hashPtr->neighbor[m].z=p->z;
		}
		return;
	}
	hashPtr=new vbf_neighborhood[1];
	hashPtr[0].number=1;
	hashPtr[0].neighbor[0].x=p->x;
	hashPtr[0].neighbor[0].y=p->y;
	hashPtr[0].neighbor[0].z=p->z;
  m_htable.insert(std::pair<hash_entry,vbf_neighborhood*>(entry,hashPtr));
	//Tcl_SetHashValue(entryPtr, hashPtr);
}

//NS_OBJECT_ENSURE_REGISTERED(AquaSimVBF);
//TODO
