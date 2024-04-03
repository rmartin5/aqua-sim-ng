#!/usr/bin/python
"""
Run TRUMAC/TDMA/ALOHA on given number of nodes and iterations
"""

import os, sys
import subprocess
import time

# N_NODES = [6, 8, 10, 12, 14, 16, 18, 20, 22, 24]
# N_NODES = [2, 4, 8, 12, 16, 20, 24, 28, 32, 36]
N_NODES = [2, 4, 8]
# N_NODES = [20, 60, 100, 140, 180, 220, 260]
# DIST = [100, 200, 300, 500, 1000]
N_ITERATIONS = 10
MAC = "trumac"      # trumac, aloha, tdma
ALGO = 0            # for trumac only: 0 - random selection; 1 - nearest neighbor (greedy)
rate = 120
dist = 100 # meters

# NS-3 command line execute
path = "../../../"
cmd = './ns3 run "TrumacTest --seed=%s --psize=100 --nodes=%s --simStop=1000 --mac=%s --rate=%s --dist=%s --algo=%s"'

processes = []

def run():
    for n in N_NODES:
    # for d in DIST:
        for i in range(N_ITERATIONS):
            # p = subprocess.Popen((path+cmd) % (i, n, MAC, rate), shell=True)
            p = subprocess.Popen((path+cmd) % (i, n, MAC, rate, dist, ALGO), shell=True)
            # print (path+cmd) % (i, 2, MAC, rate, d)
            processes.append(p)

            time.sleep(0.2)
            # p.wait()
            
        # Wait till all processes are finished
        for p in processes:
            p.wait()
            time.sleep(0.2)

        # save results to a file
        # save_cmd = "mv ../../../results_trumac.txt ../../../results_trumac_%s_%s_%s_%s.txt" % (MAC, rate, n, N_ITERATIONS)
        save_cmd = "mv ../../../results_trumac.txt ../../../results_%s_%s_%s.txt" % (MAC, n, ALGO)
        subprocess.Popen(save_cmd, shell=True)
        time.sleep(1)

if __name__ == '__main__':
    run()
