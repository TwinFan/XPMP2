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

//
// MARK: Constants
//

constexpr const char* REMOTE_CLIENT_NAME    =  "XPMP2 Remote Client";
constexpr const char* REMOTE_CLIENT_SHORT   =  "XPMP2_RC";
constexpr float REMOTE_CLIENT_VER           = 0.01f;

//
// MARK: Globals
//

/// Holds all global variables
struct XPMP2RCGlobals {
    /// What's the plugin's status?
    enum RCStatusTy {
        RCSTATUS_INACTIVE   = 0,        ///< not running, not sending, not listening
        RCSTATUS_WAITING,               ///< sending our beacon of interest, but not receiving data yet
        RCSTATUS_RECEIVING,             ///< actually receiving traffic data
    } Status = RCSTATUS_INACTIVE;       ///< Current plugin status
    
    logLevelTy logLvl = logINFO;        ///< Logging level
};

/// the one and only instance of XPMP2RCGlobals
extern XPMP2RCGlobals glob;
