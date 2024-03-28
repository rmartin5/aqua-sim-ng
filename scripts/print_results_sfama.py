#!/usr/bin/python
"""
Plot the results, obtained from the trace files.
"""


import trace_parser_sfama
import numpy

import os.path


PACKET_SIZE = 800
# N_HOPS = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25]
# N_HOPS = [6, 7, 8, 9, 10]
# N_HOPS = [0, 1]
# N_HOPS = [0, 3, 5, 7, 10]
# NODES = [50, 100, 150, 200, 250]
# NODES = [25, 50, 75, 100, 125]
# NODES = [64, 80, 96, 112, 128]
NODES = [128]

# LAMBDAS = ['0.01', '0.02', '0.03', '0.04', '0.05', '0.06', '0.07', '0.08', '0.09', '0.10']
# LAMBDAS = ['0.01']
# LAMBDAS = ['0.01', '0.20', '0.40', '0.60', '0.80', '1.00']
LAMBDAS = ['0.01']
# LAMBDAS = ['0.01', '0.05', '0.10', '0.15', '0.20', '0.25', '0.30', '0.35', '0.40', '0.45', '0.50', '0.55', '0.60']

METRICS = ["collisions", "energy", "energy_per_bit", "throughput", "pdr", "hop_count"]
# METRICS = ["hop_count"]
METRICS_NAMES = {"collisions": "Total number of collisions", "energy": "Total energy consumption, Joules", "energy_per_bit": "Energy per Bit, Joules", "throughput": "Total network throughput, bps", 
                "pdr": "Packet Delivery Ratio (PDR)", "hop_count": "Average Hop Count"}
VALUES = {"collisions": [], "energy": [], "energy_per_bit": [], "throughput": [], "pdr": [], "hop_count": []}


TRACE_PATH = "../../../"
TRACE_NAME = "sfama-density-trace-%s-%s-0.asc"
PRINT_FILENAME = "sfama-density-%s.txt" # insert number of nodes


def get_converged_stats(trace, l, n_nodes):
    TX_TIME = 800 * 8 / 10000.0    # packet_size * 8 / channel_bps
    RX_POWER = 0.82     # Watts
    
    packet_paths = {}
    attempt_number = 1
    for i in range(len(trace["TS"])):
        if trace["RX/TX-MODE"][i] == "TX" and trace["PTYPE"][i] == "SFAMA_DATA":
            unique_id = trace["UNIQUE_ID"][i]
            hop = (int(trace["MAC_SRC_ADDR"][i]), int(trace["MAC_DST_ADDR"][i]))
            dst_node = int(trace["MAC_DST_ADDR"][i])
            tx_energy = float(trace["TX_POWER"][i]) * TX_TIME
            rx_energy = RX_POWER * TX_TIME
            ts = round(float(trace["TS"][i]) / 1000000000.0, 2)   # turn to seconds

            if unique_id not in packet_paths:
                packet_paths[unique_id] = [[hop], dst_node, (tx_energy + rx_energy), ts, attempt_number]
                attempt_number += 1
            else:
                packet_paths[unique_id][0].append(hop)
                packet_paths[unique_id][2] += (tx_energy + rx_energy)

    # Filter out the packets which have been lost during the transmission - the ones which didn't reach the destination node
    packets_to_remove = []
    for packet in packet_paths:
        if packet_paths[packet][0][-1][1] != packet_paths[packet][1]:
            packets_to_remove.append(packet)

    for packet in packets_to_remove:
        del packet_paths[packet]

    # print packet_paths

    # Store the information how many times a path has been chosen, and its energy consumption
    path_stats = {} # format: {path: [times_used, energy, attempt_number]}
    for packet in packet_paths:
        path = tuple(packet_paths[packet][0])

        if path not in path_stats:
            path_stats[path] = [1, packet_paths[packet][2], packet_paths[packet][3], packet_paths[packet][4]]
        else:
            path_stats[path][0] += 1

    # calculate percentage of the path usage
    total_paths_number = 0
    for path in path_stats:
        total_paths_number += path_stats[path][0]

    for path in path_stats:
        path_stats[path][0] = round(100.0 * path_stats[path][0] / float(total_paths_number), 2)

    # print path_stats
    # Find converged path
    max_usage_percentage = 0
    converged_path = None
    converged_attempts = 0
    converge_time = 0
    converged_path_energy = 0 
    for path in path_stats:
        if path_stats[path][0] > max_usage_percentage:
            max_usage_percentage = path_stats[path][0]
            converged_path = path
            converged_attempts = path_stats[path][3]
            converge_time = path_stats[path][2]
            converged_path_energy = path_stats[path][1]

    # print converged_path
    # print converged_attempts
    # print converge_time
    # print converged_path_energy

    # Check if the converged path is optimal or not
    # Get optimal path from the text file
    optimal_path_filename = "optimal_path-%s-%s" % (l, n_nodes)
    with open(optimal_path_filename, "r") as f:
        raw = f.readlines()

        path_line = raw[-1].split("\t")[-1].split(" ")[:-1]

        optimal_path = []
        for node in path_line:
            optimal_path.append(int(node) + 1)
        
        # print "energy:", raw[-1].split("\t")[-3]
        optimal_path_energy = float(raw[-1].split("\t")[-3])

    converged_path_formatted = []
    for hop in converged_path:
        converged_path_formatted.append(hop[0])
    converged_path_formatted.append(hop[1])

    # print optimal_path
    # print converged_path_formatted
    # print converged_path

    if (optimal_path == converged_path_formatted):
        isOptimal = 1
    else:
        isOptimal = 0

    return converged_attempts, converge_time, isOptimal, converged_path_energy, optimal_path_energy

