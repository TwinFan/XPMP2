XPMP 2
=========

The original X-Plane Multiplay Library is the work of many fantastic people,
with Ben Supnik, Chris Serio, and Chris Collins appearing in recent files and documentation.
But the origins date back to 2004, and very likely many more were involved. Thanks to all of them!

This complete re-implementation honours all the basic concepts (so I hope)
but makes use of 2 modern X-Plane 11 concepts:
- [instancing](https://developer.x-plane.com/sdk/XPLMInstance/), and
- [TCAS Override](https://developer.x-plane.com/article/overriding-tcas-and-providing-traffic-information/)

Thus, it ports the idea of the library also into the times of Vulkan and Metal
when the drawing mechanisms used by the original library no longer work.

At the same time, using instancing marks basically all parts of the original rendering code
of the library outdated...it is basically replaced by one line of code calling
`XPLMInstanceSetPosition`.

Concepts like the syntax of the `xsb_aircraft.txt` file or certainly the idea of an
multi-pass matching to find a good model are retained, though re-implemented (read: can have new bugs).

XPMP2 does no longer call any OpenGL function and hence does not require
to be linked to an OpenGL library. The included XPMP2-Sample application does not link to OpenGL.

Despite its new approach, XPMP2 shall be your **drop-in replacement for libxplanemp**:
The original header files are still provided with the same name.
All original public functions are still there.
The original [XPCAircraft class](https://twinfan.github.io/XPMP2/html/classXPCAircraft.html)
is still there, now derived from [XPMP2::Aircraft](https://twinfan.github.io/XPMP2/html/classXPMP2_1_1Aircraft.html).
See [here](https://twinfan.github.io/XPMP2/BackwardsCompatibility.html) for more details.

## Pre-Built Release ##

If you don't want to build the library yourself you can find archives with
headers and release/debug builds in the
[Release section](https://github.com/TwinFan/XPMP2/releases)
here on GitHub.

## Status ##

Available, and should be rather reliable now. No (major) bugs are known.
Future development can certainly extend functionality while trying hard
to stay backward compatible.

The XPMP2 library has been successfully tested with
- X-Plane 11.50 RC3 under OpenGL, Vulkan, and Metal,
- the enclosed sample plugin,
- [LiveTraffic v2.10](https://forums.x-plane.org/index.php?/files/file/49749-livetraffic/)
- on Mac OS,
- Windows, and
- Linux (Ubuntu 18.04 and similar).

Apparently, the X-Plane versions of [X-Pilot 1.3](http://xpilot-project.org/) and
[IVAO Altitude](https://www.ivao.aero/softdev/beta/altitudebeta.asp)
clients also use XPMP2 already.

## Requirements ##

- XPMP2 implements [instancing](https://developer.x-plane.com/sdk/XPLMInstance/),
  so it **requires X-Plane 11.10** or later
- CSL models in **OBJ8 format** (ie. older OBJ7 models are no longer supported)
- Coversion of CSL model packages with
  [CSL2XSB.py](https://github.com/TwinFan/CSL2XSB/releases) is strongly recommended
  to unlock more model features, though not stricly required.

## Documentation: See [GitHub pages](https://twinfan.github.io/XPMP2/) ##

...on requirements, API, building, deployment, TCAS target, CSL mode dataRefs
and more is available in the
[GitHub pages](https://twinfan.github.io/XPMP2/).

### Sample Plugin ###

This package comes with a sample plugin in the `XPMP2-Sample` folder. It is a complete
plugin including build projects and CMake setup. It displays 3 aircraft flying circles
in front of the user's plane. Each of the 3 aircraft is using a different technology:
the now recommended way of subclassing `XPMP2::Aircraft`, the legacy way
of subclassing `XPCAircraft` (as used by LiveTraffic v1.x), and by calling
standard C functions.

The [HowTo documentation](https://twinfan.github.io/XPMP2/HowTo.html#sample-plugin)
includes how to install the sample plugin, especially also CSL models.

## Main Features ##

### Model Matching ###

The way how XPMP2 picks any of the available CSL models for display
is [documented here](https://twinfan.github.io/XPMP2/Matching.html).

### Enhancing CSL Models ###

CSL packages come in different flavours. Popular ones for general use are
the Bluebell and the X-CSL packages. Both come with different history.
For XPMP2 to use all their features (all liveries, turning rotors, props, wheels...)
it needs to adapt their `.obj` files. Performing these changes is built into XPMP2,
[details here](https://twinfan.github.io/XPMP2/CopyingObjFiles.html).

### TCAS and AI/multiplayer Support ###

XPMP2 provides TCAS blibs and AI/multiplayer data using
[TCAS Override](https://developer.x-plane.com/article/overriding-tcas-and-providing-traffic-information/)
introduced with X-Plane 11.50 Beta 8. Find
[details here](https://twinfan.github.io/XPMP2/TCAS.html)
including a reference lists of provided dataRef values and their sources.

When TCS Override is not available (like up to X-Plane 11.41),
then TCAS is provided by writing the classic multiplayer dataRefs directly.

### Map Layer ###

XPMP2 creates an additional layer in X-Plane's internal map, named after the
plugin's name. This layer displays all planes as currently controled by XPMP2
with an icon roughly matching the plane type, taken from `Resources/MapIcons.png`.
