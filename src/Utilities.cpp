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

#include "xplanemp2.h"

namespace XPMP2 {

// The one and only global variable structure
GlobVars glob
#if DEBUG
               (logDEBUG);
#else
               ();
#endif

//
// MARK: Configuration
//

/// Default config function just always returns the provided default value
int     PrefsFuncIntDefault     (const char *, const char *, int _default)
{
    return _default;
}

/// Default config function just always returns the provided default value
float   PrefsFuncFloatDefault   (const char *, const char *, float _default)
{
    return _default;
}

// Update setting for logging level by calling prefsFuncInt
void GlobVars::ReadLogLvl ()
{
    LOG_ASSERT(prefsFuncInt);
    
    // Ask for logging level
    int i = prefsFuncInt(CFG_SEC_DEBUG, CFG_ITM_LOGLEVEL,
#if DEBUG
                         logDEBUG
#else
                         logWARN
#endif
                         );
    if (logDEBUG <= i && i <= logINFO)
        logLvl = logLevelTy(i);
    
    // Ask for model matching logging
    bLogMdlMatch = prefsFuncInt(CFG_SEC_DEBUG, CFG_ITM_MODELMATCHING, 0) != 0;
}

//
// MARK: File access helpers
//

// Does a file path exist? (in absence of <filesystem>, which XCode refuses to ship)
/// @see https://stackoverflow.com/a/51301928
bool ExistsFile (const std::string& filename)
{
    struct stat buffer;
    return (stat (filename.c_str(), &buffer) == 0);
}

//
// MARK: LiveTraffic Exception classes
//

// standard constructor
XPMP2Error::XPMP2Error (const char* _szFile, int _ln, const char* _szFunc,
                  logLevelTy _lvl,
                  const char* _szMsg, ...) :
std::logic_error(LogGetString(_szFile, _ln, _szFunc, _lvl, _szMsg, NULL)),
fileName(_szFile), ln(_ln), funcName(_szFunc),
lvl(_lvl)
{
    va_list args;
    va_start (args, _szMsg);
    msg = LogGetString(_szFile, _ln, _szFunc, _lvl, _szMsg, args);
    va_end (args);
    
    // write to log (flushed immediately -> expensive!)
    if (_lvl >= glob.logLvl)
        XPLMDebugString ( msg.c_str() );
}

// protected constructor, only called by LTErrorFD
XPMP2Error::XPMP2Error (const char* _szFile, int _ln, const char* _szFunc,
                  logLevelTy _lvl) :
std::logic_error(LogGetString(_szFile, _ln, _szFunc, _lvl, "", NULL)),
fileName(_szFile), ln(_ln), funcName(_szFunc),
lvl(_lvl)
{}

const char* XPMP2Error::what() const noexcept
{
    return msg.c_str();
}

//
//MARK: Log
//

const char* LOG_LEVEL[] = {
    "DEBUG", "INFO ", "WARN ", "ERROR", "FATAL", "MSG  "
};

// returns ptr to static buffer filled with log string
const char* LogGetString (const char* szPath, int ln, const char* szFunc,
                          logLevelTy lvl, const char* szMsg, va_list args )
{
     static char aszMsg[2048];
    
    // current Zulu time
    struct tm zulu;
    std::time_t t = std::time(nullptr);
    char tZuluS[100];
    gmtime_s(&zulu, &t);
    std::strftime(tZuluS, sizeof(tZuluS), "%d-%b %T", &zulu);

    // prepare timestamp
    if ( lvl < logMSG )                             // normal messages without, all other with location info
    {
        const char* szFile = strrchr(szPath,'/');   // extract file from path
        if ( !szFile ) szFile = szPath; else szFile++;
        snprintf(aszMsg, sizeof(aszMsg), "%s/xplanemp2 %sZ %s %s:%d/%s: ",
                 XPMP_CLIENT_LONGNAME, tZuluS, LOG_LEVEL[lvl],
                 szFile, ln, szFunc);
    }
    else
        snprintf(aszMsg, sizeof(aszMsg), "%s/xplanemp2: ", XPMP_CLIENT_LONGNAME);
    
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

};

