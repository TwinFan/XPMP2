XPMP2 Remote Client
=========

**Synchronizing Planes across the Network**

Planes displayed using XPMP2 can by synchronized across the network.
The **XPMP2 Remote Client** needs to run on all remote computers,
receives aircraft data from any number of XPMP2-based plugins anywhere
in the network, and displays the aircraft using the same CSL model,
at the same world location with the same animation dataRefs.

This supports usage in setups like
- [Networking Multiple Computers for Multiple Displays](https://x-plane.com/manuals/desktop/#networkingmultiplecomputersformultipledisplays),
- [Networked Multiplayer](https://x-plane.com/manuals/desktop/#networkedmultiplayer),
- but also just locally as a **TCAS Concentrator**, which collects plane
  information from other local plugins and forwards them combined to TCAS
  and classic multiplayer dataRefs for 3rd party add-ons.

For end user documentation refer to
[LiveTraffic/XPMP2 Remote Client](https://twinfan.gitbook.io/livetraffic/setup/installation/xpmp2-remote-client)
documentation.

For more background information refer to
[GitHub XPMP2 documentation](https://twinfan.github.io/XPMP2/Remote.html).

One of many possible setups:
![XPMP2 in a Standard Networked Setup](../docs/pic/XPMP2_Remote_Standard.png)
