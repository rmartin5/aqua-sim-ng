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

#ifndef AQUA_SIM_DATASTRUCTURE_H
#define AQUA_SIM_DATASTRUCTURE_H

#include "ns3/vector.h"

//#define THIS_NODE             here_
#define JITTER                0.1    //use to generate random
#define DELAY                 1.0
//#define DELAY2                0.01

//#define SEND_MESSAGE(x,y,z)  send_to_dmux(prepare_message(x,y,z), 0)

// mess types
#define INTEREST      1
#define AS_DATA          2    /* Not DATA due to naming conflict. */
#define DATA_READY    3
#define SOURCE_DISCOVERY 4
#define SOURCE_TIMEOUT   5
#define TARGET_DISCOVERY 6
#define TARGET_REQUEST 7
#define SOURCE_DENY  8
#define V_SHIFT 9
#define FLOODING 10 // not used right now
#define DATA_TERMINATION 11
#define BACKPRESSURE 12
#define BACKFLOODING 13// not used right now
#define EXPENSION 14
#define V_SHIFT_DATA 15
#define EXPENSION_DATA 16

// next hop status
//#define UNKNOWN 1 // causes compilation issues
#define FRESHED 2
#define DEAD 3
//#define SUPPRESSED 4



// packet status
#define FORWARDED 3
#define CENTER_FORWARDED 4
#define FLOODED 5
//#define DROPPED 6
#define TERMINATED 7
//#define BACKFORWARDED 8
#define SUPPRESSED 9
#define VOID_SUPPRESSED 10


#define MAX_ATTRIBUTE 3
#define MAX_NEIGHBORS 30
#define MAX_DATA_TYPE 30
#define MAX_NEIGHBOR 10
#define WINDOW_SIZE  19

//used by hash table to limited the maximum length

//#define ROUTING_PORT 255

namespace ns3 {

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Data structures and macros used by Aqua-Sim protocols.
 */
typedef struct RoutingVector{
   Vector start;
   Vector end;
} routing_vector;


struct uw_extra_info {

 // AquaSimAddress osender_id;            // The original sender of this message
 // unsigned int seq;           //  sequence number

Vector o;   // the start point of the forward path
 //double ox;
 //double oy;
 //double oz;

 //AquaSimAddress sender_id;            // The forwarder of this message

Vector f; // the forward's position
 //double fx;
 //double fy;
 //double fz;

Vector t;  // the end point of the forward path
 //double tx;
 //double ty;
 //double tz;

Vector d;  // this is the information about relative position of the receiver
             //   to the forwarder, not include in the header of real packet
 //double dx;
 //double dy;
 //double dz;

};

} // namespace ns3
 #endif  /* AQUA_SIM_DATASTRUCTURE_H */
