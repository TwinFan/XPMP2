XPMP2 - Developer's documentation
===

These pages, available as [GitHub pages](https://twinfan.github.io/XPMP2/),
document both usage and internals of the XPMP2 library.

**This is work in progress!**

- [X] Doxgen-generated [code documentation](html/index.html)
- [X] [File format definition](XSBAircraftFormat.html) for `xsb_aircraft.txt`
- [X] [CSL model dataRefs supported by XPMP2](CSLdataRefs.html)
- [ ] Provide some general "how it works" background, can base on kuronekos wiki (but shorter) and on some of [my additions](https://github.com/TwinFan/libxplanemp/wiki#changes-to-multiplayeraitcas-handling)
    - [ ] Startup sequence
    - [X] [Model Matching](Matching.html)
    - [X] [TCAS Target support](TCAS.html)
    - [X] [AI/multiplayer support](TCAS.html)
- [X] [What to ship](Deploying.html) so that it works

## Requirements ##

- XPMP2 implements [instancing](https://developer.x-plane.com/sdk/XPLMInstance/),
  so it **requires X-Plane 11.10** or later
- TCAS is provided using [TCAS Override](https://developer.x-plane.com/article/overriding-tcas-and-providing-traffic-information/),
  which became available with **X-Plane 11.50 Beta 8** only.
  In earlier versions TCAS cannot be activated and the error message
  "TCAS requires XP11.50b8 or later" is returned.
- CSL models in **OBJ8 format**

## Feature Details ##

- [Model Matching](Matching.html)
- [TCAS and AI/Multiplayer support](TCAS.html)

## Coding, Building, Deployment ##

These aspects are relevant for developers using XPMP2 in their own plugin:

### How to Use XPMP2 ###

This part will probably need more attention, but find some [first
information here](HowTo.html) and study the working sample plugin
in the `XPMP2-Sample` folder.

### API and Code Documentation ###

All header (and code) files are documented using
[Doxygen](http://www.doxygen.nl/)-style comments.
The generated doxygen files are checked in, too, so that the are available
online:

- [Main Page](html/index.html)
- [XPMPMultiplayer.h](html/XPMPMultiplayer_8h.html) -
  Initialisation and general control functions
- [XPMPAircraft.h](html/XPMPAircraft_8h.html) -
  Defines the main class
  [XPMP2::Aircraft](html/classXPMP2_1_1Aircraft.html),
  which represents an aircraft. **Subclass this class in your plugin!**

### Backward Compatibility ###

If you are familiar with the original `libxplanemp` and are using it already
in your plugins, then you will find
[these information on backward compatibility](BackwardsCompatibility.md)
useful, which explain how you can replace `libxplanemp` with XPMP2
with limited effort.

### Building XPMP2

XCode projects, Visual Studio solutions, and a Docker environment for
Linux and Mac OS builds are provided. The details are
[documented here](Building.html).

### Deploying Your Plugin ###

Apart from the binaries that you build you will need to ship the files provided
here in the 'Resources' folder. Also, users will need to install CSL models.
Find [more details here](Deploying.html).


## CSL Models and Packages ##

These aspects are relevant for CSL model developers and package providers:

### `xsb_aircraft.txt` File Format

The `xsb_aircraft.txt` file defines the content of one CSL model packages.
It´s format is [define here](XSBAircraftFormat.html).

Links to outside locations:
--

[TCAS Override approach](https://developer.x-plane.com/article/overriding-tcas-and-providing-traffic-information/)
explains how TCAS information is provided, the classic multiplayer dataRefs are maintained
and how 3rd party plugins can access this information. XPMP2 publishes data
via the `sim/cockpit2/tcas/targets` dataRefs.

Original libxplanemp:
- [TwinFan's libxplanemp fork](https://github.com/TwinFan/libxplanemp) on GitHub
    - [wiki explaining differences to the kuroneko fork](https://github.com/TwinFan/libxplanemp/wiki)
- [kuroneko's fork](https://github.com/kuroneko/libxplanemp) on GitHub, a long-time standard and basis of other important forks
    - [kuroneko's wiki](https://github.com/kuroneko/libxplanemp/wiki) including notes on CSL development
