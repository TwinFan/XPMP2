/// @file       Utilities.cpp
/// @brief      Miscellaneous utility functions, including logging
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

#include "XPMP2-Remote.h"

/// The one global variable object
XPMP2RCGlobals rcGlob;

//
// MARK: Misc
//

// Get total running time from X-Plane (sim/time/total_running_time_sec)
float GetMiscNetwTime()
{
    // must not use dataRefs from worker threads
    if (!rcGlob.IsXPThread())
        return rcGlob.now;
    
    // In main thread we can fetch a new time
    static XPLMDataRef drMiscNetwTime = XPLMFindDataRef("sim/network/misc/network_time_sec");
    return rcGlob.now = XPLMGetDataf(drMiscNetwTime);
}

// Convenience function to check on something at most every x seconds
bool CheckEverySoOften (float& _lastCheck, float _interval, float _now)
{
    if (_lastCheck < 0.00001f ||
        _now >= _lastCheck + _interval) {
        _lastCheck = _now;
        return true;
    }
    return false;
}

// Return a plugin's name
std::string GetPluginName (XPLMPluginID who)
{
    char whoName[256];
    XPLMGetPluginInfo(who, whoName, nullptr, nullptr, nullptr);
    return whoName;
}

//
// MARK: String functions
//

// Copy _at most_ `n` chars from location, or less if zero-terminated.
std::string str_n (const char* s, size_t max)
{
    // find end of string, stopping at max
    size_t i = 0;
    for (; i < max && *(s+i) != '\0'; ++i);
    return std::string(s,i);
}


//
//MARK: Log
//

#if IBM
#define PATH_DELIM_STD '\\'
#else
#define PATH_DELIM_STD '/'
#endif

const char* LOG_LEVEL[] = {
    "DEBUG", "INFO ", "WARN ", "ERROR", "FATAL", "MSG  "
};

// returns ptr to static buffer filled with log string
const char* LogGetString (const char* szPath, int ln, const char* szFunc,
                          logLevelTy lvl, const char* szMsg, va_list args )
{
     static char aszMsg[2048];
     float runS = GetMiscNetwTime();
     const unsigned runH = unsigned(runS / 3600.0f);
     runS -= runH * 3600.0f;
     const unsigned runM = unsigned(runS / 60.0f);
     runS -= runM * 60.0f;

    // prepare timestamp
    if ( lvl < logMSG )                             // normal messages without, all other with location info
    {
        const char* szFile = strrchr(szPath, PATH_DELIM_STD);  // extract file from path
        if ( !szFile ) szFile = szPath; else szFile++;
        snprintf(aszMsg, sizeof(aszMsg), "%u:%02u:%06.3f %s %s %s:%d/%s: ",
                 runH, runM, runS,                  // Running time stamp
                 REMOTE_CLIENT_LOG, LOG_LEVEL[lvl],
                 szFile, ln, szFunc);
    }
    else
        snprintf(aszMsg, sizeof(aszMsg), "%u:%02u:%06.3f %s: ",
                 runH, runM, runS,                  // Running time stamp
                 REMOTE_CLIENT_LOG);
    
    // append given message
    if (args) {
        vsnprintf(&aszMsg[strlen(aszMsg)],
                  sizeof(aszMsg)-strlen(aszMsg)-1,      // we save one char for the CR
                  szMsg,
                  args);
    }

    // ensure there's a trailing CR
    size_t l = strlen(aszMsg);
    if ( aszMsg[l-1] != '\n' )
    {
        aszMsg[l]   = '\n';
        aszMsg[l+1] = 0;
    }

    // return the static buffer
    return aszMsg;
}

void LogMsg ( const char* szPath, int ln, const char* szFunc, logLevelTy lvl, const char* szMsg, ... )
{
    va_list args;
    
    va_start (args, szMsg);
    // write to log (flushed immediately -> expensive!)
    XPLMDebugString ( LogGetString(szPath, ln, szFunc, lvl, szMsg, args) );
    va_end (args);
}
