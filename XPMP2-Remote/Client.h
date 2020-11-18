/// @file       Client.h
/// @brief      Main Client functionality, processing received data
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

//
// MARK: Aircraft Administration
//

struct SenderTy;

/// Representation of a remote aircraft
class RemoteAC : public XPMP2::Aircraft {
protected:
    // A few defining information taken over from the sender
    XPMPPlaneID   senderId = 0;         ///< sender's plane id (might be different here in case of deduplication)
    std::uint16_t pkgHash = 0;          ///< hash value of CSL model package
    std::string   sShortId;             ///< CSL model's short id
    bool bDeleteMe = false;             ///< flag that this a/c needs to removed

    /// @brief We keep 2 historic positions to be able to calculate simple linear extrapolation
    /// @details Index 0 is the older one, index 1 the newer one
    XPLMDrawInfo_t histPos[2];
    /// Timestamps when these two positions were valid
    std::chrono::time_point<std::chrono::steady_clock> histTs[2];
    
    // World coordinates
    double lat      = NAN;              ///< latitude
    double lon      = NAN;              ///< longitude
    double alt_ft   = NAN;              ///< altitude [ft]
    std::chrono::duration<int,std::ratio<1, 10000>> diffTime;   ///< time difference to previous historic position as passed in by the sender
    bool bWorldCoordUpdated = false;    ///< shall freshly set values be used in the next UpdatePosition callback?
    
    // Did the CSL model change? Will force model-reload during UpdatePosition()
    bool bCSLModelChanged = false;
    
    // The sending plugin this plane came from
    SenderTy& sender;
    
public:
    /// Constructor for use in network thread: Does _not_ Create the aircraft but only stores the passed information
    RemoteAC (SenderTy& _sender, const XPMP2::RemoteAcDetailTy& _acDetails);
    /// Destructor 
    virtual ~RemoteAC();
    
    /// Actually create the aircraft, ultimately calls XPMP2::Aircraft::Create()
    void Create ();
    
    /// Update data from an a/c detail structure
    void Update (const XPMP2::RemoteAcDetailTy& _acDetails);
    /// Update data from an a/c position update
    void Update (const XPMP2::RemoteAcPosUpdateTy& _acPosUpd);
    /// Update data from an a/c animation dataRefs message
    void Update (const XPMP2::RemoteAcAnimTy& _acAnim);

    /// Called by XPMP2 for position updates, extrapolates from historic positions
    void UpdatePosition (float, int) override;
    
    /// Mark this aircraft as to-be-deleted in the next flight loop (can't do in worker thread)
    void MarkForDeletion () { bDeleteMe = true; }
    /// To be deleted?
    bool IsToBeDeleted () const { return bDeleteMe; }
};

/// Map of remote aircraft; key is the plane id as sent by the sending plugin (while the _modeS_id of the local copy could differ)
typedef std::map<XPMPPlaneID,RemoteAC> mapRemoteAcTy;

//
// MARK: Sender Administration
//

/// Uniquely identifies a sending plugin
struct SenderAddrTy {
    const XPLMPluginID pluginId;        ///< sender's plugin id within X-Plane (first to speed up memcmp as usually this will already differ between plugins)
    std::uint32_t from[4];              ///< numerical part of sender's IP address (only differentiating in rare cases of two plugins on different PCs having the same plugin id)
    bool operator< (const SenderAddrTy& o) const;   ///< uses memcmp to compare this
    /// Constructor just fills values
    SenderAddrTy (XPLMPluginID _id, const std::uint32_t _from[4]) :
    pluginId(_id) { memcpy(from, _from, sizeof(from)); }
};

struct SenderTy {
    std::string sFrom;                  ///< string representaton of the sender's IP address
    const bool bLocal;                  ///< is this a local sender on the same computer?
    XPMP2::RemoteMsgSettingsTy settings;///< that plugin's settings
    std::chrono::time_point<std::chrono::steady_clock> lastMsg;
    mapRemoteAcTy mapAc;                ///< map of aircraft sent by this plugin
    
    /// Constructor copies values and sets lastMsg to now
    SenderTy (const std::string& _from, const XPMP2::RemoteMsgSettingsTy& _s);
    
    /// Find a sender based on plugin id and IP address
    static SenderTy* Find (XPLMPluginID _id, const std::uint32_t _from[4]);
};

/// map of sender information
typedef std::map<SenderAddrTy,SenderTy> mapSenderTy;

//
// MARK: Global Functions
//

/// Initializes the module
void ClientInit();
/// Shuts down the module gracefully
void ClientCleanup();

/// Try getting TCAS/AI control
void ClientTryGetAI ();
/// Stop TCAS/AI control
void ClientReleaseAI ();

/// @brief Toggles the cient's activitiy status, as based on menu command
/// @param nForce 3-way toggle: `-1?  force off, `0` toggle, `+1` force on
void ClientToggleActive (int nForce = 0);

/// Called at the beginning of each flight loop processing: Get the data lock, create waiting planes
void ClientFlightLoopBegins ();
/// Called at the end of each flight loop processing: Release the data lock
void ClientFlightLoopEnds () noexcept;
