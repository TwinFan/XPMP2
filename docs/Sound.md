## Sound Support by FMOD

XPMP2's Audio Engine is FMOD Core API by Firelight Technologies Pty Ltd.
Understand FMOD [licensing](https://www.fmod.com/licensing) and
[attribution requirements](https://www.fmod.com/attribution) first,
as they will apply to _your_ plugin if using XPMP2 with sound support.

While X-Plane supports and uses FMOD, too, for aircraft-specific sounds,
XPMP2 cannot make use of any X-Plane functionality to produce the sounds
as X-Plane at the moment does not allow to dynamically add sounds
to instanced objects.

Instead, XPMP2 creates its own fully indepent instance of the FMOD system.
This also means that XPMP2 has to provide FMOD with all spacial information
for each aircraft's sound to create a proper 3D sound illusion
relative to the user's current camera position. Which opens a totally new
are for potential bugs...

### Building XPMP2 with FMOD Sound Support

Due to above licencing requirements,
sound support is only included if XPMP2 is consciously built with
CMake cache entry `INCLUDE_FMOD_SOUND`, which in turn defines a
compile-time macro by the same name, e.g. by doing
```
cmake -G Ninja -DINCLUDE_FMOD_SOUND=1 ..
```
The Docker `Makefile` allows passing CMake parameters via the `FLAGS` macro:
```
make FLAGS=-DINCLUDE_FMOD_SOUND=1 {...platform(s)...}
```
The GitHub Actions triggered by default are building without FMOD support, too.
Running "Build all Platforms" manually, though, allows to specify `FLAGS`.

### Including FMOD Sound Support in Your Plugin

In your plugin, you also need to define `INCLUDE_FMOD_SOUND` _before_ including the XPMP2 headers:
```
#define INCLUDE_FMOD_SOUND 1            // Include FMOD sound support
#include "XPMPMultiplayer.h"
#include "XPMPAircraft.h"
```

Then certainly link with an FMOD-enabled version of the XPMP2 library.

#### Avoid Crash

If the XPMP2 you linked is compiled with `INCLUDE_FMOD_SOUND`
but your actual plugin without,
then the plugin will crash in `XPMP2::Aircraft::FlightLoopCB()`
at the point when it tries to call `XPMP2::Aircraft::UpdateSound()`,
because the `XPMP2::Aircraft` object created by your plugin
is created with too short a `vtable` as it did not consider the
virtual member functions for sound support.

You can tell if your XPMP2 library includes FMOD support
latest from the init message in
`Log.txt`, when it says:

```
...XPMPMultiplayerInit: XPMP2 2.60 with FMOD support initializing...`
```

Solution: Either link to an XPMP2 build without FMOD support. Or make sure to
`#define INCLUDE_FMOD_SOUND 1` in your plugin before including the XPMP2 headers.

#### FMOD Logo

Not built into XPMP2 (to avoid a link-time dependency on OpenGL) but provided separately are functions to create the FMOD logo as per [attribution requirements](https://www.fmod.com/attribution). Include `FMOD_Logo.cpp` and `FMOD_Logo.h` from `XPMP2-Sample/lib/fmod/logo` into your project. Showing the FMOD logo in a Dear ImGui window is then as simple as
```
#include "FMOD_Logo.h"
...
int logoId = 0;
if (FMODLogo::GetTexture(logoId)) {
    ImGui::Image((void*)(intptr_t)logoId, ImVec2(FMODLogo::IMG_WIDTH, FMODLogo::IMG_HEIGHT));
}
```

### Ship `fmod.dll`

If your plugin supports X-Plane 11 under Windows, then you need to ship
`fmod.dll` in the `win_x64` folder. See [Deploying](Deploying.html) for details.

### Default Sounds

Your are **not required** to provide any sound-specific coding in your plugin.
As a matter of fact, the XPMP2-Sample plugin has no single line of code
that controls sound. (There is only some sample code to enumerate the loaded
sounds into `Log.txt`, purely for my personal testing purposes.)

Once built with sound support, XPMP2 provides a lot of sounds by default.
XPMP2 distinguishes 5 sound types, see `XPMP2::Aircraft::SoundEventsTy`,
and plays sounds that are included in any X-Plane installation's
`Resources/sounds` folders based on the level of certain dataRefs:

Event | dataRef | Sound
-------|------|-----------
Engine | Thrust Ratio | Depends on aircraft type, volume changes with Thrust
Reverse | Thrust Reverse Ratio | `XP_SOUND_REVERSE_THRUST`, volume changes with Thrust Reverse
Tire | Tire Rotation RPM | `XP_SOUND_ROLL_RUNWAY`, volume changes with RPM
Gear | Gear Ratio | `XP_SOUND_GEAR` upon a change to the Gear Ratio
Flaps | Flap Ratio | `XP_SOUND_FLAP` once upon a change to the Flap Ratio

The engine sounds depend on the aircraft type, more specifically on the
[ICAO aircraft type designators](https://www.icao.int/publications/doc8643/pages/search.aspx)
as provided in `Resources/Doc8643.txt`:

Type Designator | means             | Sound
----------------|-------------------|---------------
`ZZZC`          | Ground vehicle    | `XP_SOUND_ELECTRIC`
`H??` or `G??`  | Heli, Gyrocopter  | `XP_SOUND_PROP_HELI`
`E??`           | Electric          | `XP_SOUND_ELECTRIC`
`J1?`           | Jet, single       | `XP_SOUND_HIBYPASSJET`
`J??`           | Jet, multi        | `XP_SOUND_LOBYPASSJET`
`P??`           | Piston            | `XP_SOUND_PROP_AIRPLANE`
`T??`           | Turboprop         | `XP_SOUND_TURBOPROP`

The sounds' volume additionally depends on the number of engines,
which while obvious for the engine sounds is generally an ok indicator
for the plane's size and hence volume.

### Controlling Sound

Here are your options to influence sound.

#### Enable/Disable

Beside using `XPMPSoundEnable()` directly, you can also provides values
for configuration items `XPMP_CFG_ITM_ACTIVATE_SOUND` and
`XPMP_CFG_ITM_MUTE_ON_PAUSE` in your configuration callback.

#### General Volume

`XPMPSoundSetMasterVolume()` and `XPMPSoundMute()` allow controlling
the volume of XPMP2's sounds generally across all aircraft.

#### Add your own Sounds

`XPMPSoundAdd()` allows you to add your own sounds.
Sounds can be looping (like engine sounds, which play continuously)
or one-time sounds.

The function optionally also allows to define **sound cone** parameters.
A sound cone basically is a cone-shaped area, in which the sound
is heared with full volume, while outside of it the sound is reduced
(attenuated) to a lower volume. The default sounds use that for the
jet eingine sounds, which are louder _behind_ the aircraft.
`coneDir` and `conePitch` are defined relative to the aircraft's heading.
XPMP2 takes care of orientating the actual sound cone dynamically with
the actual aircraft orientation. The other three cone-specific
parameters are passed through to FMOD
(see [set3DConeSettings](https://www.fmod.com/docs/api/core-api-sound.html#sound_set3dconesettings)).

#### Override virtual `Aircraft` member functions

For more control over the sounds your aircraft emit you can override
`Aircraft`'s virtual member functions:

- `Aircraft::SoundSetup()`
  is called after aircraft creation once the CSL model to use is determined;
  called again after CSL model changes. Use to define/override some defaults.
  Is a good place to set `aSndCh[SoundEventsTy].bAuto = false` in case
  you want to have full control over any of the pre-defined sound types.
- `Aircraft::SoundUpdate()`
  is called in every flight loop cycle and decides,
  which sounds to start/stop at which volume. You can make calls to
  functions like `Aircraft::SoundPlay/SoundStop/SoundVolume()`.
- `Aircraft::SoundGetName()` is called by XPMP2's standard implementation of
  `SoundUpdate()`. Override if you just want to have control over which sounds
  at which volume to play for the standard event types, but otherwise
  want to leave sound handling to XPMP2. You can return any sound name,
  ie. also additional names you have registered previously with `XPMPSoundAdd()`.
- `Aircraft::SoundRemoveAll()` is called during object destruction to cleanup
  all sound objects. If you have started playing any sounds yourself
  you should clean them up here before calling the standard function
  to give XPMP2 a chance for cleanup, too.