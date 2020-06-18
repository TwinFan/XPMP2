/// @file       XPMP2.h
/// @brief      Header file covering all includes required for compiling XPMP2, basis for pre-compiled headers
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

#ifndef _XPMP2_h_
#define _XPMP2_h_

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
#include "XPLMPlugin.h"
#include "XPLMDisplay.h"
#include "XPLMCamera.h"
#include "XPLMPlanes.h"
#include "XPLMMap.h"

// Standard C
#include <sys/stat.h>
#include <cmath>
#include <cstdarg>
#include <cassert>

// Standard C++
#include <string>
#include <list>
#include <map>
#include <array>
#include <vector>
#include <valarray>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <regex>
#include <bitset>
#include <future>

// XPlaneMP 2 - Internal Header Files
#include "Utilities.h"
#include "RelatedDoc8643.h"
#include "CSLModels.h"
#include "Aircraft.h"
#include "2D.h"
#include "AIMultiplayer.h"
#include "Map.h"

// On Windows, 'max' and 'min' are defined macros in conflict with C++ library. Let's undefine them!
#if IBM
#include <direct.h>
#undef max
#undef min
#endif

//
// MARK: Global Configurations and variables
//

#define UNKNOWN_PLUGIN_NAME "(unknown)"

namespace XPMP2 {

/// Stores the function and refcon pointer for plane creation/destrcution notifications
struct XPMPPlaneNotifierTy {
    XPMPPlaneNotifier_f func    = nullptr;
    void*               refcon  = nullptr;
    
    XPMPPlaneNotifierTy (XPMPPlaneNotifier_f _func = nullptr, void* _refcon = nullptr) :
    func(_func), refcon(_refcon) {}
    
    bool operator == (const XPMPPlaneNotifierTy& o)
    { return func == o.func && refcon == o.refcon; }
};

typedef std::list<XPMPPlaneNotifierTy> listXPMPPlaneNotifierTy;

/// Send a notification to all observers
void XPMPSendNotification (const Aircraft& plane, XPMPPlaneNotification _notification);

/// All global config settings and variables are kept in one structure for convenient access and central definition
struct GlobVars {
public:
    /// Logging level
    logLevelTy      logLvl      = logINFO;
    /// Debug model matching?
    bool            bLogMdlMatch= false;
    /// Clamp all planes to the ground? Default is `false` as clamping is kinda expensive due to Y-Testing.
    bool            bClampAll   = false;
    /// Name of the plugin we are serving (used as map layer name or for folders)
    std::string     pluginName  = UNKNOWN_PLUGIN_NAME;
    /// Plugin acronym used in log output
    std::string     logAcronym  = UNKNOWN_PLUGIN_NAME;

    /// Configuration callback for integer values
    int (*prefsFuncInt)(const char *, const char *, int) = XPMP2::PrefsFuncIntDefault;
    /// List of notifier functions registered for being notified of creation/destruction/model change
    listXPMPPlaneNotifierTy listObservers;
    
    /// Path to Doc8643.txt file
    std::string     pathDoc8643;
    /// Content of `Doc8643.txt` file
    mapDoc8643Ty    mapDoc8643;
    /// Path to related.txt file
    std::string     pathRelated;
    /// Content of `related.txt` file as a map of type codes to group id
    mapRelatedTy    mapRelated;

    /// Global map of all CSL Packages, indexed by `xsb_aircraft.txt::EXPORT_NAME`
    mapCSLPackageTy mapCSLPkgs;
    /// Global map of all CSL Models, indexed by related group, aircraft type, and model id
    mapCSLModelTy   mapCSLModels;
    /// Default ICAO aircraft type designator if no match can be found
    std::string     defaultICAO = "A320";
    /// Ground vehicle type identifier (map decides icon based on this)
    std::string     carIcaoType = "ZZZC";
    /// Resource directory, to store local definitions of vertical offsets (clamping)
    std::string     resourceDir;
    
    /// Global map of all created planes
    mapAcTy         mapAc;
    /// Shall we draw aircraft labels?
    bool            bDrawLabels = true;
    /// Maximum distance for drawing labels? [m]
    float           maxLabelDist = 5000.0f;
    /// Label font scaling factor
    float           labelFontScaling = 1.0f;
    
    /// Do we control X-Plane's AI/Multiplayer planes?
    bool            bHasControlOfAIAircraft = false;
    
    /// Do we feed X-Plane's maps with our aircraft positions?
    bool            bMapEnabled = true;
    /// Do we show labels with the aircraft icons?
    bool            bMapLabels = true;
    /// Map of map layer ids, i.e. for each map this is the id of the corresponding layer of ours
    mapMapLayerIDTy mapMapLayers;
    /// path to file containing plane icons for map display
    std::string     pathMapIcons;
    
    /// X-Plane's version number (XPLMGetVersions)
    int             verXPlane = -1;
    /// XPLM's SDK version number (XPLMGetVersions)
    int             verXPLM = -1;
    /// Using a modern graphics driver, ie. Vulkan/Metal?
    bool            bUsingModernGraphicsDriver = false;
    
#ifdef DEBUG
    /// Current XP cycle number (see XPLMGetCycleNumber())
    int             xpCycleNum = 0;
#define UPDATE_CYCLE_NUM glob.xpCycleNum=XPLMGetCycleNumber();
#else
#define UPDATE_CYCLE_NUM
#endif
    
protected:
    /// Current plane ID counter
    XPMPPlaneID  planeId = 0;
public:
    /// Get the next unique artifical plane id
    XPMPPlaneID NextPlaneId ();

public:
    /// Constructor
    GlobVars (logLevelTy _logLvl = logINFO, bool _logMdlMatch = false) :
    logLvl(_logLvl), bLogMdlMatch(_logMdlMatch) {}
    /// Update all settings, e.g. for logging level, by calling prefsFuncInt
    void UpdateCfgVals ();
    /// Read version numbers into verXplane/verXPLM
    void ReadVersions ();
    /// /// Using a modern graphics driver, ie. Vulkan/Metal?
    bool UsingModernGraphicsDriver() const { return bUsingModernGraphicsDriver; }
};

/// The one and only global variable structure
extern GlobVars glob;

}       // namespace XPMP2



#endif
