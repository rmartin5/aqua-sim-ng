# Aqua-Sim Next Generation

-------------------------------------

## About

Aqua-Sim is a underwater network simulator which supports a vast amount of protocols and features. Originally developed on the basis of [NS-2](http://www.isi.edu/nsnam/ns/), Aqua-Sim can effectively simulate acoustic signal attenuation and packet collisions in underwater sensor networks (UWSN). Moreover, Aqua-Sim supports three-dimensional deployment. This work consists of rewriting and porting [Aqua-Sim](http://obinet.engr.uconn.edu/wiki/index.php/Aqua-Sim) to [NS-3](http://www.nsnam.org) to improve upon current simulation issues such as memory leakage and usage simplicity.

Work supported by the UWSN Lab at University of Connecticut.

--------------------------------------
## Advantages of Aqua-Sim
- Discrete-event driven network simulator
- Support 3D networks and mobile networks
- Simulate underwater acoustic channels with high fidelity
- Implement a complete protocol stack from physical layer to application layer

--------------------------------------

## Requirements
This code is meant to be an add-on to current NS-3 software and therefore requires NS-3 and all pre-requirements of said software.

Current recommended NS-3 version is `3.40`. The following system requirements must be met:

- `gcc-11`, `g++-11` or higher for Linux
- `clang` version `15.0.0` for Mac OS
- `cmake` version `3.16` or higher


--------------------------------------

## Documentation and Installation

To build current version of `aqua-sim-ng`, please follow these steps:

- download and install `ns-3.40`:

```
mkdir workspace
cd workspace
wget https://www.nsnam.org/releases/ns-allinone-3.40.tar.bz2
tar xvf ns-allinone-3.40.tar.bz2
cd ns-allinone-3.40/ns-3.40/
./ns3 clean
./ns3 configure --build-profile=debug --enable-examples --enable-tests --disable-python --disable-werror
./ns3 build
```

- after the initial NS-3 build, copy `aqua-sim-ng` code into `src/` folder:

```
cd src/
git clone https://github.com/rmartin5/aqua-sim-ng.git 
```

- reconfigure ns-3 project and build the new module:

```
cd ..
./ns3 configure --build-profile=debug --enable-examples --enable-tests --disable-python --disable-werror
./ns3 build
```

After that, you should be able to run example scripts located under `src/aqua-sim-ng/examples/` folder.

--------------------------------------
## Running Examples

### LIBRA MAC protocol example

LIBRA MAC protocol is a MAC protocol for UWSNs with multi-hop transmission range control capabilities, powered by Reinforcement Learning for adaptive route selection. More description can be found in the paper [1] (See `References` section down below).

To run LIBRA simulations, execute `libra_grid_test.cc` script from `examples/` folder by running:

```
./ns3 run "LibraGridTest --lambda=0.01 --packet_size=800 --grid_size=10000 --range=3000 --n_nodes=128 --tx_power=60 --simStop=10000 --RngRun=0"
```
where:

`LibraGridTest`: name of the simulation script that refers to `libra_grid_test.cc`

`--lambda`: application traffic rate, following Poisson distribution, pkts/sec

`--packet_size`: user packet size, bytes

`--grid_size`: size of the node allocation area, X by X meters

`--range`: maximum transmission range, meters

`--n_nodes`: number of nodes to allocate

`--tx_power`: maximum Tx power, Watts

`--simStop`: total simulation time, seconds

`--RngRun`: seed for random generator

After successful run, the script should generate a simulation trace-file with the following name: `libra-density-trace-0.01-128-0.asc`

The trace-file contains raw simulation logs with every Tx/Rx event traced in a specific format. To extract useful data from the trace-file, such as network throughput, PDR, and more, please refer to `print_results_libra.py` script, located under `scripts/` folder.

After executing the script, the parsed `.txt` file should be generated that will contain network metrics. An example of the parsed `libra-density-0.01.txt` content is the following:

```
Density	NodesNumber	Lambda	TxPackets	RxPackets	TxCount	RxCount	CollisionCount	TotalEnergyConsumption	EnergyPerBit	TotalThroughput	PDR	AvgHopCount
1.28	128	0.01	12871	8936	22730	152065	174667	69036.9	0.0012	5709733134989.97	0.69	1.89
d
```

### ALOHA and SFAMA simulations

Similar to `LIBRA` simulations described above, the following scripts are available for `ALOHA` and `SFAMA` protocols: `aloha_grid_test.cc` and `sfama_grid_test.cc`.

The simulation commands are similar to `LIBRA` as well:

```
./ns3 run "AlohaGridTest --lambda=0.01 --packet_size=800 --grid_size=10000 --range=3000 --n_nodes=128 --tx_power=60 --simStop=10000 --RngRun=0"
```

and

```
./ns3 run "SfamaGridTest --lambda=0.01 --packet_size=800 --grid_size=10000 --range=3000 --n_nodes=128 --tx_power=60 --simStop=10000 --RngRun=0"
```

To process raw trace-files after simulations, similar Python-scripts are available: `print_results_aloha.py` and `print_results_sfama.py`.


--------------------------------------
## Legacy Documentation

Older tested and supported versions of NS-3 included 3.24, 3.26, 3.27, 3.33 and 3.37. Older documentation, installation steps, example script walk through, and protocol creation guides can be found under [Aqua-Sim NG Documentation](documentation/).

![Aqua-Sim NG Diagram](/documentation/asDiagram.png "Aqua-Sim NG Diagram")

--------------------------------------
## New Modules and Features

Full publication of Aqua-Sim NG is currenlty underway and earlier versions of this paper will be available soon.

Aqua-Sim NG core continues to support basic underwater functions seen in Aqua-Sim 1.0, such as half-duplex, long propagation delays, and acoustic signal attenuation. Current protocol support includes: (MAC layer:) BroadcastMAC, Underwater ALOHA, CopeMAC, Underwater FAMA, Underwater Slotted FAMA, GOAL, UWAN, RMac, Tmac, (Routing Layer:) DBR, Residual-DBR, Static, Dynamic, Dummy, Flooding, DDoS-Restriction, VBF, and VBVA.

1. NS3 Architecture Support
   * Core Aqua-Sim revamp and API integration
   * Example and helper scripts addition

2. Protocol Port
   * Extensive protocol port

3. Real-World Features
   * Synchronization support
   * Localization support
   * Busy terminal model integration
   * Fluctuating SNR
   * Enhanced channel model support
   * Transmission range uncertainty module
   * Security features and module support
   * Trace driven support for channel layer

4. Information-Centric Integration
   * Adapted Named Data Network components
   * Helper and test scripts integrated
   * Specialized protocols addition

### Enhanced Channel Models
Expanded channel models to support multi-channel simulation, differentiated noise generator classes, and additional propagation model support. Our goal here is to depict better accuracy to real world scenarios in our simulation results.

### Trace Driven Simulation
By implementing a trace driven simulation we are able to control certain conditions throughout iterations of testing. An example of this could be applying channel readings from an ocean test directly into the simulator and depicting the given parameters throughout each test. This can be beneficial in cases such as ensuring channel conditions remain the same during protocol testing. While this channel consistency is not guaranteed in real world scenarios, it does add additional control when running simulations.

### Synchronization and Localization
Synchronization and localization are two critical components for ensuring accurate underwater simulation. Past Aqua-Sim work does not support a dedicated module for these two features which places the burden instead on the developer. For this work, we incorporate location features such as range-based or range-free localization support. For time synchronization, we incorporate features to better integrate clock-skew and offset. The goal for these features is to allow for easier protocol integration for users and developers alike.

### Real System Features
Real system features is an encompassing category which will better mimic the scenarios seen in UWSNs. Due to the uncertainty of channel conditions, we implement link asymmetry and heterogeneity. This allows the channel to simulate conditions such as fluctuating signal-to-noise ratios. Furthermore, these environmental factors in UWSNs may lead to acoustic transmission range uncertainty, which must be taken into account for more accurate results. Another unique feature which we see in UWSNs is the [busy terminal problem](http://dl.acm.org/citation.cfm?id=2674593). The two main factors contributing to this busy terminal problem are acoustic communication speed and the half-duplex nature of UWSNs. By demonstrating these components in Aqua-Sim Next Generation we can further strengthen our simulation of a real system.

### Security Features
Like any other network type, UWSN are susceptible to attacks. By implementing jamming, denial of service, packet spoofing, sinkhole and wormhole components, we can assist simulating potential underwater security threats. Our goal is to allow easier integration of these types of attacks by maliciously created nodes and easier analysis support.

### Information-Centric Integration
We implement adapted Named Data Networking (NDN) techniques alongside specialized NDN protocols for UWSNs. By implementing these features, we can better depict and evaluate the improvements NDN can have in this environment. This integration consists of introducing interest and data structured packets, Pending Interest Tables, Forwarding Information Bases, and Context Storages for overall NDN inclusion that is specialized for underwater. Additionally, we implement this module through a packet demux on the physical layer, allowing for protocols to implement certain NDN features without completely revamping how our simulator functions.

## References

[1] Dugaev, D.; Peng, Z.; Luo, Y.; Pu, L. Reinforcement-Learning Based Dynamic Transmission Range Adjustment in Medium Access Control for Underwater Wireless Sensor Networks. Electronics 2020, 9, 1727. https://doi.org/10.3390/electronics9101727


--------------------------------------
#### License and Contact

Copyright (c) 2017 UWSN Lab at the University of Connecticut.
All rights reserved.

Robert Martin : <robert.martin@uconn.edu>

Zheng Peng, Ph.D : <zheng@cs.ccny.cuny.edu>

Dmitrii Dugaev : <ddugaev@gradcenter.cuny.edu>
