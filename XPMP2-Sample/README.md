XPMP2 Sample Plugin
=========

This is a fully functional sample implementation that you can use to play with
XPMP2 features and as a starting point for your own implementations.

I am also using it as a testbed for new features, so the implementation
is regularly built, tested, and sometimes even enhanced to make use of
latest features.

## Build ##

There are separate projects available to only build the XPMP2-Sample plugin,
serving as a starting point for your projects.

Please refer to the
[GitHub Build documentation](https://twinfan.github.io/XPMP2/Building.html)
for details.

## Features ##

This plugin creates 3 planes, one with each of the available ways of using the XPMP2 library.
All planes move in a circle around a position 200m in front of the user's plane
(even if the user plane moves).
1. Subclassing `XPMP2::Aircraft` in `class SampleAircraft`,
   which is the recommended way for any new plugin. This plane flies
   highest, 100m above the user plane.
2. Subclassing the legacy `class XPCAircraft`. This plane flies in the middle,
   50m above the user plane.
3. Using direct C functions. This plane always rolls on the ground, no matter how
   high the user plane flies.

Three menu items are provided:

 1. "Toggle Planes" creates/removes the planes.
 2. "Toggle Visibility" shows/temporary hides the planes without destroying them.
 3. "Cycle Models" changes the CSL model used per plane.
     Also, it flips the visibility of labels in the map view...just for a change.

## Installation ##

For the plugin to work properly some CSL models are necessary in some folders
under `Resources` (all folders under `Resources` are scanned for
`xsb_aircraft.txt` file, so the actual structure underneath `Resources` does not matter).

The directory structure would look as follows:
```
XPMP2-Sample/
  lin_x64/
    XPMP2-Sample.xpl
  mac_x64/
    XPMP2-Sample.xpl
  win_x64/
    XPMP2-Sample.xpl
  Resources/
    CSL/         <-- install CSL models here
    Doc8643.txt
    MapIcons.png
    Obj8DataRefs.txt
    related.txt
