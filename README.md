XPlaneMP 2
========================

The original X-Plane Multiplay Library is the work of many fantastic people,
with Ben Supnik, Chris Serio, and Chris Collins appearing in recent files and documentation.
But the origins date back to 2004, and very likely many more were involed. Thanks to all of them!

This complete re-implementation honors all the basic concepts (so I hope)
but makes use of X-Plane 11's modern
[instancing concept](https://developer.x-plane.com/sdk/XPLMInstance/).
Thus, it ports the idea of the library also into the times of Vulkan and Metal
when the drawing mechanisms used by the original library no longer work.

At the same time, using instancing marks basically all parts of the original rendering code
of the library outdated...it is basically replaced by one line of code calling
`XPLMInstanceSetPosition`.

Concepts like the syntax of the `xsb_aircraft.txt` file or certainly the idea of an
multi-pass matching to find a good model are retained, though re-implemented (read: can have new bugs).

XPMP2 does no longer call any OpenGL function and hence  does not require
to be linked to an OpenGL library. The included XPMP2-Sample application does not link to OpenGL.

TCAS
----

**WARNING:** TCAS handling is still work in progress.

The previous TCAS hack no longer works as the used flight loop phases no longer exist
and replacement phases aren't identical between Vulkan and Metal.

XPMP2 uses a new approach: Instead of tricking X-Plane not to render AI planes
it provides X-Plane with a valid aircraft file, which does not contain any object to draw:
`NoPlane.acf`. So actually, two planes are drawn for each plane that is at the same time
supported as AI multiplayer plane for TCAS: One by XPMP2, which really is visible,
and `NoPlane.acf` on top of that by X-Plane, which is (nearly) invisible as it has (nearly) no visible objects.
This has the same effect in the internal map: The X-Plane version of the plane is (nearly) invisible,
only the track marks unveil that X-Plane is tracking its AI multiplayer planes.

When releasing AI control with `XPMPMultiplayerDisable()` then the planes types,
which were configured before XPMP2 took AI control, are restored.

**Note:** Restoring these original plane types can take a while,
even seconds, as long as during X-Plane's startup, as each model is loaded
by that time synchronously. `Log.txt` has the story.

**Note:** If the plugin cannot gracefully shutdown (deactivated via Plugin admin or X-Plane crashes),
then it is possible that the `NoPlane` setup configured by `XPMP2` remains and the user
will not see his/her individual AI Aircraft setup any longer, but a list of "NoPlane" aircraft.

The new `NoPlane.acf` file should be in the standard `Aircraft` folder of X-Plane.
Only then does X-Plane not get confused and retains the AI Aircraft configuration
also if the user opens the Flight Configuration, e.g. to change location.
`XPMP2` would also read `NoPlane.acf` from the resource directory
(provided in the call to `XPMPMultiplayerInit`), but issues a warning in that case into `Log.txt`.

Status
--
**This is work in progress.**

The XPMP2 lib has been successfully tested with
- X-Plane 11.50 Beta 3 under OpenGL, Vulkan, and Metal,
- the enclosed sample plugin,
- a new branch of LiveTraffic (executables not yet published)
- on Mac OS and
- on Windows.

Linux binaries can successfully build in a docker environment.
But I have no environment for testing them.

Using XPMP2
--

You don't need to build the library yourself if you don't want. Binaries are included in
`XPMP2-Sample/lib`.

New Implementations
---

New plugin implementations are strongly adviced to directly sub-class
from the new class `XPMP2::Aircraft`, which is the actual aircraft representation.
Override the abstract function `UpdatePosition()` and directly write into the object's
member variables to provide current position, orientation (directly into `drawInfo` or
indirectly via a call to `SetLocation()`), and config (array `v` using the elements
of `DR_VALS` as indexes).

This way minimizes the number of copy operations needed. `drawInfo` and the `v` array
are _directly_ passed on to the `XPLMInstanceSetPosition` call.

Other values like `label`, `aiPrio`, or `acInfoTexts` can also be updated by your
`UpdatePosition()` implementation and are used when drawing labels
or providing information externally like via AI/multiplayer dataRefs.

Compile-Time Compatibilty
---

Despite the new approach, XPMP2 shall be your drop-in replacement for the
original library: The original header files are still provided with the same name.
All original public functions are still there. The original `XPCAircraft` class is still there,
now derived from `XPMP2::Aircraft`.

A few changes are there, though, for clarity and to be future-proof. They should not
hinder a proper implementation to compile successfully, albeit with some new warnings:
- All enumerations are now proper `enum` definitions, i.e. `enum typeName {...}` instead of
  `typedef int typeName`.
- A number of functions and the class `XPCAircraft` are explicitely marked `[[deprecated]]`,
   which will raise a few warnings, if your compiler is configured to show them.
   Just a gentle reminder to update your plugin at some point in time...
- `XPMPMultiplayerInitLegacyData` will in turn call `XPMPMultiplayerInit`, and
   `XPMPLoadCSLPackage`. The correct future-proof way of initializing the library is to call
   `XPMPMultiplayerInit` and then do one or more calls to `XPMPLoadCSLPackage`. 
- `XPMPLoadCSLPackage` walks directories hierarchically up to 5 levels until it
   finds an `xsb_aircraft.txt` file. This should not affect classic usages,
   where such a path was just one level away from the `xsb_aircraft.txt` file.
   It would just also search deeper if needed.
- Parameters of `XPMPMultiplayerInit(LegacyData)` have been reshaped, though,
   ie. calls to these functions need adaptation. Less parameters are needed now, though,
   please see documentaton or header file. The path to the resource directory is now
   mandatory and it describes where all the supplemental files needed by the library
   are to be found: `Doc8643.txt`, `MapIcons.png`, `NoPlane.acf`, `related.txt`.
- It is no longer necessary to define the compile-time macros `XPMP_CLIENT_NAME`
  and `XPMP_CLIENT_LONGNAME`. Instead, you can use the new parameter
  `inPluginName` in the call to `XPMPMultiplayerInit` or the function
  `XPMPSetPluginName` to set the plugin's name from within your plugin.
  XPMP2 tries to guess the plugin's name if no name is explicitely set.
  This allows using the provided libraries directly without the need to recompile.

I tested the compile-time compatibility with LiveTraffic successfully. LiveTraffic has
always used subclassing of `XPCAircraft`, so I am very sure that
implementations basing on this implementation model will just compile.
There have been less tests with the direct C-style interface using `XPMPCreatePlane()` et al.,
mostly using the sample plugin included in the package.

Features
--

Full feature list to be document. In the meantime refer to:
- Generally, see [kuroneko's wiki](https://github.com/kuroneko/libxplanemp/wiki).
- All additions of my fork are included as well, see [TwinFan's wiki](https://github.com/TwinFan/libxplanemp/wiki).

Limits
---

To keep the reimplementation streamlined, this library no longer supports some aspects
I considered no longer required:
- Requires X-Plane 11.
- `.acf` and OBJ7 models are no longer supported: XPMP2 requires OBJ8 models.
   These are the by far most used models nowadays, identified by the `OBJ8` command
   in the `xsb_aircraft.txt` file.
   
New Features
---
- **Map Layer Support:** XPMP2 shows all aircraft in X-Plane's map views,
  with icons roughly related to the
  plane's type and size. The map layer is called by its plugin name (see
  `XPMPSetPluginName`).
- `XPMPLoadCSLPackage` walks directories hierarchically up to 5 levels deep
  to search for `xsb_aircraft.txt` files. In complex multi-CSL-package setups,
  it might now be sufficient to pass in some higher-level directory instead calling the
  function several times per package.

Sample Plugin
---
This package comes with a sample plugin in the `XPMP2-Sample` folder. It is a complete
plugin including build projects and CMake setup. It displays 3 aircraft flying circles
in front of the user's plane. Each of the 3 aircraft is using a different technology:
the now recommended way of subclassing `XPMP2::Aircraft`, the legacy way
of subclassing `XPCAircraft` (as used by LiveTraffic v1.x), and by calling
standard C functions.

Doxygen Documnetation
--

All code has [Doxygen](http://doxygen.nl)-style documentation.
A Doxygen configuration is provided in `docs/XPMP2.doxygen`. 
Pass it to a call to Doxygen to build the documentation.
Entry point then is `docs/html/index.html`.

Building XPMP2
--

Mac OS
---

My development environment is Mac OS, so expect the XCode environment to be
maintained best. Open `XPMP2.xcodeproj` in XCode, both for the library in the root folder
as well as for the sample plugin in `XPMP2-Sample`.

The result XPMP2 framework is also copied to `XPMP2-Sample/lib` so it is accessible
when building the sample plugin.

In the sample plugin's settings you will need to change the value of the user-defined
build macro `XPLANE11_ROOT` to point to _your_ X-Plane folder.
Then the plugin is installed there right after build.

Windows
---

Windows cannot build in the Docker environment as mingw's libraries aren't up to C++ 17 standards.

- Install Visual Studio](https://visualstudio.microsoft.com/vs/community/)
- Open the root folder to build the library
- Open the folder `XPMP2-Sample` to build the sample plugin
- In both cases will VS use the CMake setup to configure building the binaries.
- Build from within Visual Studio.

Results are in `build/x64`.

The library is also copied into `XPMP2-Sample/lib` to be available in a subsequent build
of the sample plugin.

Linux and Mac OS via Docker
---

A docker environment based on Ubuntu 18.04 is provided,
which can build both Linux and MacOS.

- Install [Docker Desktop](https://www.docker.com/products/docker-desktop) and start it.
- `cd` to the project's `docker` folder, and enter `make` for the library
- `cd` to `XPMP2-Sample/docker`, and enter `make` for the plugin.
- `make`

In the first run only, it will create the necessary Docker image based on Ubuntu 18.04,
which includes downloading lots of base images and packages and takes a couple
of minutes. This is required once only.

The actual build takes only a few seconds. Results are written to `build-lin` and `build-mac`.

The resulting library/framework are also copied into `XPMP2-Sample/lib`.
Then, also the sample plugin must be build using the docker environment.

TODOs
--

- Label writing
    - Expose maxLabelDist to some config function 
    - Scale Fonts
- AI/Multiplayer dataRefs
    - Shared dataRefs for providing textual information (implemented, open: test with FSTramp)
- Add VERT_OFS auto detection
- Support replacing textures with the extended syntax `OBJ8 SOLID YES <obj> <texture> <texture_lit>`
    - basically do what CSL2XSB.py does with respect to TEXTURE / TEXTURE_LIT
    - definition should already be read from `xsb_aircraft.txt`
    - before loading the object, the .obj file needs to be rewritten
    - devise a reproducible name scheme, so that replacement .obj file is written just once and found again next time without generation

Links to outside locations:
--

Original libxplanemp:
- [TwinFan's libxplanemp fork](https://github.com/TwinFan/libxplanemp) on GitHub
    - [wiki explaining differences to the kuroneko fork](https://github.com/TwinFan/libxplanemp/wiki)
- [kuroneko's fork](https://github.com/kuroneko/libxplanemp) on GitHub, a long-time standard and basis of other important forks
    - [kuroneko's wiki](https://github.com/kuroneko/libxplanemp/wiki) including notes on CSL development
