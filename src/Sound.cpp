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

FMOD_VECTOR HeadPitch2Vec (const float head, const float pitch);

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
// MARK: Sound Files
//

/// @brief Represents a sound file
/// @see For 3D cones also see https://www.fmod.com/docs/2.01/api/core-api-channelcontrol.html#channelcontrol_set3dconesettings
class SoundFile {
public:
    std::string filePath;           ///< File path to sound file
    bool bLoop = true;              ///< sound to be played in a loop?
    float fVolAdj = 1.0f;           ///< Volume Adjustment factor for this particular sound, `>1` amplifies
    
    // 3D Cone information, to be applied to the channel later when playing the sound
    float coneDir = NAN;            ///< Which direction relative to plane's heading does the cone point to? (180 would be typical for jet engines)
    float conePitch = NAN;          ///< Which pitch does the cone point to (up/down)? (0 would be typical, ie. level with the plane)
    float coneInAngle = NAN;        ///< Inside cone angle. This is the angle spread within which the sound is unattenuated.
    float coneOutAngle = NAN;       ///< Outside cone angle. This is the angle spread outside of which the sound is attenuated to its SoundFile::coneOutVol.
    float coneOutVol = NAN;         ///< Cone outside volume.

protected:
    FMOD_SOUND* pSound = nullptr;   ///< FMOD sound object
    mutable bool bLoaded = false;   ///< cached version of the sound's `openstate`

public:
    /// @brief Construct a sound object from a file name and have it loaded asynchonously
    /// @throws FmodError in case of errors during `CreateSound`
    SoundFile (const std::string& _filePath, bool _bLoop, float _fVolAdj,
               float _coneDir, float _conePitch,
               float _coneInAngle, float _coneOutAngle, float _coneOutVol) :
    filePath(_filePath), bLoop(_bLoop), fVolAdj(_fVolAdj),
    coneDir(_coneDir), conePitch(_conePitch),
    coneInAngle(_coneInAngle), coneOutAngle(_coneOutAngle), coneOutVol(_coneOutVol)
    {
        FMOD_TEST(FMOD_System_CreateSound(gpFmodSystem, filePath.c_str(),
                                          (bLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF) |
                                          FMOD_3D | FMOD_CREATECOMPRESSEDSAMPLE | FMOD_NONBLOCKING,
                                          nullptr, &pSound));
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
    
    /// Has loading the sound sample finished?
    bool isReady () const
    {
        // Cached values
        if (!pSound) return false;
        if (bLoaded) return true;
        // Otherwise query FMOD
        FMOD_OPENSTATE state = FMOD_OPENSTATE_LOADING;
        if ((FMOD_Sound_GetOpenState(pSound, &state, nullptr, nullptr, nullptr) == FMOD_OK) &&
            (state == FMOD_OPENSTATE_READY))
        {
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
    /// @param pChGrp The (aircraft's) channel group this sound is to be added to, can be `nullptr`
    /// @param bPaused [opt] Shall channel stay paused after creation?
    FMOD_CHANNEL* play (FMOD_CHANNELGROUP* pChGrp,
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
            LOG_MSG(logWARN, "Sound '%s' isn't ready yet, won't play now", filePath.c_str());
        }
        return pChn;
    }
    
    /// @brief Set the channel's sound cone
    /// @param pChn The channel, which is to be modified
    /// @param acHead Aircraft's heading, used to calculate sound cone orientation
    /// @param acPitch Aicraft's pitch, used to calculate sound cone orientation
    void setConeOrientation (FMOD_CHANNEL* pChn, float acHead, float acPitch)
    {
        if (!hasConeInfo()) return;
        FMOD_VECTOR coneVec = HeadPitch2Vec(acHead + coneDir, acPitch + conePitch);
        FMOD_LOG(FMOD_Channel_Set3DConeOrientation(pChn, &coneVec));
    }

    /// @brief Load fixed set of X-Plane-internal sounds
    /// @returns Number of loaded sounds
    static int LoadXPSounds ()
    {
        int n = 0;
#define ADD_SND(s,f,l,va) if (!XPMPSoundAdd(s,f,l,va)[0]) ++n;
#define ADD_CONE(s,f,l,va,cd,cp,cia,coa,cov) if (!XPMPSoundAdd(s,f,l,va,cd,cp,cia,coa,cov)[0]) ++n;
        // Engine sounds
        ADD_SND(XP_SOUND_ELECTRIC,          "Resources/sounds/engine/ENGINE_ELECTRIC_out.wav",          true,  20.0f);
        ADD_CONE(XP_SOUND_HIBYPASSJET,      "Resources/sounds/engine/ENGINE_HI_BYPASS_JET_out.wav",     true,  20.0f, 180.0f, 0.0f, 30.0f, 60.0f, 0.0f);
        ADD_CONE(XP_SOUND_LOBYPASSJET,      "Resources/sounds/engine/ENGINE_LO_BYPASS_JET_out.wav",     true,  20.0f, 180.0f, 0.0f, 30.0f, 60.0f, 0.0f);
        ADD_SND(XP_SOUND_TURBOPROP,         "Resources/sounds/engine/ENGINE_TURBOPROP_out.wav",         true,  20.0f);
        ADD_SND(XP_SOUND_PROP_AIRPLANE,     "Resources/sounds/engine/PROPELLER_OF_AIRPLANE_out.wav",    true,  20.0f);
        ADD_SND(XP_SOUND_PROP_HELI,         "Resources/sounds/engine/PROPELLER_OF_HELO_out.wav",        true,  20.0f);
        ADD_SND(XP_SOUND_REVERSE_THRUST,    "Resources/sounds/engine/REVERSE_THRUST_out.wav",           true,  20.0f);
        
        // Rolling on the ground
        ADD_SND(XP_SOUND_ROLL_RUNWAY,       "Resources/sounds/contact/roll_runway.wav",                 true,   1.0f);
        
        // One-time sounds
        ADD_SND(XP_SOUND_FLAP,              "Resources/sounds/systems/flap.wav",                        false,  1.0f);
        ADD_SND(XP_SOUND_GEAR,              "Resources/sounds/systems/gear.wav",                        false,  1.0f);
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

/// From the original Sund File, get the volume adjustment
float SoundGetVolAdj (FMOD_CHANNEL* pChn)
{
    const SoundFile* pSndFile = SoundGetSoundFile(pChn);
    return pSndFile ? pSndFile->fVolAdj : 1.0f;
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
FMOD_VECTOR HeadPitch2Vec (const float head, const float pitch)
{
    // Subtracting 90 degress is because x coordinate is heading east,
    // so 90 degrees is equivalent to x = 0
    const float radHead = deg2rad(head - 90.0f);
    const float radPitch = deg2rad(pitch);
    // Have sinus/cosinus pre-computed and let the optimizer deal with reducing local variables
    const float sinHead = std::sin(radHead);
    const float cosHead = std::cos(radHead);
    const float sinPitch = std::sin(radPitch);
    const float cosPitch = std::cos(radPitch);
    return FMOD_VECTOR {
        cosHead * cosPitch,         // x
        sinPitch,                   // y
        sinHead * cosPitch          // z
    };
}

//
// MARK: Public Aircraft member functions
//

// Play a sound; a looping sound plays until explicitely stopped
FMOD_CHANNEL* Aircraft::SoundPlay (const std::string& sndName, float vol)
{
    // Make sure there's a sound group to connect to
    if (!SoundUpdateGrp()) return nullptr;
    
    // Have FMOD create the sound
    try {
        SoundFile& snd = mapSound.at(sndName);
        FMOD_CHANNEL* pChn = snd.play(pChnGrp, true);           // Create paused(!) channel
        if(!pChn) return nullptr;
        
        snd.setConeOrientation(pChn, GetHeading(), GetPitch()); // set cone orientation if there is one defined
        SoundVolume(pChn, vol, snd.fVolAdj);                    // Set volume
        if (bChnLowPass) {                                      // if currently active, also activate low pass filter on this new channel
            FMOD_LOG(FMOD_Channel_SetLowPassGain(pChn, FMOD_LOW_PASS_GAIN));
        }
        FMOD_LOG(FMOD_Channel_SetPaused(pChn, false));          // un-pause
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
}

// Sets the sound's volume after applying master volume
void Aircraft::SoundVolume (FMOD_CHANNEL* pChn, float vol, float fVolAdj)
{
    // Set the volume, in accordance with master volume and sound volume adjustment
    if (std::isnan(fVolAdj)) fVolAdj = SoundGetVolAdj(pChn);
    FMOD_LOG(FMOD_Channel_SetVolume(pChn, vol * fVolAdj * glob.sndMasterVol));
}

// Returns the name of the sound to play per event
std::string Aircraft::SoundGetName (SoundEventsTy sndEvent) const
{
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
            if (pCSLMdl->HasRotor()) return XP_SOUND_PROP_HELI;
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
        case SND_REVERSE_THRUST: return XP_SOUND_REVERSE_THRUST;
        case SND_TIRE: return XP_SOUND_ROLL_RUNWAY;
        case SND_GEAR: return XP_SOUND_GEAR;
        case SND_FLAPS: return XP_SOUND_FLAP;
            
        default:
            LOG_MSG(logERR, "Aircraft %08X (%s): Unknown Sound Event type %d, no sound name returned",
                    modeS_id, GetFlightId().c_str(), int(sndEvent));
            return "<UnknownSndType>";
    }
}

//
// MARK: Protected Aircraft member functions
//

// Update sound, like position and volume, called once per frame
void Aircraft::SoundUpdate ()
{
    // Stopped playing or technical issues?
    if (pChnGrp) {
        FMOD_BOOL bPlaying = false;
        if (FMOD_ChannelGroup_IsPlaying(pChnGrp, &bPlaying) != FMOD_OK || !bPlaying)
        {
            // Not playing or no longer valid: remove all
            SoundRemoveAll();
            return;
        }
    }

    try {
        // Update the group and its 3D location, also tests for sound enabled
        if (!SoundUpdateGrp()) return;
        
        // Decide here already if we need to activate or inactivate the Low Pass filter
        enum { LP_NoAction = 0, LP_Enable, LP_Disable } eLP = LP_NoAction;
        if (IsViewExternal()) {
            if (bChnLowPass) { eLP = LP_Disable; bChnLowPass = false; }
        } else {
            if (!bChnLowPass) { eLP = LP_Enable; bChnLowPass = true; }
        }

        // Decide here already if this time we do expensive computations (like sound cone orientation)
        bool bDoExpensiveComp = false;
        if (++skipCounter > EXP_COMP_SKIP_CYCLES) {
            skipCounter = 0;
            bDoExpensiveComp = true;
        }
        
        // --- Loop all Sound Definitions ---
        for (SoundEventsTy eSndEvent = SoundEventsTy(0);
             eSndEvent < SND_NUM_EVENTS;
             eSndEvent = SoundEventsTy(eSndEvent + 1))
        {
            // Simpler access to values indexed by the sound event:
            const SoundDefTy    &def        = gaSoundDef[eSndEvent];
            FMOD_CHANNEL*       &chn        = apChn[eSndEvent];
            float               &lastVal    = afChnLastVal[eSndEvent];

            // Get the pointer to the sound file that created the channel
            SoundFile* pSnd = nullptr;
            if (chn)
                FMOD_Channel_GetUserData(chn, (void**)&pSnd);
            // ...and from there the volume adjustment
            float volAdj = 1.0f;
            if (pSnd)
                volAdj = pSnd->fVolAdj;
            else
                // if the UserData comes back `nullptr` then this channel is now invalid, has stopped playing
                chn = nullptr;

            // Automatic Handling of this event?
            if (abSndAuto[eSndEvent])
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
                        const float vol = std::clamp<float>((fVal - def.valMin) / (def.valMax - def.valMin), 0.0f, 1.0f);
                        // If there hasn't been a sound triggered do so now
                        if (!chn) {
                            chn = SoundPlay(SoundGetName(eSndEvent), vol);
                            LOG_MATCHING(logINFO, "Aircraft %08X (%s): Looping sound '%s' at volume %.2f for '%s'",
                                         modeS_id, GetFlightId().c_str(),
                                         SoundGetName(eSndEvent).c_str(), vol, SoundEventTxt(eSndEvent));
                        }
                    } else {
                        // There should be no sound, remove it if there was one
                        if (chn) {
                            SoundStop(chn);
                            LOG_MATCHING(logINFO, "Aircraft %08X (%s): Stopped sound for '%s'",
                                         modeS_id, GetFlightId().c_str(),
                                         SoundEventTxt(eSndEvent));
                        }
                        chn = nullptr;
                    }
                }
                // --- One-time event ---
                else
                {
                    // Should there be sound because the value changed?
                    if (!std::isnan(lastVal) && (lastVal != fVal)) {
                        // If there hasn't been a sound triggered do so now
                        if (!chn) {
                            chn = SoundPlay(SoundGetName(eSndEvent), volAdj);
                            LOG_MATCHING(logINFO, "Aircraft %08X (%s): Playing sound '%s' once at volume %.2f for '%s'",
                                         modeS_id, GetFlightId().c_str(),
                                         SoundGetName(eSndEvent).c_str(), volAdj, SoundEventTxt(eSndEvent));
                        }
                    } else {
                        // Now (more) value change, stop and remove the sound
                        if (chn) SoundStop(chn);
                        chn = nullptr;
                    }
                    // Remember the last value we've seen
                    lastVal = fVal;
                }
                
                // --- Low pass filter in case we're inside a cockpit ---
                if (chn && eLP) {
                    FMOD_LOG(FMOD_Channel_SetLowPassGain(chn,
                                                         eLP == LP_Enable ? FMOD_LOW_PASS_GAIN : 1.0f));
                }
                
                // --- Expensive Compuations: Cone Orientation ---
                if (bDoExpensiveComp && chn && pSnd && pSnd->hasConeInfo())
                {
                    pSnd->setConeOrientation(chn, GetHeading(), GetPitch());
                }
                
                if (chn) {
                    // TODO: Remove me
                    float in = NAN, out = NAN, vol = NAN;
                    FMOD_VECTOR vec { NAN, NAN, NAN };
                    FMOD_Channel_Get3DConeSettings(chn, &in, &out, &vol);
                    FMOD_Channel_Get3DConeOrientation(chn, &vec);
                    if (in == out == vol)
                        continue;
                }
                
            }
            // No automatic handling of this event
            else
            {
                // So if there currently is a channel, remove it
                if (chn) {
                    SoundStop(chn);
                    chn = nullptr;
                }
            }
        }
        
    }
    FMOD_CATCH;
}

// Make sure a Sound Grp is available and update its 3D location
bool Aircraft::SoundUpdateGrp ()
{
    // If we don't want sound we don't get sound
    if (!XPMPSoundIsEnabled()) return false;
    
    try {
        // Make sure a Sound Group is available
        if (!pChnGrp) {
            char s[20];                     // use modeS id in hey as channel name
            snprintf(s, sizeof(s), "%08X", modeS_id);
            FMOD_TEST(FMOD_System_CreateChannelGroup(gpFmodSystem, s, &pChnGrp));
            LOG_ASSERT(pChnGrp != nullptr);
            FMOD_TEST(FMOD_ChannelGroup_SetMode(pChnGrp, FMOD_3D));
            FMOD_TEST(FMOD_ChannelGroup_Set3DMinMaxDistance(pChnGrp, 10.f, 1000.f));
        }
        
        // Update 3D sound location relative to camera position (we keep camera at zero)
        FMOD_VECTOR fmodPos {
            drawInfo.x - glob.posCamera.x,
            drawInfo.y - glob.posCamera.y,
            drawInfo.z - glob.posCamera.z,
        };
        FMOD_VECTOR fmodVel { v_x, v_y, v_z };
        FMOD_TEST(FMOD_ChannelGroup_Set3DAttributes(pChnGrp, &fmodPos, &fmodVel));
        return true;
    }
    catch (const FmodError& e) {
        e.LogErr();                             // log the error
        SoundCleanup();                         // cleanup and disable sound system...here there shall be no errors!
        return false;
    }
}

// Remove all sound, e.g. during destruction
void Aircraft::SoundRemoveAll ()
{
    // Stop all sounds from this aircraft
    if (pChnGrp) {
        FMOD_ChannelGroup_Stop(pChnGrp);
        FMOD_ChannelGroup_Release(pChnGrp);
        pChnGrp = nullptr;
        LOG_MATCHING(logDEBUG, "Aircraft %08X (%s): Removed all sounds",
                     modeS_id, GetFlightId().c_str());
    }
    // All channels now stopped and invalid
    for (FMOD_CHANNEL* &chn: apChn) chn = nullptr;
}


//
// MARK: Global Functions
//

// Initialize the sound module and load the sounds
bool SoundInit ()
{
    try {
        if (!gpFmodSystem) {
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
            
            // Set master volume
            XPMPSoundSetMasterVolume(glob.sndMasterVol);
            
            // Load the default sounds
            if (!SoundFile::LoadXPSounds()) {
                LOG_MSG(logERR, "No standard sounds successfully loaded, sound system currently inactive!");
                return false;
            }
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

// Tell FMOD that all updates are done
void SoundUpdatesDone ()
{
    if (gpFmodSystem) try {
        // Update the listener position and orientation
        const FMOD_VECTOR posNull   = { 0.0f, 0.0f, 0.0f };             // We keep the listener at the coordinate system's center
        const FMOD_VECTOR velocity  = { glob.vCam_x, glob.vCam_y, glob.vCam_z };
        // The forward direction takes the heading into account
        // TODO: Listener: Take pitch and roll into account
        const FMOD_VECTOR normForw  = { std::sin(deg2rad(glob.posCamera.heading)), 0.0f, std::cos(deg2rad(glob.posCamera.heading - 180)) };
        const FMOD_VECTOR normUpw   = { 0.0f, 1.0f, 0.0f };     // Upward is current always straight up
        FMOD_TEST(FMOD_System_Set3DListenerAttributes(gpFmodSystem, 0, &posNull, &velocity, &normForw, &normUpw));
        
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
                          float fVolAdj,
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
                                std::forward_as_tuple(filePath,bLoop,fVolAdj,
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
