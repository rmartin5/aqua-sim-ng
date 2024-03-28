#!/usr/bin/python
"""
Parse SFAMA ascii trace file from NS-3 Aqua-Sim to extract results from simulation.
"""


import sys
import numpy


# Energy-related pararmeters
TX_POWER = 60.0     # Watts
RX_POWER = 0.158     # Watts
IDLE_POWER = 0.158   # Watts
LINK_SPEED = 10000.0  # bps
PACKET_SIZE = 800 + 512 # bytes // 512 bytes is the header size (see the trace files)


# Parse trace file to tx/rx events
def parse_events(trace_file_path):
    # Store raw tx/rx events, parsed from the original trace file
    EVENTS = []

    with open(trace_file_path, "r") as f:
        # Store tx/rx event list
        event = ""
        i = 0
        for line in f:

            if (line[0] == "t" or line[0] =="r") and (i != 0):

                EVENTS.append(event)

                event = line[:-1]

            else:
                event += line[:-1]

            i += 1

        EVENTS.append(event)
    return EVENTS


def print_events(EVENTS):
    for event in EVENTS:
        print (event)
        print ("")


def parse_fields(EVENTS):
    # TRACE object with parsed fields
    TRACE = {"RX/TX-MODE": [], "TS": [], "NODE_ID": [], "TX_POWER": [], "RX_POWER": [], "TX_TIME": [], "DIRECTION": [],
    "NUM_FORWARDS": [], "ERROR": [], "UNIQUE_ID": [], "PTYPE": [], "PAYLOAD_SIZE": [], "MAC_SRC_ADDR": [], "MAC_DST_ADDR": []}

    # Store per Node information: number of processed RX events, number of collisions
    NODE_VALUES = {"PROCESSED_RX_COUNT": 0, "RX_ENERGY": 0.0, "TX_ENERGY": 0.0, "IDLE_ENERGY": 0.0, "COLLISION_COUNT": 0, "LAST_TS": 0.0}
    #NODE_INFO = {"NODE_ID": {"PROCESSED_RX_COUNT": 0, "RX_ENERGY": 0.0, "TX_ENERGY": 0.0, "IDLE_ENERGY": 0.0, "COLLISION_COUNT": 0, "LAST_TS": 0.0}}
    NODE_INFO = {}
    for i in range(300):
        NODE_INFO[i] = dict(NODE_VALUES)

    # Parse the fields
    for line in EVENTS:

        stripped_line = line.split(" ")
        if line[0] == "t":
            ptype = stripped_line[37].split("=")[1][:-1]
            TRACE["PTYPE"].append(ptype)
            TRACE["RX/TX-MODE"].append("TX")
            ts = float(stripped_line[1])
            TRACE["TS"].append(ts)
            node_id = int(stripped_line[2].split("/")[2])
            TRACE["NODE_ID"].append(node_id)

            TRACE["TX_POWER"].append(TX_POWER)
            header_size = int(stripped_line[17].split("=")[1])
            TRACE["PAYLOAD_SIZE"].append(header_size)

            TRACE["RX_POWER"].append(0)
            TRACE["TX_TIME"].append(float(stripped_line[16].split("=")[1][1:-2]))
            
            TRACE["DIRECTION"].append(stripped_line[18].split("=")[1])
            TRACE["NUM_FORWARDS"].append(int(stripped_line[19].split("=")[1]))
            TRACE["ERROR"].append(stripped_line[20].split("=")[1])
            TRACE["UNIQUE_ID"].append(int(stripped_line[21].split("=")[1]))

            TRACE["MAC_SRC_ADDR"].append(stripped_line[23].split("=")[1])
            TRACE["MAC_DST_ADDR"].append(stripped_line[24].split("=")[1])

            # Update node info
            # We need to add the IDLE power to TX power (node can't consume less than IDLE energy when in TX)
            NODE_INFO[node_id]["TX_ENERGY"] += ((header_size * 8) / LINK_SPEED) * TX_POWER
            NODE_INFO[node_id]["TX_ENERGY"] += ((header_size * 8) / LINK_SPEED) * IDLE_POWER
            if ts >= NODE_INFO[node_id]["LAST_TS"]:
                NODE_INFO[node_id]["IDLE_ENERGY"] += ((ts - NODE_INFO[node_id]["LAST_TS"]) / 1000000000.0) * IDLE_POWER
                NODE_INFO[node_id]["LAST_TS"] = ts + ((header_size * 8) / LINK_SPEED) * 1000000000.0
                # print header_size
            else:
                NODE_INFO[node_id]["COLLISION_COUNT"] += 1
                # print "TX", node_id, header_size, ts, "<--->", NODE_INFO[node_id]["LAST_TS"]


        elif line[0] == "r":
            ptype = stripped_line[29].split("=")[1][:-1]
            TRACE["PTYPE"].append(ptype)
            TRACE["RX/TX-MODE"].append("RX")
            ts = float(stripped_line[1])
            TRACE["TS"].append(ts)
            node_id = int(stripped_line[2].split("/")[2])
            TRACE["NODE_ID"].append(node_id)

            TRACE["TX_POWER"].append(TX_POWER)

            header_size = int(stripped_line[9].split("=")[1])
            TRACE["PAYLOAD_SIZE"].append(header_size)

            TRACE["RX_POWER"].append(0)
            TRACE["TX_TIME"].append(0)
            
            TRACE["DIRECTION"].append(stripped_line[10].split("=")[1])
            TRACE["NUM_FORWARDS"].append(int(stripped_line[11].split("=")[1]))
            TRACE["ERROR"].append(stripped_line[12].split("=")[1])
            TRACE["UNIQUE_ID"].append(int(stripped_line[13].split("=")[1]))

            TRACE["MAC_SRC_ADDR"].append(stripped_line[15].split("=")[1])
            TRACE["MAC_DST_ADDR"].append(stripped_line[16].split("=")[1])

            # Update node info
            # if (ts - (((header_size) * 8) / LINK_SPEED) * 1000000000.0) > NODE_INFO[node_id]["LAST_TS"]:
            if (ts - (((header_size) * 8) / LINK_SPEED) * 1000000000.0) >= NODE_INFO[node_id]["LAST_TS"]:
                NODE_INFO[node_id]["PROCESSED_RX_COUNT"] += 1
                NODE_INFO[node_id]["IDLE_ENERGY"] += ((ts - NODE_INFO[node_id]["LAST_TS"]) / 1000000000.0) * IDLE_POWER
                # print header_size
                # NODE_INFO[node_id]["LAST_TS"] = ts + ((header_size * 8) / LINK_SPEED) * 1000000000.0
                # NODE_INFO[node_id]["LAST_TS"] = ts - (((header_size) * 8) / LINK_SPEED) * 1000000000.0    # Rx event is traced at the end of reception, therefore, we need to find the beginning of Rx
                NODE_INFO[node_id]["LAST_TS"] = ts
                NODE_INFO[node_id]["RX_ENERGY"] += ((header_size * 8) / LINK_SPEED) * RX_POWER
            else:
                NODE_INFO[node_id]["COLLISION_COUNT"] += 1
                # print "RX", node_id, header_size, (ts - (((header_size) * 8) / LINK_SPEED) * 1000000000.0), "<--->", NODE_INFO[node_id]["LAST_TS"]
                # print (NODE_INFO[node_id]["LAST_TS"] - (ts - (((header_size) * 8) / LINK_SPEED) * 1000000000.0)) / 1000000000.

    return TRACE, NODE_INFO


