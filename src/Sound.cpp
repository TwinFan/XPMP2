/// @file       Sound.cpp
/// @brief      Sound for aircraft, based on the FMOD library
/// @see        FMOD Core API Guide
///             https://fmod.com/docs/2.02/api/core-guide.html
/// @details    FMOD's C interface is used exclusively (and not the C++ ABI).
///             This is to ease handling of dynamic libaries between versions
///             (XP11 is using FMOD 1.x while XP12 has upgraded to 2.x)
///             and allows compiling with MingW.
/// @author     Birger Hoppe
/// @copyright  (c) 2022 Birger Hoppe
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

// FMOD header files only here in this module!
// This one includes everything
#include "fmod_errors.h"

namespace XPMP2 {

//
// MARK: Global Variables
//

/// FMOD system
static FMOD_SYSTEM* gpFmodSystem = nullptr;
static unsigned int gFmodVer = 0;

//
// MARK: FMOD Error Handling
//

/// Exception class to pass on error information
class FmodError : public std::runtime_error
{
public:
    FMOD_RESULT fmodRes;        ///< the actual FMOD result code
    int ln;                     ///< line number here in the code
    std::string sFunc;          ///< name of the function the error occurred in
public:
    /// Constructor taking on a descriptive string and the `FMOD_RESULT`
    FmodError (const std::string& _what, FMOD_RESULT _r,
               int _ln, const std::string& _sFunc) :
    std::runtime_error(leftOf(_what,"( ")),     // takes over the name of the function called, but not all the parameter string
    fmodRes(_r), ln(_ln), sFunc(_sFunc)
    {}
    
    /// Log myself to Log.txt as an error
    void LogErr () const {
        LogMsg(__FILE__, ln, sFunc.c_str(), logERR,
               "FMOD Error %d - '%s' in %s", fmodRes,
               FMOD_ErrorString(fmodRes),
               what());
    }
};

/// last FMOD result code, needed by the following macro, better don't rely on anywhere else
static FMOD_RESULT gFmodRes = FMOD_OK;

/// @brief Log an error if `fmodRes` is not `FMOD_OK`
#define FMOD_TEST(fmodCall)  {                                              \
    if ((gFmodRes = (fmodCall)) != FMOD_OK)                                 \
        throw FmodError(#fmodCall, gFmodRes, __LINE__, __func__);           \
}

/// @brief Standard catch clause to handle FmodError exceptions by just logging them
#define FMOD_CATCH catch (const FmodError& e) { e.LogErr(); }


//
// MARK: Global Functions
//

// Initialize the sound module and load the sounds
bool SoundInit ()
{
    // Shortcut if system already exists
    
    try {
        if (!gpFmodSystem) {
            FMOD_TEST(FMOD_System_Create(&gpFmodSystem, FMOD_VERSION));
            FMOD_TEST(FMOD_System_GetVersion(gpFmodSystem, &gFmodVer));
            LOG_MSG(logINFO, "Initialized FMOD version %u.%u.%u",
                    gFmodVer >> 16, (gFmodVer & 0x0000ff00) >> 8,
                    gFmodVer & 0x000000ff);
        }
        glob.bSoundAvail = true;
    }
    catch (const FmodError& e) {
        e.LogErr();                             // log the error
        SoundCleanup();                         // cleanup
    }
    
    return false;
}

// Graceful shoutdown
void SoundCleanup ()
{
    if (gpFmodSystem)
        FMOD_System_Release(gpFmodSystem);
    gpFmodSystem = nullptr;
    gFmodVer = 0;
    glob.bSoundAvail = false;
}

}

//
// MARK: Public functions outside XPMP2 namespace
//

// Enable/Disable Sound
bool XPMPSoundEnable (bool bEnable)
{
    // Any change?
    if (bEnable != XPMP2::glob.bSoundAvail)
    {
        // Enable or disable?
        if (bEnable)
            XPMP2::SoundInit();
        else
            XPMP2::SoundCleanup();
    }
    return XPMPSoundIsEnabled();
}

// Is Sound enabled?
bool XPMPSoundIsEnabled ()
{
    return XPMP2::glob.bSoundAvail;
}

