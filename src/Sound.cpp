/// @file       Sound.cpp
/// @brief      Sound for aircraft, based on the FMOD library
///
/// @note       Audio Engine is FMOD Core API by Firelight Technologies Pty Ltd.
///             Understand FMOD [licensing](https://www.fmod.com/licensing) and
///             [attribution requirements](https://www.fmod.com/attribution) first!\n
///             Sound support is only included if built with CMake cache entry `INCLUDE_FMOD_SOUND`.\n
///
/// @see        FMOD Core API Guide
///             https://fmod.com/docs/2.02/api/core-guide.html
/// @details    FMOD's C interface is used exclusively (and not the C++ ABI).
///             This is to ease handling of dynamic libaries between versions
///             (XP11 is using FMOD 1.x while XP12 has upgraded to 2.x)
///             and allows compiling with MingW.
/// @details    Some functionality looks like immitating FMOD's SoundGroup,
///             but 3D positioning was unstable when used via SoundGroup,
///             and the cone functionality did not work at all,
///             so this file handles all sound channels individually.
/// @note       If  linking to the logging version of the FMOD API library
///             (the one ending in `L`) and specifying a
///             config item `log_level` of `0 = Debug`
///             (see ::XPMPIntPrefsFuncTy) while initializing sound,
///             ie. typically during the first call to XPMPMultiplayerInit(),
///             then FMOD logging output is added to X-Plane's `Log.txt`.
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

// Only if we want Sound Support
#ifdef INCLUDE_FMOD_SOUND

// FMOD header files only here in this module!
// This one includes everything
#include "fmod_errors.h"