# Print the parsed trace object
def print_trace(TRACE):
    for field in TRACE:
        print ("%s :" % field, TRACE[field])
        print (len(TRACE[field]))
        print ("")


# Calculate number of packets, received by MAC layer AND sent up to the upper layers
def calc_recv_packets(TRACE):
    num_recv_packets = 0
    processed_ids = []

    # Go through the dictionary and find the packets, which MAC_DST_ADDR matches the address of the node.
    # Also, ignore duplicate receptions (if any) by checking UNIQUE_ID field
    for i in range(len(TRACE["TS"])):
        if (int(TRACE["MAC_DST_ADDR"][i]) == TRACE["NODE_ID"][i] + 1) and (TRACE["UNIQUE_ID"][i] not in processed_ids and TRACE["PTYPE"][i] == "SFAMA_DATA"):
            num_recv_packets += 1
            processed_ids.append(TRACE["UNIQUE_ID"][i])

    return num_recv_packets


# Calculate number of actual hops a received packet has traversed (avg hop count)
def calc_hop_count(TRACE):
    return 1.0


# Calculate average end-to-end delay
def calc_delay(TRACE):
    delays = []
    processed_ids = []
    for i in range(len(TRACE["TS"])):
        if (int(TRACE["MAC_DST_ADDR"][i]) == TRACE["NODE_ID"][i] + 1) and (TRACE["UNIQUE_ID"][i] not in processed_ids and TRACE["PTYPE"][i] == "SFAMA_DATA"):
            # Find the timestamp when this packet were initially sent
            for j in range(len(TRACE["TS"])):
                if (TRACE["UNIQUE_ID"][i] == TRACE["UNIQUE_ID"][j] and TRACE["RX/TX-MODE"][j] == "TX"):

                    delays.append((TRACE["TS"][i] - TRACE["TS"][j]) / 1000000000.) # to Seconds


            processed_ids.append(TRACE["UNIQUE_ID"][i])

    return numpy.array(delays).mean()


