/// @file       Sound.h
/// @brief      Sound for aircraft, based on the FMOD library
///
/// @note       If built with `INCLUDE_FMOD_SOUND=1` then
///             Audio Engine is FMOD Core API by Firelight Technologies Pty Ltd.
///             Understand FMOD [licensing](https://www.fmod.com/licensing) and
///             [attribution requirements](https://www.fmod.com/attribution) first!
///
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

#pragma once

// Only included if specified. Understand FMOD licensing and attribution first!
#if INCLUDE_FMOD_SOUND + 0 >= 1

namespace XPMP2 {

/// Represents a sound file to be passed on to FMOD to be played
class SoundFMOD : public SoundFile {
public:
    FMOD_CHANNELGROUP* pChGrp = nullptr;
protected:
    FMOD_SOUND* pSound = nullptr;   ///< FMOD sound object
    bool bLoaded = false;           ///< cached version of the sound's `openstate`
    
public:
    /// Constructor creates the FMOD sound object and initiates loading of the file
    SoundFMOD (const std::string& _filePath, bool _bLoop,
               float _coneDir, float _conePitch,
               float _coneInAngle, float _coneOutAngle, float _coneOutVol);
    /// Destructor removes the sound object
    ~SoundFMOD() override;
    
    /// Set the channel group this sound shall play on, used in next play()
    void setChGrp (FMOD_CHANNELGROUP* p) { pChGrp = p; }
    
    /// Finished loading, ready to play?
    bool isReady () override;
    
    /// @brief Play the sound
    /// @param bPaused [opt] Shall channel stay paused after creation?
    /// @return `nullptr` in case of failure
    FMOD_CHANNEL* play (bool bPaused = false) override;
    
    /// @brief Set the channel's sound cone relative to the aircraft's orientation in space and the cone configuration of the SoundFile
    /// @param pChn The channel, which is to be modified
    /// @param ac Aircraft
    void setConeOrientation (FMOD_CHANNEL* pChn, const Aircraft& ac) override;
};

}

#endif // INCLUDE_FMOD_SOUND
