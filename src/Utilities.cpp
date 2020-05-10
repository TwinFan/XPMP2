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

#include "XPMP2.h"

namespace XPMP2 {


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#if DEBUG
GlobVars glob (logDEBUG, true);
#else
/// The one and only global variable structure (accepted to require an exit-time destructor)
GlobVars glob;
#endif
#pragma clang diagnostic pop

//
// MARK: Configuration
//

/// Default config function just always returns the provided default value
int     PrefsFuncIntDefault     (const char *, const char *, int _default)
{
    return _default;
}

// Update all config values, e.g. for logging level, by calling prefsFuncInt
void GlobVars::UpdateCfgVals ()
{
    // Let's do the update only about every 2 seconds
    static float timLstCheck = 0.0f;
    if (!CheckEverySoOften(timLstCheck, 2.0f))
        return;
    
    LOG_ASSERT(prefsFuncInt);
    
    // Ask for logging level
    int i = prefsFuncInt(XPMP_CFG_SEC_DEBUG, XPMP_CFG_ITM_LOGLEVEL, logLvl);
    if (logDEBUG <= i && i <= logINFO)
        logLvl = logLevelTy(i);
    
    // Ask for model matching logging
    bLogMdlMatch = prefsFuncInt(XPMP_CFG_SEC_DEBUG, XPMP_CFG_ITM_MODELMATCHING, bLogMdlMatch) != 0;
    
    // Ask for clam-to-ground config
    bClampAll = prefsFuncInt(XPMP_CFG_SEC_PLANES, XPMP_CFG_ITM_CLAMPALL, bClampAll) != 0;    
}

// Read version numbers into verXplane/verXPLM
void GlobVars::ReadVersions ()
{
    XPLMHostApplicationID hostID = -1;
    XPLMGetVersions(&glob.verXPlane, &glob.verXPLM, &hostID);
    
    // Also read if we are using Vulkan/Metal
    XPLMDataRef drUsingModernDriver = XPLMFindDataRef("sim/graphics/view/using_modern_driver");
    if (drUsingModernDriver)            // dataRef not defined before 11.50
        bUsingModernGraphicsDriver = XPLMGetDatai(drUsingModernDriver) != 0;
    else
        bUsingModernGraphicsDriver = false;
}

//
// MARK: File access helpers
//

// Windows is missing a few simple macro definitions
#if !defined(S_ISDIR)
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif

// Does a file path exist? (in absence of <filesystem>, which XCode refuses to ship)
/// @see https://stackoverflow.com/a/51301928
bool ExistsFile (const std::string& filename)
{
    struct stat buffer;
    return (stat (filename.c_str(), &buffer) == 0);
}

// Is path a directory?
bool IsDir (const std::string& path)
{
    struct stat buffer;
    if (stat (path.c_str(), &buffer) != 0)  // get stats...error?
        return false;                       // doesn't exist...no directory either
    return S_ISDIR(buffer.st_mode);         // check for S_IFDIR mode flag
}

// Create directory if it does not exist
bool CreateDir(const std::string& path)
{
    // Just try creating it...
#if IBM
    if (_mkdir(path.c_str()) == 0)
#else
    if (mkdir(path.c_str(), 0775) == 0)
#endif
        return true;

    // If creation failed, directory might still have already existed
    return IsDir(path);
}

// Copy file if source is newer or destination missing
bool CopyFileIfNewer(const std::string& source, const std::string& destDir)
{
    // Check source
    struct stat bufSource;
    if (stat(source.c_str(), &bufSource) != 0)
        // source doesn't exist
        return false;               
    size_t nPos = source.find_last_of("/\\");       // find beginning of file name
    if (nPos == std::string::npos)
        return false;

    // Check destination directory
    if (destDir.empty())
        return false;
    std::string target = destDir;
    if (target.back() == '\\' ||
        target.back() == '/')                       // remove traling (back)slash
        target.pop_back();
    if (!CreateDir(target))
        // directory does not exist and could be be created
        return false;

    // Check target file
    target += source.substr(nPos);
    struct stat bufTarget;
    if (stat(target.c_str(), &bufTarget) == 0)                  // file exists?
    {
        // is target not older than source?
        if (bufTarget.st_mtime >= bufSource.st_mtime)
            // Skip copy, all good already
            return true;
    }

    // Target dir exists, target file does not or is older -> Do copy!
    // https://stackoverflow.com/a/10195497
    std::ifstream src(source, std::ios::binary);
    std::ofstream dst(target, std::ios::binary | std::ios::trunc);
    if (!src || !dst)
        return false;
    dst << src.rdbuf();

    // all good?
    if (src.good() && dst.good()) {
        LOG_MSG(logINFO, "Copied %s to %s", source.c_str(), destDir.c_str());
        return true;
    }
    else {
        LOG_MSG(logERR, "FAILED copying %s to %s", source.c_str(), destDir.c_str());
        return true;
    }
}

// List of files in a directory (wrapper around XPLMGetDirectoryContents)
std::list<std::string> GetDirContents (const std::string& path)
{
    std::list<std::string> l;               // the list to be returned
    char szNames[4048];                     // buffer for file names
    char* indices[256];                     // buffer for indices to beginnings of names
    int start = 0;                          // first file to return
    int numFiles = 0;                       // number of files returned (per batch)
    bool bFinished = false;
    
    // Call XPLMGetDirectoryContents as often as needed to read all directory content
    do {
        numFiles = 0;
        bFinished = XPLMGetDirectoryContents(path.c_str(),
                                             start,
                                             szNames, sizeof(szNames),
                                             indices, sizeof(indices)/sizeof(*indices),
                                             NULL, &numFiles);
        // process (the batch of) files we received now
        for (int i = 0; i < numFiles; ++i)
            if (indices[i][0] != '.')           // skip parent_dir and hidden entries
                l.push_back(indices[i]);
        // next batch start (if needed)
        start += numFiles;
    } while(!bFinished);
    
    // return the list of files
    return l;
}

/// @details Read a text line, handling both Windows (CRLF) and Unix (LF) ending
/// Code makes use of the fact that in both cases LF is the terminal character.
/// So we read from file until LF (_without_ widening!).
/// In case of CRLF files there then is a trailing CR, which we just remove.
std::istream& safeGetline(std::istream& is, std::string& t)
{
    // read a line until LF
    std::getline(is, t, '\n');
    
    // if last character is CR then remove it
    if (!t.empty() && t.back() == '\r')
        t.pop_back();
    
    return is;
}

//
// MARK: HFS-to-Posix path conversion (Mac only)
//

#if APL

#include <Carbon/Carbon.h>

// Funnily, even Apple deprecated HFS style...we need to ignore that warning here
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

/// Convert a path from HFS (colon separated) to Posix (slash separated)
bool HFS2PosixPath(const char *path, char *result, int resultLen);
/// Convert a path from Posix (slash separated) to HFS (colon separated)
bool Posix2HFSPath(const char *path, char *result, int resultLen);

/// A simple smart pointer structure, which makes sure to correctly release a CF pointer
template <typename T>
struct CFSmartPtr {
    CFSmartPtr(T p) : p_(p) {                          }
    ~CFSmartPtr()             { if (p_) CFRelease(p_); }
    operator T ()             { return p_; }
    T p_;
};


bool HFS2PosixPath(const char *path, char *result, int resultSize)
{
    bool is_dir = (path[strlen(path)-1] == ':');

    CFSmartPtr<CFStringRef>        inStr(CFStringCreateWithCString(kCFAllocatorDefault, path ,kCFStringEncodingMacRoman));
    if (inStr == NULL) return false;
    
    CFSmartPtr<CFURLRef>        url(CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inStr, kCFURLHFSPathStyle,0));
    if (url == NULL) return false;
    