# Calculate "instantaneous" throuhgput - number of bits received over a second
def calc_isntantaneous_throughput(TRACE):
    processed_ids = []
    # Find the fisrt starting ts
    start_ts = 0.0
    num_recv_packets = 0
    timestamps = []
    throughputs = []
    moving_average = []
    previous_average = 0.0
    k = 1
    for i in range(len(TRACE["TS"])):
        if (int(TRACE["MAC_DST_ADDR"][i]) == TRACE["NODE_ID"][i] + 1) and (TRACE["UNIQUE_ID"][i] not in processed_ids and TRACE["PTYPE"][i] == "SFAMA_DATA"):
            start_ts = float(TRACE["TS"][i])
            num_recv_packets += 1
            break

    # Go through the dictionary and find the packets, which MAC_DST_ADDR matches the address of the node.
    # Also, ignore duplicate receptions (if any) by checking UNIQUE_ID field
    for i in range(1, len(TRACE["TS"])):
        if (int(TRACE["MAC_DST_ADDR"][i]) == TRACE["NODE_ID"][i] + 1) and (TRACE["UNIQUE_ID"][i] not in processed_ids and TRACE["PTYPE"][i] == "SFAMA_DATA"):
            processed_ids.append(TRACE["UNIQUE_ID"][i])
            if (float(TRACE["TS"][i] - start_ts)) < 10000000000.0: # if the time difference between current ts and starting ts is less than 10 seconds
                num_recv_packets += 1
            
            else:
                timestamps.append(float(TRACE["TS"][i] / 1000000000.0))
                current_throughput = (num_recv_packets * PACKET_SIZE * 8) / ((float(TRACE["TS"][i]) - start_ts) / 1000000000.0)
                throughputs.append(current_throughput)
                current_average = previous_average + (current_throughput - previous_average) / k
                moving_average.append(current_average)
                previous_average = current_average
                k += 1
                # inst_throuhgput[float(TRACE["TS"][i] / 1000000000.0)] = (num_recv_packets * PACKET_SIZE * 8) / ((float(TRACE["TS"][i]) - start_ts) / 1000000000.0) # bits
                start_ts = float(TRACE["TS"][i])
                num_recv_packets = 1

    return (timestamps, throughputs, moving_average)


