/// @file       xplanemp2.h
/// @brief      Header file covering all includes required for compiling XPlaneMP2, basis for pre-compiled headers
/// @author     Birger Hoppe
/// @copyright  (c) 2020 Birger Hoppe
/// @copyright  Permission is hereby granted, free of charge, to any person obtaining a
///             copy of this software and associated documentation files (the "Software"),
///             to deal in the Software without restriction, including without limitation
///             the rights to use, copy, modify, merge, publish, distribute, sublicense,
///             and/or sell copies of the Software, and to permit persons to whom the
///             Software is furnished to do so, subject to the following conditions:\n
///             The above copyright notice and this permission notice shall be included in
///             all copies or substantial portions of the Software.\n
///             THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
///             IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
///             FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
///             AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
///             LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
///             OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
///             THE SOFTWARE.

#ifndef _xplanemp2_h_
#define _xplanemp2_h_

// XPlaneMP 2 - Public Header Files
#include "XPMPMultiplayer.h"
#include "XPMPAircraft.h"
#include "XPCAircraft.h"
#include "XPMPPlaneRenderer.h"

// X-Plane SDK
#include "XPLMUtilities.h"
#include "XPLMScenery.h"
#include "XPLMProcessing.h"
#include "XPLMGraphics.h"
#include "XPLMDataAccess.h"

// Standard C
#include <sys/stat.h>
#include <cmath>

// Standard C++
#include <string>
#include <list>
#include <map>
#include <vector>
#include <algorithm>
#include <fstream>

// XPlaneMP 2 - Internal Header Files
#include "Utilities.h"
#include "CSLModels.h"
#include "Aircraft.h"

//
// MARK: Global Configurations and variables
//

namespace XPMP2 {

/// All global config settings and variables are kept in one structure for convenient access and central definition
struct GlobVars {
public:
    /// Logging level
    logLevelTy      logLvl      = logWARN;
    /// Debug model matching?
    bool            bLogMdlMatch= false;
    /// Clamp all planes to the ground?
    bool            bClampAll   = false;
    /// Register also always in addition the standard dataRef root "libxplanemp/"?
    bool            bDrStdToo   = true;

    /// Configuration callback for integer values
    int (*prefsFuncInt)(const char *, const char *, int) = XPMP2::PrefsFuncIntDefault;
    /// Configuration callback for float values
    float (*prefsFuncFloat)(const char *, const char *, float) = XPMP2::PrefsFuncFloatDefault;

    /// Global map of all CSL Packages, indexed by `xsb_aircraft.txt::EXPORT_NAME`
    mapCSLPackageTy mapCSLPkgs;
    /// Global map of all CSL Models, indexed by ID (xsb_aircraft.txt::OBJ8_AIRCRAFT)
    mapCSLModelTy   mapCSLModels;
    /// Default ICAO model type if no match can be found
    std::string     defaultICAO = "A320";
    /// Resource directory, to store local definitions of vertical offsets (clamping)
    std::string     resourceDir;
    
    /// Global map of all created planes
    mapAcTy         mapAc;
    /// Shall we draw aircraft labels?
    bool            bDrawLabels = true;
    
protected:
    /// Current plane ID counter
    unsigned long long  planeId = 0;
public:
    /// Get the next plane id
    XPMPPlaneID NextPlaneId () { return XPMPPlaneID(++planeId); }

public:
    /// Constructor
    GlobVars (logLevelTy _logLvl = logWARN) : logLvl(_logLvl) {}
    /// Update setting for logging level by calling prefsFuncInt
    void ReadLogLvl ();
};

/// The one and only global variable structure
extern GlobVars glob;

}

#endif