    CFSmartPtr<CFStringRef>        outStr(CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle));
    if (outStr == NULL) return false;
    
    if (!CFStringGetCString(outStr, result, resultSize, kCFStringEncodingMacRoman))
        return false;

    if(is_dir) strcat(result, "/");

    return true;
}

bool Posix2HFSPath(const char *path, char *result, int resultSize)
{
    CFSmartPtr<CFStringRef>        inStr(CFStringCreateWithCString(kCFAllocatorDefault, path ,kCFStringEncodingMacRoman));
    if (inStr == NULL) return false;
    
    CFSmartPtr<CFURLRef>        url(CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inStr, kCFURLPOSIXPathStyle,0));
    if (url == NULL) return false;
    
    CFSmartPtr<CFStringRef>        outStr(CFURLCopyFileSystemPath(url, kCFURLHFSPathStyle));
    if (outStr == NULL) return false;
    
    if (!CFStringGetCString(outStr, result, resultSize, kCFStringEncodingMacRoman))
        return false;

    return true;
}

// Checks how XPLM_USE_NATIVE_PATHS is set (recommended to use), and if not set converts the path from HFS to POSIX
std::string TOPOSIX (const std::string& p)
{
    // no actual conversion if XPLM_USE_NATIVE_PATHS is activated
    if (XPLMIsFeatureEnabled("XPLM_USE_NATIVE_PATHS"))
        return p;
    else {
        char posix[1024];
        if (HFS2PosixPath(p.c_str(), posix, sizeof(posix)))
            return posix;
        else
            return p;
    }
}

