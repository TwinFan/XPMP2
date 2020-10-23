/// @file       XPMP2-Remote.h
/// @brief      XPMP2 Remote Client: Displays aircraft served from other XPMP2-based plugins in the network
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

#pragma once

// Standard C headers
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cmath>

// X-Plane SDK
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "XPLMPlugin.h"
#include "XPLMGraphics.h"
#include "XPLMMenus.h"

// Include XPMP2 headers
#include "XPMPAircraft.h"
#include "XPMPMultiplayer.h"
#include "XPMPRemote.h"

// Include XPMP2-Remote headers
#include "Utilities.h"
#include "Client.h"

//
// MARK: Constants
//

constexpr const char* REMOTE_CLIENT_NAME    =  "XPMP2 Remote Client";   ///< Plugin name
constexpr const char* REMOTE_CLIENT_LOG     =  "XPMP2_RC";              ///< ID used in own log entries
constexpr const char* REMOTE_CLIENT_LOG2    =  "RC";                    ///< Short ID used in XPMP2 log entries
constexpr float REMOTE_CLIENT_VER           = 0.01f;

//
// MARK: Globals
//

/// Holds all global variables
struct XPMP2RCGlobals {
    
    // *** Config values reconciled from sending plugins ***
    
    /// Logging level
    logLevelTy      logLvl      = logINFO;
    /// Debug model matching?
    bool            bLogMdlMatch= false;
    /// Replace dataRefs in `.obj` files on load? (defaults to OFF!)
    bool            bObjReplDataRefs = false;
    /// Replace textures in `.obj` files on load if needed?
    bool            bObjReplTextures = true;
    
    /// Default ICAO aircraft type designator if no match can be found
    std::string     defaultICAO = "A320";
    /// Ground vehicle type identifier (map decides icon based on this)
    std::string     carIcaoType = "ZZZC";
    
    /// Shall we draw aircraft labels?
    bool            bDrawLabels = true;
    /// Maximum distance for drawing labels? [m], defaults to 3nm
    float           maxLabelDist = 5556.0f;
    /// Cut off labels at XP's reported visibility mit?
    bool            bLabelCutOffAtVisibility = true;
    
    /// Do we feed X-Plane's maps with our aircraft positions?
    bool            bMapEnabled = true;
    /// Do we show labels with the aircraft icons?
    bool            bMapLabels = true;

};

/// the one and only instance of XPMP2RCGlobals
extern XPMP2RCGlobals glob;