namespace XPMP2 {

FMOD_VECTOR FmodHeadPitch2Vec (const float head, const float pitch);

//
// MARK: FMOD Old version Structures
//

/// FMOD 1.08.30 version of FMOD_ADVANCEDSETTINGS
struct FMOD_10830_ADVANCEDSETTINGS
{
    int                 cbSize;                     /* [w]   Size of this structure.  Use sizeof(FMOD_ADVANCEDSETTINGS)  NOTE: This must be set before calling System::getAdvancedSettings or System::setAdvancedSettings! */
    int                 maxMPEGCodecs;              /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_CREATECOMPRESSEDSAMPLE only.  MPEG   codecs consume 22,216 bytes per instance and this number will determine how many MPEG   channels can be played simultaneously. Default = 32. */
    int                 maxADPCMCodecs;             /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_CREATECOMPRESSEDSAMPLE only.  ADPCM  codecs consume  2,480 bytes per instance and this number will determine how many ADPCM  channels can be played simultaneously. Default = 32. */
    int                 maxXMACodecs;               /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_CREATECOMPRESSEDSAMPLE only.  XMA    codecs consume  6,263 bytes per instance and this number will determine how many XMA    channels can be played simultaneously. Default = 32. */
    int                 maxVorbisCodecs;            /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_CREATECOMPRESSEDSAMPLE only.  Vorbis codecs consume 16,512 bytes per instance and this number will determine how many Vorbis channels can be played simultaneously. Default = 32. */
    int                 maxAT9Codecs;               /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_CREATECOMPRESSEDSAMPLE only.  AT9    codecs consume 20,664 bytes per instance and this number will determine how many AT9    channels can be played simultaneously. Default = 32. */
    int                 maxFADPCMCodecs;            /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_CREATECOMPRESSEDSAMPLE only.  FADPCM codecs consume  2,232 bytes per instance and this number will determine how many FADPCM channels can be played simultaneously. Default = 32. */
    int                 maxPCMCodecs;               /* [r/w] Optional. Specify 0 to ignore. For use with PS3 only.                          PCM    codecs consume  2,536 bytes per instance and this number will determine how many streams and PCM voices can be played simultaneously. Default = 32. */
    int                 ASIONumChannels;            /* [r/w] Optional. Specify 0 to ignore. Number of channels available on the ASIO device. */
    char              **ASIOChannelList;            /* [r/w] Optional. Specify 0 to ignore. Pointer to an array of strings (number of entries defined by ASIONumChannels) with ASIO channel names. */
    FMOD_SPEAKER       *ASIOSpeakerList;            /* [r/w] Optional. Specify 0 to ignore. Pointer to a list of speakers that the ASIO channels map to.  This can be called after System::init to remap ASIO output. */
    float               HRTFMinAngle;               /* [r/w] Optional.                      For use with FMOD_INIT_HRTF_LOWPASS.  The angle range (0-360) of a 3D sound in relation to the listener, at which the HRTF function begins to have an effect. 0 = in front of the listener. 180 = from 90 degrees to the left of the listener to 90 degrees to the right. 360 = behind the listener. Default = 180.0. */
    float               HRTFMaxAngle;               /* [r/w] Optional.                      For use with FMOD_INIT_HRTF_LOWPASS.  The angle range (0-360) of a 3D sound in relation to the listener, at which the HRTF function has maximum effect. 0 = front of the listener. 180 = from 90 degrees to the left of the listener to 90 degrees to the right. 360 = behind the listener. Default = 360.0. */
    float               HRTFFreq;                   /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_INIT_HRTF_LOWPASS.  The cutoff frequency of the HRTF's lowpass filter function when at maximum effect. (i.e. at HRTFMaxAngle).  Default = 4000.0. */
    float               vol0virtualvol;             /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_INIT_VOL0_BECOMES_VIRTUAL.  If this flag is used, and the volume is below this, then the sound will become virtual.  Use this value to raise the threshold to a different point where a sound goes virtual. */
    unsigned int        defaultDecodeBufferSize;    /* [r/w] Optional. Specify 0 to ignore. For streams. This determines the default size of the double buffer (in milliseconds) that a stream uses.  Default = 400ms */
    unsigned short      profilePort;                /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_INIT_PROFILE_ENABLE.  Specify the port to listen on for connections by the profiler application. */
    unsigned int        geometryMaxFadeTime;        /* [r/w] Optional. Specify 0 to ignore. The maximum time in miliseconds it takes for a channel to fade to the new level when its occlusion changes. */
    float               distanceFilterCenterFreq;   /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_INIT_DISTANCE_FILTERING.  The default center frequency in Hz for the distance filtering effect. Default = 1500.0. */
    int                 reverb3Dinstance;           /* [r/w] Optional. Specify 0 to ignore. Out of 0 to 3, 3d reverb spheres will create a phyical reverb unit on this instance slot.  See FMOD_REVERB_PROPERTIES. */
    int                 DSPBufferPoolSize;          /* [r/w] Optional. Specify 0 to ignore. Number of buffers in DSP buffer pool.  Each buffer will be DSPBlockSize * sizeof(float) * SpeakerModeChannelCount.  ie 7.1 @ 1024 DSP block size = 8 * 1024 * 4 = 32kb.  Default = 8. */
    unsigned int        stackSizeStream;            /* [r/w] Optional. Specify 0 to ignore. Specify the stack size for the FMOD Stream thread in bytes.  Useful for custom codecs that use excess stack.  Default 49,152 (48kb) */
    unsigned int        stackSizeNonBlocking;       /* [r/w] Optional. Specify 0 to ignore. Specify the stack size for the FMOD_NONBLOCKING loading thread.  Useful for custom codecs that use excess stack.  Default 65,536 (64kb) */
    unsigned int        stackSizeMixer;             /* [r/w] Optional. Specify 0 to ignore. Specify the stack size for the FMOD mixer thread.  Useful for custom dsps that use excess stack.  Default 49,152 (48kb) */
    FMOD_DSP_RESAMPLER  resamplerMethod;            /* [r/w] Optional. Specify 0 to ignore. Resampling method used with fmod's software mixer.  See FMOD_DSP_RESAMPLER for details on methods. */
    unsigned int        commandQueueSize;           /* [r/w] Optional. Specify 0 to ignore. Specify the command queue size for thread safe processing.  Default 2048 (2kb) */
    unsigned int        randomSeed;                 /* [r/w] Optional. Specify 0 to ignore. Seed value that FMOD will use to initialize its internal random number generators. */
};


//
// MARK: Global Variables and Types
//

constexpr int EXP_COMP_SKIP_CYCLES = 10;            ///< In how many cycles to skip expensive computations?
constexpr int FMOD_NUM_VIRT_CHANNELS = 1000;        ///< Number of virtual channels during initialization
constexpr float FMOD_3D_MAX_DIST     = 10000.0f;    ///< Value used for 3D max distance, which doesn't have much of a function for the inverse roll-off model used here
constexpr float FMOD_LOW_PASS_GAIN   = 0.2f;        ///< Gain used when activating Low Pass filter

static FMOD_SYSTEM* gpFmodSystem = nullptr;         ///< FMOD system
static unsigned int gFmodVer = 0;                   ///< FMOD version

/// Use pre-v2 FMOD version structures?
inline bool UseOldFmod() { return gFmodVer < 0x00020000; }

/// Definition of how sound is handled based on dataRef values (type)
struct SoundDefTy {
    float (Aircraft::* pVal)() const = nullptr;     ///< Function return the value to observe, typically a dataRef value
    bool bSndLoop = true;                           ///< Sound to be played in a loop while value is large than `valMin`? Otherwise a single sound upon detection of a change of value
    float valMin = NAN;                             ///< Sound to be played if `*pVal > valMin`
    float valMax = NAN;                             ///< Only used to control volume, which raises for `*pVal` between `valMin` and `valMax`
};

/// Definition of how sound is handled based on dataRef values
static SoundDefTy gaSoundDef[Aircraft::SND_NUM_EVENTS] = {
    { &Aircraft::GetThrustRatio,        true, 0.0f,  1.0f },    // SND_ENG
    { &Aircraft::GetThrustReversRatio,  true, 0.0f,  1.0f },    // SND_REVERSE_THRUST
    { &Aircraft::GetTireRotRpm,         true, 60.0f, 1000.0f }, // SND_TIRE
    { &Aircraft::GetGearRatio,          false, NAN, NAN },      // SND_GEAR
    { &Aircraft::GetFlapRatio,          false, NAN, NAN },      // SND_FLAPS
};


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

/// Just log an error without raising an exception
#define FMOD_LOG(fmodCall)  {                                                      \
    if ((XPMP2::gFmodRes = (fmodCall)) != FMOD_OK)                                 \
        XPMP2::FmodError(#fmodCall, XPMP2::gFmodRes, __LINE__, __func__).LogErr(); \
}

//
// MARK: FMOD Logging
//

/// @brief Callback function called by FMOD for logging purposes
/// @note FMOD warns this can be called from any thread,
///       so strictly speaking we must not call `XPLMDebugString()`,
///       but so far it didn't hurt, and it should be debug builds only anyway.
FMOD_RESULT F_CALLBACK SoundLogCB(FMOD_DEBUG_FLAGS flags,
                                  const char * file,
                                  int line,
                                  const char * func,
                                  const char *message)
{
    // Basically convert it to our standard log output
    LogMsg(file, line, func,
           flags & FMOD_DEBUG_LEVEL_ERROR   ? logERR  :
           flags & FMOD_DEBUG_LEVEL_WARNING ? logWARN : logINFO,
           "FMOD_LOG: %s", message);
    return FMOD_OK;
}

/// @brief Enables/disables FMOD logging
/// @note FMOD Logging only works if linked to the `L` versions of the FMOD library
void SoundLogEnable (bool bEnable = true)
{
    gFmodRes = FMOD_Debug_Initialize(bEnable ? FMOD_DEBUG_LEVEL_LOG : FMOD_DEBUG_LEVEL_NONE,
                                     FMOD_DEBUG_MODE_CALLBACK, SoundLogCB, nullptr);
    if (gFmodRes == FMOD_OK) {
        LOG_MSG(logDEBUG, "FMOD logging has been %s.", bEnable ? "enabled" : "disabled");
    }
    // FMOD_ERR_UNSUPPORTED just means: not linked to fmodL, ie. "normal"
    else if (gFmodRes != FMOD_ERR_UNSUPPORTED)
    {
        FmodError("FMOD_Debug_Initialize", gFmodRes, __LINE__, __func__);
    }
}

//
// MARK: Sound Files
//

/// @brief Represents a sound file
/// @see For 3D cones also see https://www.fmod.com/docs/2.01/api/core-api-channelcontrol.html#channelcontrol_set3dconesettings
class SoundFile {
public:
    std::string filePath;           ///< File path to sound file
    bool bLoop = true;              ///< sound to be played in a loop?
    
