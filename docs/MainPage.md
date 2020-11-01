XPMP 2 - API and Code documentation
========================

API
--

### Required Header Files

A new plugin, which wants to use the XPMP2 library, needs to include
the following 2 header files, provided in the `inc` folder:

- XPMPMultiplayer.h - Initialization and general control functions
- XPMPAircraft.h - Defines the class `XPMP2::Aircraft`,
  which represents an aircraft. **Subclass this class** in your plugin!

### Additional Header Files

There are 3 additional headers files, which are usually **not required**:
  
The following header files are deprecated and provided only for
backward compatibility with previous `libxplanemp`,
_not to be used for new projects_:

- XPCAircraft.h - Defines the class `XPCAircraft`,
  representing an aircraft in `XPMP2`, now derived from `XPMP2::Aircraft`.
- XPMPPlaneRenderer.h - Three rendering control functions,
  which serve no purpose any longer.

The following header file is for use by the "XPMP2 Remote Client":
- XPMPRemote.h - Defines network functionality and data structure
  for synchronizing planes between networked computers. 

Quick Links to Detailed Documentation:
--

- <a href=annotated.html>Full Class List</a>

- <a href=hierarchy.html>Full Class Hierarchy</a>

- <a href=files.html>File List</a>
