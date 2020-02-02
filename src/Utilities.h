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

/// Is path a directory?
bool IsDir (const std::string& path);

/// List of files in a directory (wrapper around XPLMGetDirectoryContents)
std::list<std::string> GetDirContents (const std::string& path);

/// Read a line from a text file, no matter if ending on CRLF or LF
std::istream& safeGetline(std::istream& is, std::string& t);

//
// MARK: String helpers
//

#define WHITESPACE              " \t\f\v\r\n"

/// change a std::string to uppercase
std::string& str_tolower(std::string& s);

/// @brief trimming of string (from right)
/// @see https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
inline std::string& rtrim(std::string& s, const char* t = WHITESPACE)
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

/// @brief trimming of string (from left)
/// @see https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
inline std::string& ltrim(std::string& s, const char* t = WHITESPACE)
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

/// @brief trimming of string (from both ends)
/// @see https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
inline std::string& trim(std::string& s, const char* t = WHITESPACE)
{
    return ltrim(rtrim(s, t), t);
}

/// separates string into tokens
std::vector<std::string> str_tokenize (const std::string s,
                                       const std::string tokens,
                                       bool bSkipEmpty = true);

//
// MARK: Math helpers
//

/// Square
template <class T>
inline T sqr (const T a) { return a*a; }

/// Pythagorean distance between two points in a 3-D world
template <class T>
inline T dist (const T x1, const T y1, const T z1,
               const T x2, const T y2, const T z2)
{
    return std::sqrt(sqr(x1-x2) + sqr(y1-y2) + sqr(z1-z2));
}

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
// MARK: Logging macros
//

/// @brief Log a message if lvl is greater or equal currently defined log level
/// @note First parameter after lvl must be the message text,
///       which can be a format string with its parameters following like in sprintf
#define LOG_MSG(lvl,...)  {                                         \
    if (lvl >= glob.logLvl)                                         \
    {LogMsg(__FILE__, __LINE__, __func__, lvl, __VA_ARGS__);}       \
}

/// @brief Log a message about matching if logging of model matching is enabled
/// @note First parameter after lvl must be the message text,
///       which can be a format string with its parameters following like in sprintf
#define LOG_MATCHING(lvl,...)  {                                    \
    if (glob.bLogMdlMatch && lvl >= glob.logLvl)                                          \
    {LogMsg(__FILE__, __LINE__, __func__, lvl, __VA_ARGS__);}       \
}

/// @brief Throws an exception using XPMP2Error
/// @note First parameter after lvl must be the message text,
///       which can be a format string with its parameters following like in sprintf
#define THROW_ERROR(...)                                            \
throw XPMP2Error(__FILE__, __LINE__, __func__, __VA_ARGS__);

/// @brief Throw in an assert-style (logging takes place in XPMP2Error constructor)
/// @note This conditional check _always_ takes place, independend of any build or logging settings!
#define LOG_ASSERT(cond)                                            \
    if (!(cond)) {                                                  \
        THROW_ERROR(ERR_ASSERT,#cond);                              \
    }

//
// MARK: Compiler differences
//

#if APL == 1 || LIN == 1
// not quite the same but close enough for our purposes
inline void strcpy_s(char * dest, size_t destsz, const char * src)
{ strncpy(dest, src, destsz); dest[destsz-1]=0; }
inline void strcat_s(char * dest, size_t destsz, const char * src)
{ strncat(dest, src, destsz - strlen(dest) - 1); }

// these simulate the VC++ version, not the C standard versions!
inline struct tm *gmtime_s(struct tm * result, const time_t * time)
{ return gmtime_r(time, result); }
inline struct tm *localtime_s(struct tm * result, const time_t * time)
{ return localtime_r(time, result); }

#endif

/// Simpler access to strcpy_s if dest is a char array (not a pointer!)
#define STRCPY_S(dest,src) strcpy_s(dest,sizeof(dest),src)
#define STRCPY_ATMOST(dest,src) strcpy_s(dest,sizeof(dest),strAtMost(src,sizeof(dest)-1).c_str())

#if APL == 1
// XCode/Linux don't provide the _s functions, not even with __STDC_WANT_LIB_EXT1__ 1
inline int strerror_s( char *buf, size_t bufsz, int errnum )
{ return strerror_r(errnum, buf, bufsz); }
#endif
#if LIN == 1
inline int strerror_s( char *buf, size_t bufsz, int errnum )
{ strcpy_s(buf,bufsz,strerror(errnum)); return 0; }
#endif

// In case of Mac we need to prepare for HFS-to-Posix path conversion
#if APL
/// Checks how XPLM_USE_NATIVE_PATHS is set (recommended to use), and if not set converts the path
std::string TOPOSIX (const std::string& p);
#else
/// On Lin/Win there is no need for a conversion, but we do treat `p` now as `std::string`
inline std::string TOPOSIX (const std::string& p) { return p; }
#endif

}

#endif