    // 3D Cone information, to be applied to the channel later when playing the sound
    float coneDir = NAN;            ///< Which direction relative to plane's heading does the cone point to? (180 would be typical for jet engines)
    float conePitch = NAN;          ///< Which pitch does the cone point to (up/down)? (0 would be typical, ie. level with the plane)
    float coneInAngle = NAN;        ///< Inside cone angle. This is the angle spread within which the sound is unattenuated.
    float coneOutAngle = NAN;       ///< Outside cone angle. This is the angle spread outside of which the sound is attenuated to its SoundFile::coneOutVol.
    float coneOutVol = NAN;         ///< Cone outside volume.

protected:
    unsigned len_ms = 0;            ///< Sound length in ms
    FMOD_SOUND* pSound = nullptr;   ///< FMOD sound object
    bool bLoaded = false;           ///< cached version of the sound's `openstate`

public:
    /// @brief Construct a sound object from a file name and have it loaded asynchonously
    /// @throws FmodError in case of errors during `CreateSound`
    SoundFile (const std::string& _filePath, bool _bLoop,
               float _coneDir, float _conePitch,
               float _coneInAngle, float _coneOutAngle, float _coneOutVol) :
    filePath(_filePath), bLoop(_bLoop),
    coneDir(_coneDir), conePitch(_conePitch),
    coneInAngle(_coneInAngle), coneOutAngle(_coneOutAngle), coneOutVol(_coneOutVol)
    {
        FMOD_TEST(FMOD_System_CreateSound(gpFmodSystem, filePath.c_str(),
                                          (bLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF) |
                                          FMOD_3D | FMOD_3D_WORLDRELATIVE | FMOD_3D_INVERSEROLLOFF |
                                          FMOD_CREATECOMPRESSEDSAMPLE | FMOD_NONBLOCKING,
                                          nullptr, &pSound));
        LOG_ASSERT(pSound);
    }
    
    /// Copying is not permitted
    SoundFile (const SoundFile& o) = delete;
    
    /// Destructor removes the sound object
    ~SoundFile()
    {
        if (pSound)
            FMOD_Sound_Release(pSound);
        pSound = nullptr;
        bLoaded = false;
    }
    
    /// Called the first time the sound is played after loading has finished
    void firstTimeInit ()
    {
        FMOD_LOG(FMOD_Sound_GetLength(pSound, &len_ms, FMOD_TIMEUNIT_MS));
    }
    
    /// Has loading the sound sample finished?
    bool isReady ()
    {
        // Cached values
        if (!pSound) return false;
        if (bLoaded) return true;
        // Otherwise query FMOD
        FMOD_OPENSTATE state = FMOD_OPENSTATE_LOADING;
        if ((FMOD_Sound_GetOpenState(pSound, &state, nullptr, nullptr, nullptr) == FMOD_OK) &&
            (state == FMOD_OPENSTATE_READY))
        {
            firstTimeInit();
            return bLoaded = true;
        }
        return false;
    }
    
    /// Has full cone information?
    bool hasConeInfo () const
    {
        return
            !std::isnan(coneDir) &&
            !std::isnan(conePitch) &&
            !std::isnan(coneInAngle) &&
            !std::isnan(coneOutAngle) &&
            !std::isnan(coneOutVol);
    }
    
    /// @brief Play the sound
    /// @param pChGrp [opt] The channel group this sound is to be added to, can be `nullptr`
    /// @param bPaused [opt] Shall channel stay paused after creation?
    /// @return `nullptr` in case of failure
    FMOD_CHANNEL* play (FMOD_CHANNELGROUP* pChGrp = nullptr,
                        bool bPaused = false)
    {
        FMOD_CHANNEL* pChn = nullptr;
        if (isReady()) {
            FMOD_LOG(FMOD_System_PlaySound(gpFmodSystem, pSound, pChGrp, bPaused, &pChn));
            if (!pChn) return nullptr;
            FMOD_LOG(FMOD_Channel_SetUserData(pChn, this));     // save pointer to this SoundFile as user data to the channel
            if (hasConeInfo()) {
                FMOD_LOG(FMOD_Channel_Set3DConeSettings(pChn, coneInAngle, coneOutAngle, coneOutVol));
            }
        } else {
            LOG_MSG(logDEBUG, "Sound '%s' isn't ready yet, won't play now", filePath.c_str());
        }
        return pChn;
    }
    
    /// @brief Set the channel's sound cone relative to the aircraft's orientation in space and the cone configuration of the SoundFile
    /// @param pChn The channel, which is to be modified
    /// @param ac Aircraft
    void setConeOrientation (FMOD_CHANNEL* pChn, const Aircraft& ac)
    {
        if (!hasConeInfo()) return;
        const float coneDirRad = deg2rad(coneDir);
        FMOD_VECTOR coneVec = FmodHeadPitch2Vec(ac.GetHeading() + coneDir,
                                                std::cos(coneDirRad)*ac.GetPitch() - std::sin(coneDirRad)*ac.GetRoll() + conePitch);
        FMOD_LOG(FMOD_Channel_Set3DConeOrientation(pChn, &coneVec));
    }