# Print the results to a file
def print_results():
    # for n_nodes in NODES:
    for l in LAMBDAS:

        file_name = PRINT_FILENAME % l
        if not os.path.exists(file_name):
            f =open(file_name, "a")
            # f.write("Density\tNodesNumber\tLambda\tTxPackets\tRxPackets\tTxCount\tRxCount\tCollisionCount\tTotalEnergyConsumption\tEnergyPerBit\tTotalThroughput\tPDR\tAvgHopCount\tAvgDelay\tConvergedAttempt\tConvergeTime\tIsOptimal?\tEnergyConsumptionConvergedPath\tEnergyConsumptionOptimalPath\tOptimalConvergedRatio\n")
            f.write("Density\tNodesNumber\tLambda\tTxPackets\tRxPackets\tTxCount\tRxCount\tCollisionCount\tTotalEnergyConsumption\tEnergyPerBit\tTotalThroughput\tPDR\n")

        else:
            f =open(file_name, "a")
            
        # line = "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s"
        line = "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s"

        # for l in LAMBDAS:
        for n_nodes in NODES:

            for metric in METRICS:
                VALUES[metric] = []
            x = []

            events = trace_parser_sfama.parse_events((TRACE_PATH + TRACE_NAME) % (l, n_nodes))
            trace, node_info = trace_parser_sfama.parse_fields(events)

            # converged_attempts, converge_time, is_optimal, converged_path_energy, optimal_path_energy = get_converged_stats(trace, l, n_nodes)

            # # Print results to file
            # n_recv_packets = trace_parser_sfama.calc_recv_packets(trace)
            # total_energy = trace_parser_sfama.calc_energy_consumption(node_info)
            # data = (n_nodes/100., n_nodes, l, trace_parser_sfama.calc_sent_packets(trace), n_recv_packets, 
            # trace_parser_sfama.calc_tx_calls(trace), trace_parser_sfama.calc_rx_calls(trace), trace_parser_sfama.calc_total_collisions(node_info), 
            # round(total_energy, 2), round(float(total_energy) / (n_recv_packets * PACKET_SIZE * 8), 4), round(trace_parser_sfama.calc_throughput(trace), 2),
            # round(trace_parser_sfama.calc_pdr(trace), 2), round(trace_parser_sfama.calc_hop_count(trace), 2), round(trace_parser_sfama.calc_delay(trace), 2), 
            # converged_attempts, converge_time, is_optimal, round(converged_path_energy, 4), round(optimal_path_energy, 4), 
            # round(optimal_path_energy/converged_path_energy, 4))

            n_recv_packets = trace_parser_sfama.calc_recv_packets(trace)
            total_energy = trace_parser_sfama.calc_energy_consumption(node_info)
            data = (n_nodes/100., n_nodes, l, trace_parser_sfama.calc_sent_packets(trace), n_recv_packets, 
            trace_parser_sfama.calc_tx_calls(trace), trace_parser_sfama.calc_rx_calls(trace), trace_parser_sfama.calc_total_collisions(node_info), 
            round(total_energy, 2), round(float(total_energy) / (n_recv_packets * PACKET_SIZE * 8), 4), round(trace_parser_sfama.calc_throughput(trace), 2),
            round(trace_parser_sfama.calc_pdr(trace), 2))


            f.write(line % data)
            # f.write(str(trace_parser_sfama.calc_hop_count(trace)))
            f.write("\n")
            f.flush()

        f.close()


def main():
    print_results()


if __name__ == '__main__':
    main()
