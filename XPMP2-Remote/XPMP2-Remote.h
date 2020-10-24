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

// Standard C++ headers
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include <memory>

// X-Plane SDK
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "XPLMPlugin.h"
#include "XPLMMenus.h"
#include "XPLMProcessing.h"

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
    
    /// Config values reconciled from sending plugins
    XPMP2::RemoteMsgSettingsTy mergedS;

    /// The global map of all sending plugins we've ever heard of
    mapSenderTy gmapSender;
    
    /// Latest timestamp read from network_time_sec
    float now = 0.0f;
    
    ///< id of X-Plane's thread (when it is OK to use XP API calls)
    std::thread::id xpThread;
};

/// the one and only instance of XPMP2RCGlobals
extern XPMP2RCGlobals rcGlob;
