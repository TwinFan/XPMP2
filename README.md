XPlaneMP 2
========================

The original X-Plane Multiplay Library is the work of many fantastic people,
with Ben Supnik, Chris Serio, and Chris Collins appearing in recent files and documentation.
But the origins date back to 2004, and very likely many more were involed. Thanks to all of them!

This complete re-implementation honors all the basic concepts (so I hope)
but makes use of X-Plane 11's modern
[instancing concept](https://developer.x-plane.com/sdk/XPLMInstance/).
Thus, it should be able to port the idea of the library also into the times of Vulkan and Meta
when the drawing mechanisms used by the original library will no longer work.

At the same time, using instancing marks basically all parts of the original rendering code
of the library outdated...it is basically replaced by one line of code calling
`XPLMInstanceSetPosition`.

Concepts like the syntax of the `xsb_aircraft.txt` file or certainly the idea of an
multi-pass matching to find a good model are retained, though re-implemented (read: can have new bugs).

Status
--
**This is work in progress and not yet ready to be used!**

New Implementations
--

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
--

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
   `XPMPLoadCSLPackage`. If your code calls both `XPMPMultiplayerInitLegacyData` and
   `XPMPMultiplayerInit` you could remove the latter...but it should still work, just
   does some stuff twice.
- `XPMPLoadCSLPackage` walks directories hierarchically up to 5 levels until it
   finds an `xsb_aircraft.txt` file. This should not affect classic usages,
   where such a path was just one level away from the `xsb_aircraft.txt` file.
   It would just also search deeper if needed.
- `XPMPMultiplayerInitLegacyData` can take one more optional parameter,
  `inMapIconFile`, which defines the full path to the `MapIcons.png` file,
  which contains the icons shown in X-Plane's maps.\n
  Its parameter `inTexturePath` is no longer required and can be `nullptr`.
- It is no longer necessary to define the compile-time macros `XPMP_CLIENT_NAME`
  and `XPMP_CLIENT_LONGNAME`. Instead, you can use the new function
  `XPMPSetPluginName` to set the plugin's name from within your plugin,
  ideally as the very first call even before `XPMPMultiplayerInitLegacyData`.
  (XPMP2 tries to guess the plugin's name if no call to `XPMPSetPluginName`
  is made.)
  This allows using the provided libraries directly without the need to recompile.

I tested the compile-time compatibility with LiveTraffic successfully. LiveTraffic has
always used subclassing of `XPCAircraft`, so I am very sure that
implementations basing on this implementation model will just compile.
There have been a lot less tests with the direct C-style interface using `XPMPCreatePlane()` et al.

Limits
--

To keep the reimplementation streamlined, this library no longer supports some aspects
I considered no longer required:
- Requires X-Plane 11.
- `.acf` and OBJ7 models are no longer supported: XPMP2 requires OBJ8 models.
   These are the by far most used models nowadays, identified by the `OBJ8` command
   in the `xsb_aircraft.txt` file.
   
New Features
--
- XPMP2 shows all aircraft in X-Plane's map views, with icons roughly related to the
  plane's type and size. The map layer is called by its plugin name (see
  `XPMPSetPluginName`).
- `XPMPLoadCSLPackage` walks directories hierarchically up to 5 levels deep
  to search for `xsb_aircraft.txt` files. In complex multi-CSL-package setups,
  it might now be sufficient to pass in some higher-level directory instead calling the
  function several times per package.

Sample Plugin
--
This package comes with a sample plugin in the `XPMP2-Sample` folder. It is a complete
plugin including build projects and CMake setup. It displays 3 aircraft flying circles
in front of the user's plane. Each of the 3 aircraft is using a different technology:
the now recommended way of subclassing `XPMP2::Aircraft`, the legacy way
of subclassing `XPCAircraft` (as used by LiveTraffic v1.x), and by calling
standard C functions.

TODOs
--

- Model loading / unloading
    - add garbage collection to models with reference counter zero, not used for a few minutes
- Label writing
    - Map: Make label writing configurable 
    - Expose maxLabelDist to some config function 
- AI/Multiplayer dataRefs
    - Shared dataRefs for providing textual information (test with FSTramp)
- Add VERT_OFS auto detection
- Test with camera...might still jitter
    - Test with ABC
    - Consider a "next position" and also multiplayer dataRef handling in AIMultiUpdate()
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