// Checks how XPLM_USE_NATIVE_PATHS is set (recommended to use), and if not set converts the path from POSIX to HFS
std::string FROMPOSIX (const std::string& p)
{
    // no actual conversion if XPLM_USE_NATIVE_PATHS is activated
    if (XPLMIsFeatureEnabled("XPLM_USE_NATIVE_PATHS"))
        return p;
    else {
        char hfs[1024];
        if (Posix2HFSPath(p.c_str(), hfs, sizeof(hfs)))
            return hfs;
        else
            return p;
    }
}

#pragma clang diagnostic pop

#endif

//
// MARK: String helpers
//

// change a string to lowercase
std::string& str_tolower(std::string& s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) -> unsigned char { return (unsigned char) tolower(c); });
    return s;
}

// separates string into tokens
std::vector<std::string> str_tokenize (const std::string s,
                                       const std::string tokens,
                                       bool bSkipEmpty)
{
    std::vector<std::string> v;
 
    // find all tokens before the last
    size_t b = 0;                                   // begin
    for (size_t e = s.find_first_of(tokens);        // end
         e != std::string::npos;
         b = e+1, e = s.find_first_of(tokens, b))
    {
        if (!bSkipEmpty || e != b)
            v.emplace_back(s.substr(b, e-b));
    }
    
    // add the last one: the remainder of the string (could be empty!)
    v.emplace_back(s.substr(b));
    
    return v;
}

//
// MARK: Math helpers
//

// (Shortest) difference between 2 angles: How much to turn to go from h1 to h2?
float headDiff (float head1, float head2)
{
    // if either value is nan return nan
    if (std::isnan(head1) || std::isnan(head2)) return NAN;
    
    // if 0° North lies between head1 and head2 then simple
    // diff doesn't work
    if ( std::abs(head2-head1) > 180 ) {
        // add 360° to the lesser value...then diff works
        if ( head1 < head2 )
            head1 += 360;
        else
            head2 += 360;
    }
    
    return head2 - head1;
}

//
// MARK: Misc
//

// Get total running time from X-Plane (sim/time/total_running_time_sec)
float GetMiscNetwTime()
{
    static XPLMDataRef drMiscNetwTime = nullptr;
    if (!drMiscNetwTime)
        drMiscNetwTime = XPLMFindDataRef("sim/network/misc/network_time_sec");
    return XPLMGetDataf(drMiscNetwTime);
}

// Text string for current graphics driver in use
const char* GetGraphicsDriverTxt ()
{
    if (glob.UsingModernGraphicsDriver())
#if APL
        return "Metal";
#else
        return "Vulkan";
#endif
    else
        return "OpenGL";
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

//
// MARK: LiveTraffic Exception classes
//

// standard constructor
XPMP2Error::XPMP2Error (const char* _szFile, int _ln, const char* _szFunc,
                        const char* _szMsg, ...) :
std::logic_error(LogGetString(_szFile, _ln, _szFunc, logFATAL, _szMsg, NULL)),
fileName(_szFile), ln(_ln), funcName(_szFunc)
{
    va_list args;
    va_start (args, _szMsg);
    msg = LogGetString(_szFile, _ln, _szFunc, logFATAL, _szMsg, args);
    va_end (args);
    
    // write to log (flushed immediately -> expensive!)
    if (logFATAL >= glob.logLvl)
        XPLMDebugString ( msg.c_str() );
}

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
        snprintf(aszMsg, sizeof(aszMsg), "%u:%02u:%06.3f %s/XPMP2 %s %s:%d/%s: ",
                 runH, runM, runS,                  // Running time stamp
                 glob.logAcronym.c_str(), LOG_LEVEL[lvl],
                 szFile, ln, szFunc);
    }
    else
        snprintf(aszMsg, sizeof(aszMsg), "%u:%02u:%06.3f %s/XPMP2: ",
                 runH, runM, runS,                  // Running time stamp
                 glob.logAcronym.c_str());
    
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

}       // namespace XPMP2
