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

Concepts like the "language" of the `xsb_aircraft.txt` file or certainly the idea of an
multi-pass matching to find a good model are retained, though re-implemented (read: can have new bugs).

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

A few changes are there for clarity and future proveness, though, which should not
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
- `XPMPLoadCSLPackage` walks directries hierarchically up to 5 levels until it
   finds an `xsb_aircraft.txt` file. This should not affect classic usages,
   where such a path was just one level away from the `xsb_aircraft.txt` file.
   It would just also search deeper if needed.

I tested the compile-time compatibility with LiveTraffic successfully. LiveTraffic has
always used subclassing of `XPCAircraft`, so I am very sure that
implementations basing on this implementation model will just compile.
There have been a lot less tests with the direct C-style interface using `XPMPCreatePlane()` et al.

Compatibility with X-Plane 10 (planned)
--

I am planning to even make this library compatible with XP10 to a certain extent.
XP10 has no instancing, so most of the performance benefits are lost.
The XP10 compatibility will base on the idea of the
[instancing compatibility wrapper](https://developer.x-plane.com/code-sample/x-plane-10-instancing-compatibility-wrapper/),
enhanced by support for non-writeable dataRefs and especially
for dynamically identifying XP11 vs. XP10 by resolving function pointers
using `XPLMFindSymbol`. The latter is practised already by LiveTraffic,
which makes use of XP11 feature if available, but still runs on XP10, so there's
quite some experience with it...

The original library had quite exentensive code to avoid drawing in unnecessary cases.
Instancing takes care of all that in XP11, so that code is not taken over. Be prepared that XPMP2
will be less performant on XP10 than the original library is.

Limits
--

To keep the reimplementation streamlined, this library no longer supports some aspects
I considered no longer required:
- `.acf` and OBJ7 models are no longer supported: XPMP2 requires OBJ8 models.
   These are the by far most used models nowadays, identified by the `OBJ8` command
   in the `xsb_aircraft.txt` file.

TODOs
--

- Model loading / unloading
    - add garbage collection to models with reference counter zero, not used for a few minutes
- Label writing
    - Expose maxLabelDist to some config function 
- AI/Multiplayer dataRefs (logic can be taken over from original library, it's my code anyway)
    - Selecting priority planes 
    - Standard X-Plane dataRefs
    - Shared dataRefs for providing textual information (test with FSTramp)
- Test with camera...might still jitter
    - Test with ABC
    - Consider a "next position" and also multiplayer dataRef handling in AIMultiUpdate()
- Support replacing textures with the extended syntax `OBJ8 SOLID YES <obj> <texture> <texture_lit>`
    - basically do what CSL2XSB.py does with respect to TEXTURE / TEXTURE_LIT
    - definition should already be read from `xsb_aircraft.txt`
    - before loading the object, the .obj file needs to be rewritten
    - devise a reproducible name scheme, so that replacement .obj file is written just once and found again next time without generation
- X-Plane 10 compatibility (?)
    - The <a href="">instancing compatibility wrapper</a> is a start but is lacking
    - support for non-writeable dataRefs (basically all in the case of XPLM2)
    - proper wrapping of in XP10 unavailable functions XPLM*Instance* via XPLMFindSymbol

Links to outside locations:
--

Original libxplanemp:
- [TwinFan's libxplanemp fork](https://github.com/TwinFan/libxplanemp) on GitHub
    - [wiki explaining differences to the kuroneko fork](https://github.com/TwinFan/libxplanemp/wiki)
- [kuroneko's fork](https://github.com/kuroneko/libxplanemp) on GitHub, a long-time standard and basis of other important forks
    - [kuroneko's wiki](https://github.com/kuroneko/libxplanemp/wiki) including notes on CSL development
