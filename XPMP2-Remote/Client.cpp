/// @file       Client.cpp
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

#include "XPMP2-Remote.h"

#define INFO_NEW_SENDER_PLUGIN  "First data received from %.*s (%u) @ %s"
#define INFO_GOT_AI_CONTROL     "Have TCAS / AI control now"
#define INFO_DEACTIVATED        "Deactivated, stopped listening to network"

//
// MARK: Aircraft Administration
//

/// Contains 'now' at the time the flight loop starts
std::chrono::time_point<std::chrono::steady_clock> nowFlightLoop;

// Constructor for use in network thread: Does _not_ Create the aircraft but only stores the passed information
RemoteAC::RemoteAC (const XPMP2::RemoteAcDetailTy& _acDetails) :
XPMP2::Aircraft()               // do _not_ create an actual plane!
{
    // Initialization
    histPos[0] = {sizeof(XPLMDrawInfo_t), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    histPos[1] = histPos[0];
    
    // temporarily store the information without doing anything with it yet
    pAcDetail = new XPMP2::RemoteAcDetailTy(_acDetails);
    LOG_ASSERT(pAcDetail);
}

// Destructor
RemoteAC::~RemoteAC()
{
    Free();
}

// Actually create the aircraft, ultimately calls XPMP2::Aircraft::Create()
void RemoteAC::Create ()
{
    LOG_ASSERT(pAcDetail);

    // Save all information
    Update(*pAcDetail);

    // Create the actual plane
    Aircraft::Create(acIcaoType, acIcaoAirline, acLivery,
                     pAcDetail->GetModeSId(), "",
                     XPMP2::CSLModelByPkgShortId(pAcDetail->pkgHash,
                                                 pAcDetail->sShortId, sizeof(pAcDetail->sShortId)));
    
    // Initialize both historic positions to the only position we know
    histPos[0] = histPos[1];
    histTs[0]  = histTs[1];
    
    // don't need pAcDetail any longer, free up that space
    Free();
}

// Update data from a a/c detail structure
void RemoteAC::Update (const XPMP2::RemoteAcDetailTy& _acDetails)
{
    acIcaoType      = STR_N(_acDetails.icaoType);
    acIcaoAirline   = STR_N(_acDetails.icaoOp);
    acLivery.clear();
    label = STR_N(_acDetails.label);
    _acDetails.GetLabelCol(colLabel);

    // We might be in a worker thread...only temporarily store the position
    lat = _acDetails.lat;
    lon = _acDetails.lon;
    alt_ft = double(_acDetails.alt_ft);
    histTs[2] = std::chrono::steady_clock::now();
    bWorldCoordUpdated = true;          // flag for UpdatePosition() to read fresh data

    drawInfo.pitch      = _acDetails.GetPitch();
    drawInfo.heading    = _acDetails.GetHeading();
    drawInfo.roll       = _acDetails.GetRoll();
    aiPrio              = _acDetails.aiPrio;
    
    // Validity and Visibility
    SetVisible(_acDetails.bVisible);
    if (!_acDetails.bValid)
        SetInvalid();
    
    // Animation dataRefs
    for (size_t i = 0; i < XPMP2::V_COUNT; ++i)
        v[i] = XPMP2::REMOTE_DR_DEF[i].unpack(_acDetails.v[i]);
}

// Update data from an a/c position update
void RemoteAC::Update (const XPMP2::RemoteAcPosUpdateTy& _acPosUpd)
{
    // Update position information based on diff values in the position update
    lat     += XPMP2::REMOTE_DEGREE_RES * double(_acPosUpd.dLat);
    lon     += XPMP2::REMOTE_DEGREE_RES * double(_acPosUpd.dLon);
    alt_ft  += XPMP2::REMOTE_ALT_FT_RES * double(_acPosUpd.dAlt_ft);
    histTs[2] = std::chrono::steady_clock::now();
    bWorldCoordUpdated = true;          // flag for UpdatePosition() to read fresh data

    drawInfo.pitch      = _acPosUpd.GetPitch();
    drawInfo.heading    = _acPosUpd.GetHeading();
    drawInfo.roll       = _acPosUpd.GetRoll();
}

// Called by XPMP2 for position updates, extrapolates from historic positions
void RemoteAC::UpdatePosition (float, int)
{
    // If we have a fresh world position then that's the one that counts
    if (bWorldCoordUpdated) {
        SetLocation(lat, lon, alt_ft);      // Convert to local, stored in drawInfo
        bWorldCoordUpdated = false;

        // cycle the historic positions and save the new one
        histPos[0] = histPos[1];
        histTs[0]  = histTs[1];
        histPos[1] = drawInfo;
        histTs[1]  = histTs[2];             // [2] was also set in Update()
    }
    
    std::chrono::steady_clock::duration dHist = histTs[1] - histTs[0];
    if (dHist == std::chrono::steady_clock::duration::zero())
        drawInfo = histPos[1];
    else {
        // extrapolate current position based on the two historic ones and current time
        std::chrono::steady_clock::duration dNow = nowFlightLoop - histTs[0];
        try {
            const float f = float(dNow / dHist);
            drawInfo.x       = histPos[0].x       + f * (histPos[1].x       - histPos[0].x);
            drawInfo.y       = histPos[0].y       + f * (histPos[1].y       - histPos[0].y);
            drawInfo.z       = histPos[0].z       + f * (histPos[1].z       - histPos[0].z);
            drawInfo.pitch   = histPos[0].pitch   + f * (histPos[1].pitch   - histPos[0].pitch);
            drawInfo.heading = histPos[0].heading + f * (histPos[1].heading - histPos[0].heading);
            drawInfo.roll    = histPos[0].roll    + f * (histPos[1].roll    - histPos[0].roll);
        }
        catch (...) {
            drawInfo = histPos[1];
        }
    }
    
    // TODO: Animation dataRefs
}

// frees the temporary stored network msg
void RemoteAC::Free ()
{
    if (pAcDetail)
        delete pAcDetail;
    pAcDetail = nullptr;
}

//
// MARK: Sender Administration
//

// TODO: Identify stale connections and drop them

bool SenderAddrTy::operator< (const SenderAddrTy& o) const
{
    return memcmp(this, &o, sizeof(*this)) < 0;
}

// Constructor copies values and sets lastMsg to now
SenderTy::SenderTy (const std::string& _from, const XPMP2::RemoteMsgSettingsTy& _s) :
sFrom(_from), settings(_s), lastMsg(std::chrono::steady_clock::now())
{}

// Find a sender based on plugin id and IP address
SenderTy* SenderTy::Find (XPLMPluginID _id, std::uint32_t _from[4])
{
    SenderAddrTy senderId (_id, _from);
    mapSenderTy::iterator itSender = rcGlob.gmapSender.find(senderId);
    if (itSender != rcGlob.gmapSender.end())
        return &itSender->second;
    return nullptr;
}

//
// MARK: Process Messages
//

bool bWaitingForAI = false;         ///< have we requested AI access and are now waiting for a callback?
std::mutex gmutexData;              ///< the mutex controlling access to all data between network and main thread
/// The lock used by _main thread_ functions to acquire data access
std::unique_lock<std::mutex> glockDataMain(gmutexData, std::defer_lock);
/// Indicates if it is needed in the main thread to process updates to settings
std::atomic_flag gbSkipSettingsUpdate = ATOMIC_FLAG_INIT;
/// Indicates if it is needed in the main thread to process new aircraft
std::atomic_flag gbSkipNewAc = ATOMIC_FLAG_INIT;

void ClientTryGetAI ();

/// Callback for requesting TCAS: Maybe now we have a chance?
void ClientCBRetryGetAI (void*)
{
    bWaitingForAI = false;
    
    // If we still want to have TCAS we just try again
    if (rcGlob.mergedS.bHaveTCASControl && !XPMPHasControlOfAIAircraft())
        ClientTryGetAI();
}

// Try getting TCAS/AI control
void ClientTryGetAI ()
{
    // make sure we do this from the main thread only!
    assert (rcGlob.IsXPThread());
    
    // no need to try (again)?
    if (bWaitingForAI || XPMPHasControlOfAIAircraft())
        return;
    
    // Try getting AI control, pass callback for the case we couldn't get it
    const char* cszResult = XPMPMultiplayerEnable(ClientCBRetryGetAI);
    if ( cszResult[0] ) {
        bWaitingForAI = true;
        LOG_MSG(logWARN, "%s", cszResult);
    } else if (XPMPHasControlOfAIAircraft()) {
        bWaitingForAI = false;
        LOG_MSG(logINFO, INFO_GOT_AI_CONTROL);
    }
}

// Stop TCAS/AI control
void ClientReleaseAI ()
{
    XPMPMultiplayerDisable();
    bWaitingForAI = false;
}

/// @brief Handles a received Settings message, which also identifies a sending plugin
/// @details 1. Plugin information is stored in gmapSender.
///          2. A "merged" version of all sender plugins' settings is created,
///             make a per-field decision for the more significant value.
///             This merged version shall become our local settings.
///          3. In a last step the changes to current actual settings are determined
///             and necessary action taken.
void ClientProcSettings (std::uint32_t from[4],
                         const std::string& sFrom,
                         const XPMP2::RemoteMsgSettingsTy& _msgSettings)
{
    // Require access
    std::lock_guard<std::mutex> lk(gmutexData);
    
    // 1. Store sender information
    SenderTy* pSender = SenderTy::Find(_msgSettings.pluginId, from);
    if (!pSender) {
        // Seeing this plugin the first time!
        LOG_MSG(logINFO, INFO_NEW_SENDER_PLUGIN,
                (int)sizeof(_msgSettings.name), _msgSettings.name,
                _msgSettings.pluginId,
                sFrom.c_str());
        rcGlob.gmapSender.emplace(SenderAddrTy (_msgSettings.pluginId, from),
                                  SenderTy(sFrom, _msgSettings));
        
        // If this one also happens to be the first plugin at all then it defines
        // some settings that cannot be well merged
        if (rcGlob.gmapSender.size() == 1) {
            // packed config structure does not have zero-termination
            std::string defAc  = STR_N(_msgSettings.defaultIcao);
            std::string defCar = STR_N(_msgSettings.carIcaoType);
            XPMPSetDefaultPlaneICAO(defAc.c_str(), defCar.c_str());
        }
    }
    else
    {
        // plugin is known, update its settings
        pSender->settings = _msgSettings;
        pSender->lastMsg  = std::chrono::steady_clock::now();
    }
    
    // 2. Consolidate/merge the settings of all plugins
    mapSenderTy::iterator itSender = rcGlob.gmapSender.begin();
    LOG_ASSERT(itSender != rcGlob.gmapSender.end());
    rcGlob.mergedS = itSender->second.settings;
    for (++itSender; itSender != rcGlob.gmapSender.end(); ++itSender)
    {
        const XPMP2::RemoteMsgSettingsTy& otherS = itSender->second.settings;
        if (otherS.logLvl < rcGlob.mergedS.logLvl) rcGlob.mergedS.logLvl = otherS.logLvl;
        if ( otherS.bLogMdlMatch)               rcGlob.mergedS.bLogMdlMatch = true;
        if ( otherS.bObjReplDataRefs)           rcGlob.mergedS.bLogMdlMatch = true;
        if ( otherS.bObjReplTextures)           rcGlob.mergedS.bLogMdlMatch = true;
        if (!otherS.bLabelCutOffAtVisibility)   rcGlob.mergedS.bLabelCutOffAtVisibility = false;
        if ( otherS.bMapEnabled)                rcGlob.mergedS.bMapEnabled = true;
        if ( otherS.bMapLabels)                 rcGlob.mergedS.bMapLabels = true;
        if ( otherS.bHaveTCASControl)           rcGlob.mergedS.bHaveTCASControl = true;
    }
    
    // 3. Take action based on whatever settings differ between mergedS and my current settings
    XPMPSetAircraftLabelDist(rcGlob.mergedS.maxLabelDist / XPMP2::M_per_NM,
                             rcGlob.mergedS.bLabelCutOffAtVisibility);
    // The next flight loop cycle needs to update some stuff
    gbSkipSettingsUpdate.clear();
}

// Called at the beginning of each flight loop processing: Get the data lock, create waiting planes
void ClientFlightLoopBegins ()
{
    // Any main thread stuff to do due to settings updates?
    if (!gbSkipSettingsUpdate.test_and_set()) {
        XPMPEnableMap(rcGlob.mergedS.bMapEnabled,
                      rcGlob.mergedS.bMapLabels);
        if (rcGlob.mergedS.bHaveTCASControl)
            ClientTryGetAI();
    }
    
    // Store current time once for all position calculations
    nowFlightLoop = std::chrono::steady_clock::now();
    GetMiscNetwTime();

    // Acquire the data access lock once and keep it while the flight loop is running
    glockDataMain.lock();

    // If needed create new aircraft the have been prepared in the meantime
    if (gbSkipNewAc.test_and_set()) {
        for (auto& s: rcGlob.gmapSender) {              // loop all senders
            for (auto& a: s.second.mapAc) {             // loop all a/c of that sender                
                if (a.second.GetModeS_ID() == 0)        // create a/c if not created
                    a.second.Create();
            }
        }
    }
}

// Called at the end of each flight loop processing: Release the data lock
void ClientFlightLoopEnds ()
{
    // Release the data access lock
    glockDataMain.unlock();
}

/// @brief Handle A/C Details messages, called by XPMP2 via callback
/// @details 1. If the aircraft does not exist create it
///          2. Else update it's data
void ClientProcAcDetails (std::uint32_t _from[4], size_t _msgLen,
                          const XPMP2::RemoteMsgAcDetailTy& _msgAcDetails)
{
    // Find the sender, bail if we don't know it
    SenderTy* pSender = SenderTy::Find(_msgAcDetails.pluginId, _from);
    if (!pSender) return;
    
    // Loop over all aircraft contained in the message
    const size_t numAc = _msgAcDetails.NumElem(_msgLen);
    for (size_t i = 0; i < numAc; ++i) {
        const XPMP2::RemoteAcDetailTy& acDetails = _msgAcDetails.arr[i];
        // Is the aircraft known?
        mapRemoteAcTy::iterator iAc = pSender->mapAc.find(acDetails.GetModeSId());
        // Now require access (for each plane, because we want to give the main thread's flight loop a better change to grab the lock if needed)
        std::unique_lock<std::mutex> lk(gmutexData);
        // Is the aircraft known?
        if (iAc == pSender->mapAc.end()) {
            // new aircraft, create an object for it, but not yet the actual plane (as this is the network thread)
            pSender->mapAc.emplace(acDetails.GetModeSId(), acDetails);
            // tell the main thread that it shall process new a/c
            gbSkipNewAc.clear();
        } else {
            // known aircraft, update its data
            iAc->second.Update(acDetails);
        }
        lk.unlock();
    }
}

/// Handle A/C Position Update message, called by XPMP2 via callback
void ClientProcAcPosUpdate (std::uint32_t _from[4], size_t _msgLen,
                            const XPMP2::RemoteMsgAcPosUpdateTy& _msgAcPosUpdate)
{
    // Find the sender, bail if we don't know it
    SenderTy* pSender = SenderTy::Find(_msgAcPosUpdate.pluginId, _from);
    if (!pSender) return;
    
    // Loop over all aircraft contained in the message
    const size_t numAc = _msgAcPosUpdate.NumElem(_msgLen);
    for (size_t i = 0; i < numAc; ++i) {
        const XPMP2::RemoteAcPosUpdateTy& acPosUpd = _msgAcPosUpdate.arr[i];
        // Is the aircraft known?
        mapRemoteAcTy::iterator iAc = pSender->mapAc.find(XPMPPlaneID(acPosUpd.modeS_id));
        // Is the aircraft known?
        if (iAc != pSender->mapAc.end()) {
            // Now require access (for each plane, because we want to give the main thread's flight loop a better change to grab the lock if needed)
            std::lock_guard<std::mutex> lk(gmutexData);
            // known aircraft, update its data
            iAc->second.Update(acPosUpd);
        }
    }

}


//
// MARK: Global Functions
//

// Initializes the module
void ClientInit()
{
    // Initialize the atomic flags to 'set'
    gbSkipSettingsUpdate.test_and_set();
    gbSkipNewAc.test_and_set();
}

// Shuts down the module gracefully
void ClientCleanup()
{
    // Force off
    ClientToggleActive(-1);
}


// Toggles the cient's activitiy status, as based on menu command
void ClientToggleActive (int nForce)
{
    // Need to start?
    if (nForce > 0 ||
        XPMP2::RemoteGetStatus() == XPMP2::REMOTE_OFF)
    {
        // Start the listener to receive message
        XPMP2::RemoteCBFctTy rmtCBFcts = {
            ClientFlightLoopBegins,         // before flight loop processing starts
            ClientFlightLoopEnds,           // after flight loop processing ends
            ClientProcSettings,             // Settings
            ClientProcAcDetails,            // Aircraft Details
            ClientProcAcPosUpdate,          // Aircraft Position Update
        };
        XPMP2::RemoteRecvStart(rmtCBFcts);
    }
    // Need to stop?
    else if (nForce < 0 ||
             XPMP2::RemoteGetStatus() != XPMP2::REMOTE_OFF)
    {
        // Stop the listener
        XPMP2::RemoteRecvStop();
        // Shut down everything
        ClientReleaseAI();
        rcGlob.gmapSender.clear();      // removes aircraft in turn
        LOG_MSG(logINFO, INFO_DEACTIVATED);
    }
}
