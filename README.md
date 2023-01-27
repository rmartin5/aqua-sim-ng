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
This code is meant to be an add-on to current NS-3 software and therefore requires NS-3 and all pre-requirements of said software. Current tested and supported versions of NS-3 include 3.24, 3.26, 3.27, 3.33 and 3.37.

--------------------------------------

## Documentation and Installation
Further documentation, installation steps, example script walk through, and protocol creation guides can be found under [Aqua-Sim NG Documentation](documentation/).

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

--------------------------------------
#### License and Contact

Copyright (c) 2017 UWSN Lab at the University of Connecticut.
All rights reserved.

Robert Martin : <robert.martin@uconn.edu>

Zheng Peng, Ph.D : <zheng@cs.ccny.cuny.edu>
