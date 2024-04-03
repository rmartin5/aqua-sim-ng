#!/usr/bin/python
"""
Run JMAC on given number of nodes and iterations
"""

import os, sys
import subprocess
import time

# N_NODES = [6, 8, 10, 12, 14, 16, 18, 20, 22, 24]
N_NODES = [6]
N_ITERATIONS = 2
MAC = "aloha"

# NS-3 command line execute
path = "../../../"
cmd = './ns3 run "JmacTest --seed=%s --psize=400 --rate=120 --nodes=%s --mac=%s --sinks=1 --simStop=1000 --center_x=1000 --center_z=1000 --radius=300 --depth=400 --epochTime=10"'

processes = []

def run():
    for n in N_NODES:
        for i in range(N_ITERATIONS):
            p = subprocess.Popen((path+cmd) % (i, n, MAC), shell=True)
            print((path+cmd) % (i, n, MAC))
            processes.append(p)
            time.sleep(2)

            p.wait()
            
        # # Wait till all processes are finished
        # for p in processes:
        #     p.wait()

        # save results to a file
        save_cmd = "mv ../../../jmac_results.txt ../../../results_%s_%s_%s.txt" % (MAC, n, N_ITERATIONS)
        subprocess.Popen(save_cmd, shell=True)
        time.sleep(1)

if __name__ == '__main__':
    run()
