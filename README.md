## Aqua-Sim Next Generation

-------------------------------------

#### About

Aqua-Sim is a underwater network simulator which supports a vast amount of protocols and features. Originally developed on the basis of [NS-2](http://www.isi.edu/nsnam/ns/), Aqua-Sim can effectively simulate acoustic signal attenuation and packet collisions in underwater sensor networks (UWSN). Moreover, Aqua-Sim supports three-dimensional deployment. This work consists of rewriting and porting [Aqua-Sim](http://uwsn.engr.uconn.edu/wiki/index.php?title=Aqua-Sim&redirect=no) from [NS-3](http://www.nsnam.org) to improve upon current simulation issues such as memory leakage and usage simplicity.

Work supported by the UWSN Lab at University of Connecticut.

--------------------------------------
#### Advantages of Aqua-Sim
- Discrete-event driven network simulator
- Support 3D networks and mobile networks
- Simulate underwater acoustic channels with high fidelity
- Implement a complete protocol stack from physical layer to application layer

--------------------------------------

#### Requirements
This code is meant to be as a add-on to current NS3 software and therefore requires NS-3 and all pre-requirements of said software. Current work of Aqua-Sim Next Generation is in very early stages and therefore will be extremely buggy (so use with caution). 

--------------------------------------

#### Ongoing Work
Current support includes a basic example and helper class, as well as base protocol stack support. Extensive protocol support is planned in future iterations. 

The protocol port will be focused on transferring MAC and routing protocols, from past Aqua-Sim work, such as R-MAC, UW-ALOHA, Broadcast-MAC, VBF, and DBR. Furthermore, we will continue supporting the core Aqua-Sim implemented functions such as half-duplex, long propagation delays, and acoustic signal attenuation. 

###### Enhanced Channel Models
Supporting additional propagation models such as [Bellhop](http://oalib.hlsresearch.com/Rays/) ray tracing model for acoustic communications. Our goal here is to depict better accuracy to real world scenarios in our simulation results.

###### Trace Driven Simulation
By implementing a trace driven simulation we will be able to control certain conditions throughout iterations of testing. An example of this could be applying channel readings from an ocean test directly into the simulator and depicting the given parameters throughout each test. This can be beneficial in cases such as ensuring channel conditions remain the same during protocol testing. While this channel consistency is not guaranteed in real world scenarios, it does add additional control when running simulations.

###### Synchronization and Localization
Synchronization and localization are two very important conditions which are necessary in certain UWSN protocols. Past Aqua-Sim work does not support a dedicated module for these two features which places the burden instead on the developer. For this work, we plan to incorporate features such as range-based or range-free localization scheme support. For time synchronization, we plan to incorporate features to better integrate clock-skew and offset.

###### Real System Features
Real system features is an encompassing category which will better mimic the scenarios seen in UWSNs. Due to the uncertainty of channel conditions, we implement link asymmetry and heterogeneity. This allows the channel to simulate conditions such as additional external noise affecting the overall signal-to-noise ratio for a specified or randomized period of time. Furthermore, these environmental factors in UWSNs may lead to acoustic transmission range uncertainty, which must be taken into account for more accurate results. Another unique feature which we see in UWSNs is the [busy terminal problem](http://dl.acm.org/citation.cfm?id=2674593). The two main factors contributing to this problem are acoustic communication speed and the half-duplex nature of UWSNs. By demonstrating this in Aqua-Sim: Next Generation we can further strengthen our simulation of a real system.

--------------------------------------
#### License

Copyright (c) 2016 UWSN Lab at the University of Connecticut.
All rights reserved.

Robert Martin : <robert.martin@uconn.edu>
