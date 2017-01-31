# Aqua-Sim Next Generation

-------------------------------------

## About

Aqua-Sim is a underwater network simulator which supports a vast amount of protocols and features. Originally developed on the basis of [NS-2](http://www.isi.edu/nsnam/ns/), Aqua-Sim can effectively simulate acoustic signal attenuation and packet collisions in underwater sensor networks (UWSN). Moreover, Aqua-Sim supports three-dimensional deployment. This work consists of rewriting and porting [Aqua-Sim](http://uwsn.engr.uconn.edu/wiki/index.php?title=Aqua-Sim&redirect=no) to [NS-3](http://www.nsnam.org) to improve upon current simulation issues such as memory leakage and usage simplicity.

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

--------------------------------------

## Documentation and Installation
Further documentation, installation steps, example script walk through, and protocol creation guides can be found under [Aqua-Sim NG Documentation](documentation/).

![Aqua-Sim NG Diagram](/documentation/asDiagram.png "Aqua-Sim NG Diagram")

--------------------------------------
## Ongoing Work

1. NS3 Architecture Support
  * ~~_Core Aqua-Sim revamp_~~
  * ~~_Example/helper scripts_~~

2. Protocol Port
  * ~~_Extensive protocol port_~~

3. Real-World Features
  * ~~_Synchronization support_~~
  * ~~_Localization support_~~
  * ~~_Busy terminal model_~~
  * ~~_Fluctuating SNR_~~
  * ~~_Enhanced channel model support_~~
  * ~~_Transmission range uncertainty module_~~
  * ~~_Security features_~~
  * ~~_Trace driven support_~~

4. Information-Centric Integration
  * ~~_Adapted Named Data Network components_~~
  * ~~_Helper/test scripts_~~
  * ~~_Protocol integration_~~



Current protcol support includes: (MAC layer:) BroadcastMAC, Underwater ALOHA, CopeMAC, Underwater FAMA, Underwater Slotted FAMA, GOAL, UWAN, RMac, Tmac, (Routing Layer:) DBR, Static, Dynamic, Flooding, VBF, and VBVA. Furthermore, Aqua-Sim NG core continues to support basic underwater functions such as half-duplex, long propagation delays, and acoustic signal attenuation.

### Enhanced Channel Models
Supporting additional propagation models such as [Bellhop](http://oalib.hlsresearch.com/Rays/) ray tracing model for acoustic communications. Our goal here is to depict better accuracy to real world scenarios in our simulation results.

### Trace Driven Simulation
By implementing a trace driven simulation we will be able to control certain conditions throughout iterations of testing. An example of this could be applying channel readings from an ocean test directly into the simulator and depicting the given parameters throughout each test. This can be beneficial in cases such as ensuring channel conditions remain the same during protocol testing. While this channel consistency is not guaranteed in real world scenarios, it does add additional control when running simulations.

### Synchronization and Localization
Synchronization and localization are two very important conditions which are necessary in certain UWSN protocols. Past Aqua-Sim work does not support a dedicated module for these two features which places the burden instead on the developer. For this work, we plan to incorporate features such as range-based or range-free localization scheme support. For time synchronization, we plan to incorporate features to better integrate clock-skew and offset.

### Real System Features
Real system features is an encompassing category which will better mimic the scenarios seen in UWSNs. Due to the uncertainty of channel conditions, we implement link asymmetry and heterogeneity. This allows the channel to simulate conditions such as additional external noise affecting the overall signal-to-noise ratio for a specified or randomized period of time. Furthermore, these environmental factors in UWSNs may lead to acoustic transmission range uncertainty, which must be taken into account for more accurate results. Another unique feature which we see in UWSNs is the [busy terminal problem](http://dl.acm.org/citation.cfm?id=2674593). The two main factors contributing to this problem are acoustic communication speed and the half-duplex nature of UWSNs. By demonstrating this in Aqua-Sim: Next Generation we can further strengthen our simulation of a real system.

### Security Features
Like any other network type, UWSN are susceptible to attacks. By implementing jamming, denial of service, packet spoofing, sinkhole and wormhole components, we can assist simulating potential underwater scenarios. Our goal is to allow easier integration of these types of attacks by maliciously created nodes and easier analysis support.

### Information-Centric Integration
We plan on implementing adapted Named Data Networking (NDN) techniques alongside specialized NDN protocols for UWSNs. By implementing these features, our hope is to better depict and evaluate the improvements NDN can have in this environment. This integration will consist of introducing interest and data structured packets, Pending Interest Table, Forwarding Information Base, and Context Storage for overall inclusion that is specialized for underwater. Additionally, we will include protocols which handle NDN components to better illustrate this new integration alongside helper and test scripts.

--------------------------------------
#### License

Copyright (c) 2017 UWSN Lab at the University of Connecticut.
All rights reserved.

Robert Martin : <robert.martin@uconn.edu>
