XPMP 2 - API and Code documentation
========================

API
--

The public API of XPMP2 consists of the following 4 header files,
which are provided in the `inc` folder:

- XPMPMultiplayer.h - Initialisation and general control functions
- XPMPAircraft.h - Defines the class `XPMP2::Aircraft`,
  which represents an aircraft. Subclass this class!

**The following header files are deprecated** and only provided only for
backward compatibility with previous `libxplanemp`,
_not to be used for new projects_:

- XPCAircraft.h - Defines the class `XPCAircraft`,
  representing an aircraft in `XPMP2`, now derived from `XPMP2::Aircraft`.
- XPMPPlaneRenderer.h - Three rendering control functions,
  which serve no purpose any longer.

Quick Links to Detailed Documentation:
--

- <a href=annotated.html>Full Class List</a>

- <a href=hierarchy.html>Full Class Hierarchy</a>

- <a href=files.html>File List</a>