    /// @brief Load fixed set of X-Plane-internal sounds
    /// @returns Number of loaded sounds
    static int LoadXPSounds ()
    {
        int n = 0;
#define ADD_SND(s,f,l) if (!XPMPSoundAdd(s,f,l)[0]) ++n;
#define ADD_CONE(s,f,l,cd,cp,cia,coa,cov) if (!XPMPSoundAdd(s,f,l,cd,cp,cia,coa,cov)[0]) ++n;
        // Engine sounds
        ADD_SND(XP_SOUND_ELECTRIC,          "Resources/sounds/engine/ENGINE_ELECTRIC_out.wav",          true);
        // The two jet sounds define a sound cone: 180 degrees heading (ie. backwards), no pitch, with 30 degree inner angle (full sound), and 60 degree outer angle (reduced sound), with sound reduction to 50% outside the outer angle:
        ADD_CONE(XP_SOUND_HIBYPASSJET,      "Resources/sounds/engine/ENGINE_HI_BYPASS_JET_out.wav",     true, 180.0f, 0.0f, 30.0f, 60.0f, 0.5f);
        ADD_CONE(XP_SOUND_LOBYPASSJET,      "Resources/sounds/engine/ENGINE_LO_BYPASS_JET_out.wav",     true, 180.0f, 0.0f, 30.0f, 60.0f, 0.5f);
        ADD_SND(XP_SOUND_TURBOPROP,         "Resources/sounds/engine/ENGINE_TURBOPROP_out.wav",         true);
        ADD_SND(XP_SOUND_PROP_AIRPLANE,     "Resources/sounds/engine/PROPELLER_OF_AIRPLANE_out.wav",    true);
        ADD_SND(XP_SOUND_PROP_HELI,         "Resources/sounds/engine/PROPELLER_OF_HELO_out.wav",        true);
        ADD_SND(XP_SOUND_REVERSE_THRUST,    "Resources/sounds/engine/REVERSE_THRUST_out.wav",           true);
        
        // Rolling on the ground
        ADD_SND(XP_SOUND_ROLL_RUNWAY,       "Resources/sounds/contact/roll_runway.wav",                 true);
        
        // One-time sounds
        ADD_SND(XP_SOUND_FLAP,              "Resources/sounds/systems/flap.wav",                        false);
        ADD_SND(XP_SOUND_GEAR,              "Resources/sounds/systems/gear.wav",                        false);
        return n;
    }
};

/// Map of all sounds, indexed by a sound name (type)
typedef std::map<std::string,SoundFile> mapSoundTy;

/// Map of all sounds, indexed by a sound name
static mapSoundTy mapSound;

//
// MARK: Local Functions
//

/// Get the FMOD system's master channel group, or `nullptr`
FMOD_CHANNELGROUP* SoundGetMasterChn()
{
    FMOD_CHANNELGROUP* pMstChnGrp = nullptr;
    if (!gpFmodSystem) return nullptr;
    FMOD_LOG(FMOD_System_GetMasterChannelGroup(gpFmodSystem, &pMstChnGrp));
    return pMstChnGrp;
}

/// Get pointer to Sound File that created the channel
SoundFile* SoundGetSoundFile (FMOD_CHANNEL* pChn)
{
    if (!pChn) return nullptr;
    SoundFile* pSnd = nullptr;
    FMOD_Channel_GetUserData(pChn, (void**)&pSnd);
    return pSnd;
}

/// Is the channel still playing / is it valid?
bool SoundIsChnValid (FMOD_CHANNEL* pChn)
{
    if (!pChn) return false;
    try {
        FMOD_BOOL bPlaying = false;
        FMOD_TEST(FMOD_Channel_IsPlaying(pChn, &bPlaying));
        return bPlaying;
    }
    catch (...) {
        return false;
    }
}

/// @brief Helper functon to set FMOD settings
/// @details Implement as template function so it works with both
///          `FMOD_ADVANCEDSETTINGS` and `FMOD_10830_ADVANCEDSETTINGS`
///          for backwards compatibility with v1.8 of FMOD as used in XP11.
template<class T_ADVSET>
void SoundSetFmodSettings(T_ADVSET& advSet)
{
    memset(&advSet, 0, sizeof(advSet));
    advSet.cbSize = sizeof(advSet);
    advSet.vol0virtualvol = 0.01f;     // set some low volume for fading to 0
    FMOD_TEST(FMOD_System_SetAdvancedSettings(gpFmodSystem, (FMOD_ADVANCEDSETTINGS*)&advSet));
}

/// Return a text for the values of Aircraft::SoundEventsTy
const char* SoundEventTxt (Aircraft::SoundEventsTy e)
{
    switch (e) {
        case Aircraft::SND_ENG:             return "Engine";
        case Aircraft::SND_REVERSE_THRUST:  return "Reverse Thrust";
        case Aircraft::SND_TIRE:            return "Tires";
        case Aircraft::SND_GEAR:            return "Gear";
        case Aircraft::SND_FLAPS:           return "Flaps";
        default:
            return "<unknown>";
    }
}

/// @brief Convert heading/pitch to normalized x/y/z vector
/// @note Given all the trigonometric functions this is probably expensive,
///       so use with care and avoid in flight loop callback when unnecessary.
FMOD_VECTOR FmodHeadPitch2Vec (const float head, const float pitch)
{
    const std::valarray<float> v = HeadPitch2Vec(head, pitch);
    assert(v.size() == 3);
    return FMOD_VECTOR { v[0], v[1], v[2] };
}

/// @brief Convert heading/pitch to normal x/y/z vector
/// @note Given all the trigonometric functions this is probably expensive,
///       so use with care and avoid in flight loop callback when unnecessary.
void FmodHeadPitchRoll2Normal(const float head, const float pitch, const float roll,
                              FMOD_VECTOR& vecDir, FMOD_VECTOR& vecNorm)
{
    const std::valarray<float> v = HeadPitchRoll2Normal(head, pitch, roll);
    assert(v.size() == 6);
    vecDir.x  = v[0];
    vecDir.y  = v[1];
    vecDir.z  = v[2];
    vecNorm.x = v[3];
    vecNorm.y = v[4];
    vecNorm.z = v[5];
}

/// @brief Normalize Pitch/Heading
/// @details Heading: [0; 360), Pitch: [-90; 90]
void FmodNormalizeHeadingPitch(float& head, float& pitch)
{
    // pitch "over the top"
    if (pitch > 90.0f) {
        pitch = 180.0f - pitch;
        head += 180.0f;
    }
    if (pitch < -90.0f) {
        pitch = -180.0f - pitch;
        head += 180.0f;
    }

    // normalize heading
    while (head >= 360.0f) head -= 360.0f;
    while (head < 0.0f) head += 360.0f;
}

//
// MARK: Public Aircraft member functions
//

// Play a sound; a looping sound plays until explicitely stopped
FMOD_CHANNEL* Aircraft::SoundPlay (const std::string& sndName, float vol)
{
    // Have FMOD create the sound
    try {
        SoundFile& snd = mapSound.at(sndName);
        // start paused to avoid cracking, will be unpaused only in the next call to Aircraft::SoundUpdate()
        FMOD_CHANNEL* pChn = snd.play(nullptr, true);
        if (pChn) {
            chnList.push_back(pChn);                    // add to managed list of sounds
            FMOD_LOG(FMOD_Channel_Set3DMinMaxDistance(pChn, (float)sndMinDist, FMOD_3D_MAX_DIST));
            snd.setConeOrientation(pChn, *this);        // set cone orientation if there is one defined
            SoundVolume(pChn, vol);                     // Set volume
            if (bChnLowPass) {                          // if currently active, also activate low pass filter on this new channel
                FMOD_LOG(FMOD_Channel_SetLowPassGain(pChn, FMOD_LOW_PASS_GAIN));
            }
            if (bChnMuted) {                            // if currently muted, then mute
                FMOD_LOG(FMOD_Channel_SetMute(pChn, true));
            }
        }
        return pChn;
    }
    // Raised by map.at if not finding the key
    catch (const std::out_of_range& e) {
        LOG_MSG(logERR, "Didn't find a sound named '%s': %s", sndName.c_str(), e.what());
        return nullptr;
    }
}

// Stop a continuously playing sound
void Aircraft::SoundStop (FMOD_CHANNEL* pChn)
{
    FMOD_Channel_Stop(pChn);
    chnList.remove(pChn);
}

// Sets the sound's volume
void Aircraft::SoundVolume (FMOD_CHANNEL* pChn, float vol)
{
    // Just set the volume
    FMOD_LOG(FMOD_Channel_SetVolume(pChn, vol));
}

// Mute/Unmute all sounds of the airplane temporarily
void Aircraft::SoundMuteAll (bool bMute)
{
    for (FMOD_CHANNEL* pChn: chnList)
        FMOD_Channel_SetMute(pChn, bMute);
    bChnMuted = bMute;
    if (!chnList.empty()) {
        LOG_MATCHING(logDEBUG, "Aircraft %08X (%s): Sound %s",
                     modeS_id, GetFlightId().c_str(),
                     bMute ? "muted" : "unmuted");
    }
}

// Returns the name of the sound to play per event
std::string Aircraft::SoundGetName (SoundEventsTy sndEvent, float& volAdj) const
{
    // The default is: The more engines, the louder;
    // that not ownly goes for engine sounds, but also the rest will be bigger the bigger the plane
    volAdj = float(pCSLMdl->GetNumEngines());
    // Now go by event type:
    switch (sndEvent) {
        // Engine Sound is based on aircraft classification
        case SND_ENG:
            // Sanity check: need a CSL model to derive details
            if (!pCSLMdl) {
                LOG_MSG(logWARN, "Aircraft %08X (%s): No CSL model info, using default engine sound",
                        modeS_id, GetFlightId().c_str());
                return XP_SOUND_PROP_AIRPLANE;
            }
            // Now check out engine type and return proper sound name
            if (IsGroundVehicle()) {                                // We assume all ground vehicles are electric cars by now...they should be at least ;-)
                return XP_SOUND_ELECTRIC;
            }
            if (pCSLMdl->HasRotor()) {
                volAdj += 1.0f;                                     // Helis are loud anyway, but louder with more engines
                return XP_SOUND_PROP_HELI;
            }
            switch (pCSLMdl->GetClassEngType()) {
                case 'E': return XP_SOUND_ELECTRIC;
                case 'J': return pCSLMdl->GetClassNumEng() == '1' ? XP_SOUND_HIBYPASSJET : XP_SOUND_LOBYPASSJET;
                case 'P': return XP_SOUND_PROP_AIRPLANE;
                case 'T': return XP_SOUND_TURBOPROP;
                default:
                    LOG_MSG(logWARN, "Aircraft %08X (%s): Unknown engine type '%c', using default engine sound",
                            modeS_id, GetFlightId().c_str(), pCSLMdl->GetClassEngType());
                    return XP_SOUND_PROP_AIRPLANE;
            }
            
        // All other sound types have constant sounds assigned
        case SND_REVERSE_THRUST:                        return XP_SOUND_REVERSE_THRUST;
        case SND_TIRE:              volAdj *= 0.50f;    return XP_SOUND_ROLL_RUNWAY;
        case SND_GEAR:              volAdj *= 0.25f;    return XP_SOUND_GEAR;   // that sound is too loud compared to engines, make it more quiet
        case SND_FLAPS:             volAdj *= 0.25f;    return XP_SOUND_FLAP;   // that sound is too loud compared to engines, make it more quiet
            
        default:
            LOG_MSG(logERR, "Aircraft %08X (%s): Unknown Sound Event type %d, no sound name returned",
                    modeS_id, GetFlightId().c_str(), int(sndEvent));
            return "";
    }
}

//
// MARK: Protected Aircraft member functions
//

// Sound-related initializations, called by Create() and ChangeModel()
void Aircraft::SoundSetup ()
{
    // Just to be sure: remove everything
    // In case of a model-rematch this may in fact change the aircraft's sound!
    SoundRemoveAll();
    
    // Find a default "sound size" depending on engine type and number of engines
    if      (pCSLMdl->HasRotor())               sndMinDist = 25;    // Helis are loud, even with a single engine!
    else if (pCSLMdl->GetClassEngType() == 'J') sndMinDist = 20;    // each jet engine, too
    else if (pCSLMdl->GetClassEngType() == 'T') sndMinDist = 15;    // Turboprops are nearly as loud
    else sndMinDist = 10;                                           // everything else falls behind
    sndMinDist *= pCSLMdl->GetNumEngines();
    
    // For gliders there's no engine sound, otherwise there is
    aSndCh[SND_ENG].bAuto = !IsGlider();
}

// Update sound, like position and volume, called once per frame
void Aircraft::SoundUpdate ()
{
    // If we don't want sound we don't get sound
    if (!gpFmodSystem) return;

    try {
        // Decide here already if we need to activate or inactivate the Low Pass filter
        enum { LP_NoAction = 0, LP_Enable, LP_Disable } eLP = LP_NoAction;
        if (IsViewExternal()) {
            if (bChnLowPass) { eLP = LP_Disable; bChnLowPass = false; }
        } else {
            if (!bChnLowPass) { eLP = LP_Enable; bChnLowPass = true; }
        }

        // --- Loop all Sound Definitions ---
        for (SoundEventsTy eSndEvent = SoundEventsTy(0);
             eSndEvent < SND_NUM_EVENTS;
             eSndEvent = SoundEventsTy(eSndEvent + 1))
        {
            // Simpler access to values indexed by the sound event:
            const SoundDefTy    &def        = gaSoundDef[eSndEvent];
            SndChTy             &sndCh      = aSndCh[eSndEvent];

            // Channel is no longer playing?
            if (!SoundIsChnValid(sndCh.pChn))
                sndCh.pChn = nullptr;

            // Automatic Handling of this event?
            if (sndCh.bAuto)
            {
                assert(def.pVal);
                const float fVal = (this->*def.pVal)();     // get the current (dataRef) value
                // --- Looping sound? ---
                if (def.bSndLoop)
                {
                    assert(def.valMax > def.valMin);
                    
                    // Looping sound: Should there be sound?
                    if (fVal > def.valMin) {
                        // Set volume based on val (between min and max)
                        float vol = std::clamp<float>((fVal - def.valMin) / (def.valMax - def.valMin), 0.0f, 1.0f);
                        // If there hasn't been a sound triggered do so now
                        if (!sndCh.pChn) {
                            // Get Sound's name and volume adjustment
                            const std::string sndName = SoundGetName(eSndEvent, sndCh.volAdj);
                            if (!sndName.empty()) {
                                vol *= sndCh.volAdj;
                                sndCh.pChn = SoundPlay(sndName, vol);
                                if (sndCh.pChn) {
                                    LOG_MATCHING(logINFO, "Aircraft %08X (%s): Looping sound '%s' at volume %.2f for '%s'",
                                                 modeS_id, GetFlightId().c_str(),
                                                 sndName.c_str(), vol, SoundEventTxt(eSndEvent));
                                }
                            }
                        } else {
                            // Update the volume as it can change any time
                            SoundVolume(sndCh.pChn, vol * sndCh.volAdj);
                        }
                    } else {
                        // There should be no sound, remove it if there was one
                        if (sndCh.pChn) {
                            SoundStop(sndCh.pChn);
                            LOG_MATCHING(logINFO, "Aircraft %08X (%s): Stopped sound for '%s'",
                                         modeS_id, GetFlightId().c_str(),
                                         SoundEventTxt(eSndEvent));
                            sndCh.pChn = nullptr;
                        }
                    }
                }
                // --- One-time event ---
                else
                {
                    // Fresh object, don't even know a 'last value'?
                    if (std::isnan(sndCh.lastDRVal)) {
                        sndCh.lastDRVal = fVal;                         // Remember the initial value 
                    }
                    // Should there be sound because the value changed?
                    else if (std::fabs(sndCh.lastDRVal - fVal) > 0.01f)
                    {
                        sndCh.lastDRVal = fVal;                         // Remember this current value 
                        // If there hasn't been a sound triggered do so now
                        if (!sndCh.pChn)
                        {
                            const std::string sndName = SoundGetName(eSndEvent, sndCh.volAdj);
                            if (!sndName.empty()) {
                                sndCh.pChn = SoundPlay(sndName, sndCh.volAdj);
                                LOG_MATCHING(logINFO, "Aircraft %08X (%s): Playing sound '%s' once for '%s'",
                                             modeS_id, GetFlightId().c_str(),
                                             sndName.c_str(), SoundEventTxt(eSndEvent));
                            }
                        }
                    }
                    // No more significant value change: Clear pointer to sound once sound has ended
                    else if (sndCh.pChn && !SoundIsChnValid(sndCh.pChn)) {
                        sndCh.pChn = nullptr;
                    }
                }
                
                // --- Low pass filter in case we're inside a cockpit ---
                if (sndCh.pChn && eLP) {
                    FMOD_LOG(FMOD_Channel_SetLowPassGain(sndCh.pChn,
                                                         eLP == LP_Enable ? FMOD_LOW_PASS_GAIN : 1.0f));
                }
            }
            // No automatic handling of this event
            else
            {
                // So if there currently is a channel, remove it
                if (sndCh.pChn) {
                    SoundStop(sndCh.pChn);
                    sndCh.pChn = nullptr;
                }
            }
        }
        
        // --- Update all channels' 3D position
        {
            // Decide here already if this time we do expensive computations (like sound cone orientation)
            bool bDoExpensiveComp = false;
            if (++skipCounter > EXP_COMP_SKIP_CYCLES) {
                skipCounter = 0;
                bDoExpensiveComp = true;
            }
            
            // Aircraft position and velocity
            FMOD_VECTOR fmodPos {                   // Copy of the aircraft's 3D location
                drawInfo.x,
                drawInfo.y,
                drawInfo.z,
            };
            FMOD_VECTOR fmodVel { v_x, v_y, v_z };  // Copy of the aircraft's relative speed
            
            for (auto iter = chnList.begin(); iter != chnList.end(); )
            {
                // Set location
                gFmodRes = FMOD_Channel_Set3DAttributes(*iter, &fmodPos, &fmodVel);
                if (gFmodRes == FMOD_OK) {
                    // --- Expensive Compuations: Cone Orientation ---
                    if (bDoExpensiveComp)
                    {
                        SoundFile* pSnd = SoundGetSoundFile(*iter);
                        if (pSnd && pSnd->hasConeInfo())
                            pSnd->setConeOrientation(*iter, *this);
                    }
                    // Unpause the channel if still paused (always starts paused to avoid cracking)
                    FMOD_BOOL bPaused = false;
                    FMOD_LOG(FMOD_Channel_GetPaused(*iter, &bPaused));
                    if (bPaused) {
                        FMOD_LOG(FMOD_Channel_SetPaused(*iter, false));
                    }
                    // Next sound channel
                    iter++;
                }
                else
                    // Channels might have become invalid along the way, e.g. if a non-looping sound has ended
                    iter = chnList.erase(iter);
            }
        }
    }
    FMOD_CATCH;
}

// Remove all sound, e.g. during destruction
void Aircraft::SoundRemoveAll ()
{
    // Log a message only if there actually is anything to stop
    if (!chnList.empty()) {
        LOG_MATCHING(logDEBUG, "Aircraft %08X (%s): Removed all sounds",
                     modeS_id, GetFlightId().c_str());
    }
    // All channels now to be stopped
    for (FMOD_CHANNEL* pChn: chnList)
        FMOD_Channel_Stop(pChn);
    for (SndChTy &sndChn: aSndCh)
        sndChn.pChn = nullptr;
}


//
// MARK: Global Functions
//

// Initialize the sound module and load the sounds
bool SoundInit ()
{
    try {
        if (!gpFmodSystem) {
            // Enable FMOD logging
            if (glob.logLvl == logDEBUG)
                SoundLogEnable(true);

            // Create FMOD system and first of all determine its version,
            // which depends a bit if run under XP11 or XP12 and the OS we are on.
            // There are subtle difference in settings.
            FMOD_TEST(FMOD_System_Create(&gpFmodSystem, FMOD_VERSION));
            FMOD_TEST(FMOD_System_GetVersion(gpFmodSystem, &gFmodVer));
            LOG_MSG(logINFO, "Initializing FMOD version %u.%u.%u",
                    gFmodVer >> 16, (gFmodVer & 0x0000ff00) >> 8,
                    gFmodVer & 0x000000ff);
            FMOD_TEST(FMOD_System_Init(gpFmodSystem, FMOD_NUM_VIRT_CHANNELS,
                                       FMOD_INIT_3D_RIGHTHANDED | FMOD_INIT_CHANNEL_LOWPASS | FMOD_INIT_VOL0_BECOMES_VIRTUAL, nullptr));
            
            // Set advanced settings, which are version-dependend
            if (UseOldFmod()) {
                FMOD_10830_ADVANCEDSETTINGS oldAdvSet;
                SoundSetFmodSettings(oldAdvSet);
            } else {
                FMOD_ADVANCEDSETTINGS advSet;
                SoundSetFmodSettings(advSet);
            }
            
            // Load the default sounds
            if (!SoundFile::LoadXPSounds()) {
                LOG_MSG(logERR, "No standard sounds successfully loaded, sound system currently inactive!");
                return false;
            }

            // Set master volume
            XPMPSoundSetMasterVolume(glob.sndMasterVol);
            
            // Ensure updates are done once and a proper listener location is set
            SoundUpdatesDone();
        }
        glob.bSoundAvail = true;
        return true;
    }
    catch (const FmodError& e) {
        e.LogErr();                             // log the error
        SoundCleanup();                         // cleanup
        return false;
    }
}

// Prepare for this frame's updates, which are about to start
void SoundUpdatesBegin()
{
    // Currently, there's nothing in here
}

// Tell FMOD that all updates are done
void SoundUpdatesDone ()
{
    if (gpFmodSystem) try {
        // Update the listener position and orientation
        const FMOD_VECTOR posCam   = {
            glob.posCamera.x,
            glob.posCamera.y,
            glob.posCamera.z
        };
        const FMOD_VECTOR velocity  = { glob.vCam_x, glob.vCam_y, glob.vCam_z };

        // The forward direction takes heading, pitch, and roll into account
        FmodNormalizeHeadingPitch(glob.posCamera.heading, glob.posCamera.pitch);
        FMOD_VECTOR normForw;
        FMOD_VECTOR normUpw;
        FmodHeadPitchRoll2Normal(glob.posCamera.heading, glob.posCamera.pitch, glob.posCamera.roll, normForw, normUpw);
        
        // FMOD_ERR_INVALID_VECTOR used to be a problem, but should be no longer
        // Still, just in case it comes back, we log more details
        gFmodRes = FMOD_System_Set3DListenerAttributes(gpFmodSystem, 0, &posCam, &velocity, &normForw, &normUpw);
        if (gFmodRes != FMOD_OK) {
            if (gFmodRes != FMOD_ERR_INVALID_VECTOR)
                throw FmodError("FMOD_System_Set3DListenerAttributes", gFmodRes, __LINE__, __func__);
            else
            {
                static float lastInvVecErrMsgTS = -500.0f;
                FmodError("FMOD_System_Set3DListenerAttributes", gFmodRes, __LINE__, __func__).LogErr();
                if (GetMiscNetwTime() >= lastInvVecErrMsgTS + 300.0f) {
                    lastInvVecErrMsgTS = GetMiscNetwTime();
                    LOG_MSG(logERR, "Please report the following details as a reply to https://bit.ly/LTSound36");
                    LOG_MSG(logERR, "Camera   roll=%.3f, heading=%.3f, pitch=%.3f",
                            glob.posCamera.roll, glob.posCamera.pitch, glob.posCamera.heading);
                    LOG_MSG(logERR, "normForw x=%.6f, y=%.6f, z=%.6f", normForw.x, normForw.y, normForw.z);
                    LOG_MSG(logERR, "normUpw  x=%.6f, y=%.6f, z=%.6f", normUpw.x, normUpw.y, normUpw.z);
                }
            }
        }
        
        // Mute-on-Pause
        if (glob.bSoundMuteOnPause) {                           // shall act automatically on Pause?
            if (IsPaused()) {                                   // XP is paused?
                if (!glob.bSoundAutoMuted) {                    // ...but not yet muted?
                    XPMPSoundMute(true);                        //    -> do mute
                    glob.bSoundAutoMuted = true;
                }
            } else {                                            // XP is not paused
                if (glob.bSoundAutoMuted) {                     // ...but sound is auto-muted?
                    XPMPSoundMute(false);                       //    -> unmute
                    glob.bSoundAutoMuted = false;
                }
            }
        }

        // Tell FMOD we're done
        FMOD_TEST(FMOD_System_Update(gpFmodSystem));
    }
    catch (const FmodError& e) {
        e.LogErr();                             // log the error
        // We ignore one specific error, which has been reported on some Linux configurations
        if (e.fmodRes != FMOD_ERR_INVALID_VECTOR)
            SoundCleanup();                         // in case of errors here disable the sound system
    }
}

// Graceful shoutdown
void SoundCleanup ()
{
    mapSound.clear();               // cleanup the sounds

    if (gpFmodSystem)
        FMOD_System_Release(gpFmodSystem);
    gpFmodSystem = nullptr;
    gFmodVer = 0;
    glob.bSoundAvail = false;
    LOG_MSG(logDEBUG, "Sound system shut down");
    // Disable FMOD logging
    SoundLogEnable(false);
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

// Set Master Volume
void XPMPSoundSetMasterVolume (float fVol)
{
    XPMP2::glob.sndMasterVol = fVol;
    FMOD_CHANNELGROUP* pMstChnGrp = XPMP2::SoundGetMasterChn();
    if (!pMstChnGrp) return;
    FMOD_LOG(FMOD_ChannelGroup_SetVolume(pMstChnGrp, XPMP2::glob.sndMasterVol));
}

// Mute all sounds (temporarily)
void XPMPSoundMute(bool bMute)
{
    FMOD_CHANNELGROUP* pMstChnGrp = XPMP2::SoundGetMasterChn();
    if (!pMstChnGrp) return;
    FMOD_LOG(FMOD_ChannelGroup_SetMute(pMstChnGrp, bMute));
    LOG_MSG(XPMP2::logDEBUG, "All sounds %s", bMute ? "muted" : "unmuted");
}

// Add a sound that can later be referenced from an XPMP2::Aircraft
const char* XPMPSoundAdd (const char* sName,
                          const char* filePath,
                          bool bLoop,
                          float coneDir, float conePitch,
                          float coneInAngle, float coneOutAngle,
                          float coneOutVol)
{
    // Test file existence first before bothering
    if (!XPMP2::ExistsFile(filePath)) {
        LOG_MSG(XPMP2::logERR, "Sound file not found: %s", filePath)
        return "Sound File not found";
    }
    // Load the sound and add it to the map
    try {
        XPMP2::mapSound.emplace(std::piecewise_construct,
                                std::forward_as_tuple(sName),
                                std::forward_as_tuple(filePath,bLoop,
                                                      coneDir, conePitch,
                                                      coneInAngle, coneOutAngle,
                                                      coneOutVol));
        LOG_MSG(XPMP2::logDEBUG, "Added%ssound '%s' from file '%s'",
                bLoop ? " looping " : " ", sName, filePath);
    }
    catch (const XPMP2::FmodError& e) {
        e.LogErr();                             // log the error
        return FMOD_ErrorString(e.fmodRes);     // return the text
    }
    
    // All good
    return "";
}

// Enumerate all sounds, including the internal ones
const char* XPMPSoundEnumerate (const char* prevName, const char** ppFilePath)
{
    // No sounds available at all?
    if (XPMP2::mapSound.empty())
        return nullptr;
    
    auto sndIter = XPMP2::mapSound.end();
    // Return first sound?
    if (!prevName || !prevName[0])
        sndIter = XPMP2::mapSound.begin();
    else {
        // Try finding the given `prevName` sound, then return next
        sndIter = XPMP2::mapSound.find(prevName);
        if (sndIter != XPMP2::mapSound.end())
            sndIter++;
    }
    
    // Not found anything anymore?
    if (sndIter == XPMP2::mapSound.end()) {
        if (ppFilePath) *ppFilePath = nullptr;
        return nullptr;
    } else {
        // Return the found element
        if (ppFilePath) *ppFilePath = sndIter->second.filePath.c_str();
        return sndIter->first.c_str();
    }
}

#endif // INCLUDE_FMOD_SOUND