# Calculate number of packets, sent by the source node (not counting the relays)
def calc_sent_packets(TRACE):
    num_sent_packets = 0
    processed_ids = []

    # Go through the dictionary and find the packets, which MAC_SRC_ADDR mathes the address of the node.
    # Also, ignore duplicate receptions (if any) by checking UNIQUE_ID field
    for i in range(len(TRACE["TS"])):
        if (int(TRACE["MAC_SRC_ADDR"][i]) == TRACE["NODE_ID"][i] + 1) and (TRACE["UNIQUE_ID"][i] not in processed_ids and TRACE["PTYPE"][i] == "SFAMA_DATA"):
            num_sent_packets += 1
            processed_ids.append(TRACE["UNIQUE_ID"][i])

    return num_sent_packets


# Calculate total number of TX events
def calc_tx_calls(TRACE):
    return TRACE["RX/TX-MODE"].count("TX")


# Calculate total number of RX events
def calc_rx_calls(TRACE):
    return TRACE["RX/TX-MODE"].count("RX")


# Calculate total energy consumption
def calc_energy_consumption(NODE_INFO):
    total_energy = 0.0

    # print NODE_INFO
    for node in NODE_INFO:
        total_energy += NODE_INFO[node]["RX_ENERGY"]
    	# print "RX_ENERGY: ", NODE_INFO[node]["RX_ENERGY"]
        total_energy += NODE_INFO[node]["TX_ENERGY"]
        # print "TX_ENERGY: ", NODE_INFO[node]["TX_ENERGY"]
        total_energy += NODE_INFO[node]["IDLE_ENERGY"]
        # print "IDLE_ENERGY: ", NODE_INFO[node]["IDLE_ENERGY"]

    # print total_energy
    return total_energy


# Calculate energy per bit (received bit)
def calc_energy_per_bit(NODE_INFO, TRACE):
    total_energy = calc_energy_consumption(NODE_INFO)
    return total_energy / (calc_recv_packets(TRACE) * PACKET_SIZE * 8)


# Calculate total number of collisionis from all nodes
def calc_total_collisions(NODE_INFO):
    collision_count = 0
    for node in NODE_INFO:
        collision_count += NODE_INFO[node]["COLLISION_COUNT"]

    return collision_count


# Throughput calculation
def calc_throughput(TRACE):
    n_recv_packets = calc_recv_packets(TRACE)
    throughput = (n_recv_packets * 8 * PACKET_SIZE) / ((TRACE["TS"][-1] / 1000000000.0))
    return throughput


# PDR
def calc_pdr(TRACE):
    n_recv_packets = calc_recv_packets(TRACE)
    n_sent_packets = calc_sent_packets(TRACE)
    pdr = (float(n_recv_packets) / n_sent_packets)
    return pdr


def main():
    # Parse the trace file to a list of tx/rx events
    TRACE_FILE_PATH = sys.argv[1]
    EVENTS = parse_events(TRACE_FILE_PATH)
    # print_events()
    TRACE, NODE_INFO = parse_fields(EVENTS)
    # print_trace()

    # Calculate number of sent and recevied packets
    n_recv_packets = calc_recv_packets(TRACE)
    print ("Number of received packets: ", n_recv_packets)
    n_sent_packets = calc_sent_packets(TRACE)
    print ("Number of sent packets: ", n_sent_packets)
    print ("Total number of tx calls: ", calc_tx_calls(TRACE))
    print ("Total number of rx calls: ", calc_rx_calls(TRACE))
    print ("Total energy consumption: ", calc_energy_consumption(NODE_INFO))
    print ("Energy per bit: ", calc_energy_per_bit(NODE_INFO, TRACE))
    print ("Throughput: ", calc_throughput(TRACE))
    # print "Throughput per node: ", (n_recv_packets * 8 * TRACE["PAYLOAD_SIZE"][0]) / ((max(TRACE["NODE_ID"]) + 1) * (TRACE["TS"][-1] / 1000000000.0))
    print ("PDR: ", float(n_recv_packets) / n_sent_packets)
    print ("Number of collisions: ", calc_total_collisions(NODE_INFO))
    print ("Instantaneous throuhgput: ", calc_isntantaneous_throughput(TRACE))
    print ("Average hop count: ", calc_hop_count(TRACE))


if __name__ == '__main__':
    main()
