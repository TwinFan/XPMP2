/// @file       Utilities.h
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

#ifndef _Utilities_h_
#define _Utilities_h_

namespace XPMP2 {

//
// MARK: General texts
//

#define ERR_ASSERT              "ASSERT FAILED: %s"
#define WARN_RSRC_DIR_UNAVAIL   "Resource directory '%s' does not exist!"
#define WARN_MODEL_NOT_FOUND    "Named CSL Model '%s' not found"

// Config items read via preferences callback functions
#define CFG_SEC_PLANES          "planes"
#define CFG_ITM_CLAMPALL        "clamp_all_to_ground"
#define CFG_ITM_DR_LIBXPLANEMP  "dr_libxplanemp"

#define CFG_SEC_DEBUG           "debug"
#define CFG_ITM_LOGLEVEL        "log_level"
#define CFG_ITM_MODELMATCHING   "model_matching"

//
// MARK: Default configuration callbacks
//

int     PrefsFuncIntDefault     (const char *, const char *, int _default);
float   PrefsFuncFloatDefault   (const char *, const char *, float _default);

//
// MARK: File access helpers
//

/// Does a file path exist?
bool ExistsFile (const std::string& filename);

//
// MARK: Logging Support
//

/// Logging level
enum logLevelTy {
    logDEBUG = 0,       ///< Debug, highest level of detail
    logINFO,            ///< regular info messages
    logWARN,            ///< warnings, i.e. unexpected, but uncritical events, maybe leading to unwanted display, but still: display of aircraft
    logERR,             ///< errors mean, aircraft can potentially not be displayed
    logFATAL,           ///< fatal is shortly before a crash
    logMSG              ///< will always be output, no matter what has been configured, cannot be suppressed
};

/// Returns ptr to static buffer filled with formatted log string
const char* LogGetString ( const char* szFile, int ln, const char* szFunc, logLevelTy lvl, const char* szMsg, va_list args );
             
/// Log Text to log file
void LogMsg ( const char* szFile, int ln, const char* szFunc, logLevelTy lvl, const char* szMsg, ... );

//
// MARK: XPlaneMP2 Exception class
//
class XPMP2Error : public std::logic_error {
protected:
    std::string fileName;
    int ln;
    std::string funcName;
    logLevelTy lvl;
    std::string msg;
public:
    XPMP2Error (const char* szFile, int ln, const char* szFunc, logLevelTy lvl,
             const char* szMsg, ...);
protected:
    XPMP2Error (const char* szFile, int ln, const char* szFunc, logLevelTy lvl);
public:
    // returns msg.c_str()
    virtual const char* what() const noexcept;
    
public:
    // copy/move constructor/assignment as per default
    XPMP2Error (const XPMP2Error& o) = default;
    XPMP2Error (XPMP2Error&& o) = default;
    XPMP2Error& operator = (const XPMP2Error& o) = default;
    XPMP2Error& operator = (XPMP2Error&& o) = default;
};

//
// MARK: Logging macros
//

/// @brief Log a message if lvl is greater or equal currently defined log level
/// @note First parameter after lvl must be the message text,
///       which can be a format string with its parameters following like in sprintf
#define LOG_MSG(lvl,...)  {                                         \
    if (lvl >= glob.logLvl)                                             \
    {LogMsg(__FILE__, __LINE__, __func__, lvl, __VA_ARGS__);}       \
}

/// @brief Throws an exception using XPMP2Error
/// @note First parameter after lvl must be the message text,
///       which can be a format string with its parameters following like in sprintf
#define THROW_ERROR(lvl,...)                                        \
throw XPMP2Error(__FILE__, __LINE__, __func__, lvl, __VA_ARGS__);

/// @brief Throw in an assert-style (logging takes place in XPMP2Error constructor)
/// @note This conditional check _always_ takes place, independend of any build or logging settings!
#define LOG_ASSERT(cond)                                            \
    if (!(cond)) {                                                  \
        THROW_ERROR(logFATAL,ERR_ASSERT,#cond);                     \
    }

//
// MARK: Compiler differences
//

#if APL == 1 || LIN == 1
// these simulate the VC++ version, not the C standard versions!
inline struct tm *gmtime_s(struct tm * result, const time_t * time)
{ return gmtime_r(time, result); }
#endif

}

#endif
