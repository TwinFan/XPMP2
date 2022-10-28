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
#define INFO_SENDER_PLUGIN_LOST "Sender %.*s (%u) @ %s fell silent, removed"
#define WARN_LOST_PLANE_CONTACT "Lost contact to plane 0x%06X of %.*s (%u) @ %s"
#define INFO_GOT_AI_CONTROL     "Have TCAS / AI control now"
#define INFO_DEACTIVATED        "Deactivated, stopped listening to network"

//
// MARK: Aircraft Administration
//

/// Contains 'now' at the time the flight loop starts
std::chrono::time_point<std::chrono::steady_clock> nowFlightLoop;

// Constructor for use in network thread: Does _not_ Create the aircraft but only stores the passed information
RemoteAC::RemoteAC (SenderTy& _sender, const XPMP2::RemoteAcDetailTy& _acDetails) :
XPMP2::Aircraft(),              // do _not_ create an actual plane!
senderId(_acDetails.modeS_id),
pkgHash(_acDetails.pkgHash),
sShortId(STR_N(_acDetails.sShortId)),
sender(_sender)
{
    // Initialization
    histPos[0] = {sizeof(XPLMDrawInfo_t), NAN, NAN, NAN, 0.0f, 0.0f, 0.0f};
    histPos[1] = histPos[0];
    
    // store the a/c detail information
    Update(_acDetails);
}

// Destructor
RemoteAC::~RemoteAC()
{}

// Actually create the aircraft, ultimately calls XPMP2::Aircraft::Create()
void RemoteAC::Create ()
{
    // Create the actual plane
    Aircraft::Create(acIcaoType, acIcaoAirline, acLivery, senderId, "",
                     XPMP2::CSLModelByPkgShortId(pkgHash, sShortId));
    bCSLModelChanged = false;
}

// Update data from a a/c detail structure
void RemoteAC::Update (const XPMP2::RemoteAcDetailTy& _acDetails)
{
    acIcaoType      = STR_N(_acDetails.icaoType);
    acIcaoAirline   = STR_N(_acDetails.icaoOp);
    acLivery.clear();

    // CSL Model: Did it change? (Will be set during UpdatePosition())
    bCSLModelChanged = (pkgHash != _acDetails.pkgHash) || (sShortId != _acDetails.sShortId);
    if (bCSLModelChanged) {
        pkgHash  = _acDetails.pkgHash;
        sShortId = _acDetails.sShortId;
    }
    
    label = STR_N(_acDetails.label);
    _acDetails.GetLabelCol(colLabel);
    // Labels are forced on if our local config says so
    bDrawLabel = _acDetails.bDrawLabel || (rcGlob.eDrawLabels == XPMP2RCGlobals::LABELS_ON);

    // We might be in a worker thread...only temporarily store the position
    lat = _acDetails.lat;
    lon = _acDetails.lon;
    alt_ft = double(_acDetails.alt_ft);
    diffTime = std::chrono::duration<int,std::ratio<1, 10000>>(_acDetails.dTime);
    bWorldCoordUpdated = true;          // flag for UpdatePosition() to read fresh data

    drawInfo.pitch      = _acDetails.GetPitch();
    drawInfo.heading    = _acDetails.GetHeading();
    drawInfo.roll       = _acDetails.GetRoll();
    aiPrio              = _acDetails.aiPrio;
    bOnGrnd             = _acDetails.bOnGrnd;
    
    // Info texts
#if defined(__GNUC__) && (__GNUC__ >= 9)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
    std::memset(&acInfoTexts, 0, sizeof(acInfoTexts));
#if defined(__GNUC__) && (__GNUC__ >= 9)
#pragma GCC diagnostic pop
#endif
#define memcpy_min(to,from) std::memcpy(to,from,std::min(sizeof(from),sizeof(to)))
    memcpy_min(acInfoTexts.tailNum,         _acDetails.tailNum);
    memcpy_min(acInfoTexts.icaoAcType,      _acDetails.icaoType);
    memcpy_min(acInfoTexts.manufacturer,    _acDetails.manufacturer);
    memcpy_min(acInfoTexts.model,           _acDetails.model);
    memcpy_min(acInfoTexts.icaoAirline,     _acDetails.icaoOp);
    memcpy_min(acInfoTexts.airline,         _acDetails.airline);
    memcpy_min(acInfoTexts.flightNum,       _acDetails.flightNum);
    memcpy_min(acInfoTexts.aptFrom,         _acDetails.aptFrom);
    memcpy_min(acInfoTexts.aptTo,           _acDetails.aptTo);

    // Don't render local planes (the local plugin does already),
    // or if explicitely instructed so by the sender
    SetRender(sender.bLocal ? false : _acDetails.bRender);

    // Validity and Visibility
    SetVisible(_acDetails.bVisible);
    if (!_acDetails.bValid)
        SetInvalid();
    
    // Animation dataRefs
    for (size_t i = 0; i < XPMP2::V_COUNT; ++i)
        v[i] = XPMP2::REMOTE_DR_DEF[i].unpack(_acDetails.v[i]);
/*
    LOG_MSG(logDEBUG, " 0x%06X: %.7f / %.7f, %.1f",
            GetModeS_ID(), lat, lon, alt_ft);*/
}

// Update data from an a/c position update
void RemoteAC::Update (const XPMP2::RemoteAcPosUpdateTy& _acPosUpd)
{
    // Update position information based on diff values in the position update
    lat     += XPMP2::REMOTE_DEGREE_RES * double(_acPosUpd.dLat);
    lon     += XPMP2::REMOTE_DEGREE_RES * double(_acPosUpd.dLon);
    alt_ft  += XPMP2::REMOTE_ALT_FT_RES * double(_acPosUpd.dAlt_ft);
    diffTime = std::chrono::duration<int,std::ratio<1, 10000>>(_acPosUpd.dTime);
    bWorldCoordUpdated = true;          // flag for UpdatePosition() to read fresh data

    drawInfo.pitch      = _acPosUpd.GetPitch();
    drawInfo.heading    = _acPosUpd.GetHeading();
    drawInfo.roll       = _acPosUpd.GetRoll();
/*
    LOG_MSG(logDEBUG, "0x%06X: %.7f / %.7f, %.1f  <-- %+.7f / %+.7f, %+.1f",
            GetModeS_ID(), lat, lon, alt_ft,
            _acPosUpd.dLat * XPMP2::REMOTE_DEGREE_RES,
            _acPosUpd.dLon * XPMP2::REMOTE_DEGREE_RES,
            _acPosUpd.dAlt_ft * XPMP2::REMOTE_ALT_FT_RES); */
}

// Update data from an a/c animation dataRefs message
void RemoteAC::Update (const XPMP2::RemoteAcAnimTy& _acAnim)
{
    // Loop over all includes values and update the respective dataRef values
    for (std::uint8_t idx = 0; idx < _acAnim.numVals; ++idx) {
        const XPMP2::RemoteAcAnimTy::DataRefValTy& dr = _acAnim.v[idx];
        v[dr.idx] = XPMP2::REMOTE_DR_DEF[dr.idx].unpack(dr.v);
    }
}

// Called by XPMP2 for position updates, extrapolates from historic positions
void RemoteAC::UpdatePosition (float _elapsed, int)
{
    // Did the CSL model change?
    if (bCSLModelChanged) {
        bCSLModelChanged = false;
        XPMP2::CSLModel *pNewCSLModel = XPMP2::CSLModelByPkgShortId(pkgHash, sShortId);
        if (pNewCSLModel && pNewCSLModel != pCSLMdl)
            AssignModel("", pNewCSLModel);
    }
    
    // If we have a fresh world position then that's the one that counts
    if (bWorldCoordUpdated) {
        SetLocation(lat, lon, alt_ft);      // Convert to local, stored in drawInfo
        bWorldCoordUpdated = false;

        // cycle the historic positions and save the new one
        histPos[0] = histPos[1];
        histPos[1] = drawInfo;
        // Timestamp: This one [1] is valid _now_
        histTs[1]  = nowFlightLoop;
        histTs[0]  = histTs[1] - diffTime;

        // Special handling for heading as [0] and [1] could be on different sides of 0 / 360 degrees:
        // We push things towards positive figures larger than 360
        // as we can then just use fmod(h,360) and don't care about negatives
        if (histPos[0].heading >= 360.0f)       // if we increased the value last time we need to normalize it before re-calculation
            histPos[0].heading -= 360.0f;
        const float diffHead = histPos[0].heading - histPos[1].heading;
        if (diffHead > 180.0f)
            histPos[1].heading += 360.0f;
        else if (diffHead < -180.0f)
            histPos[0].heading += 360.0f;
    }
    // extrapolate based on last 2 positions if 2 positions are known with differing timestamps
    else if (!std::isnan(histPos[0].x) && histTs[1] > histTs[0]) {
        const std::chrono::steady_clock::duration dHist = histTs[1] - histTs[0];
        const std::chrono::steady_clock::duration dNow = nowFlightLoop - histTs[0];
        
        // We extrapolate only for a maximum of 1s, after that planes just stop
        // (If the sender is, e.g., stuck in an X-Plane dialog then that sender
        //  will not send position updates, so stopping is fairly close to reality.)
        if (dNow - dHist <= std::chrono::seconds(1)) {
            try {
                const float f = float(dNow.count()) / float(dHist.count());
                drawInfo.x       = histPos[0].x       + f * (histPos[1].x       - histPos[0].x);
                drawInfo.y       = histPos[0].y       + f * (histPos[1].y       - histPos[0].y);
                drawInfo.z       = histPos[0].z       + f * (histPos[1].z       - histPos[0].z);
                drawInfo.pitch   = histPos[0].pitch   + f * (histPos[1].pitch   - histPos[0].pitch);
                drawInfo.heading = std::fmod(histPos[0].heading + f * (histPos[1].heading - histPos[0].heading), 360.0f);
                drawInfo.roll    = histPos[0].roll    + f * (histPos[1].roll    - histPos[0].roll);

/*                    LOG_MSG(logDEBUG, "0x%06X: %.2f / %.2f / %.2f  ==> %+8.2f / %+8.2f / %+8.2f  [%.4f]",
                            GetModeS_ID(), drawInfo.x, drawInfo.y, drawInfo.z,
                            drawInfo.x-prevDraw.x,
                            drawInfo.y-prevDraw.y,
                            drawInfo.z-prevDraw.z, f); */
            }
            catch (...) {
                drawInfo = histPos[1];
            }
        } else {
            // This plane has no updates...but the sender is updated?
            // Then we lost contact to this plane and shall remove it
            if (nowFlightLoop - sender.lastMsg < std::chrono::milliseconds(500)) {
                LOG_MSG(logWARN, WARN_LOST_PLANE_CONTACT, senderId,
                        (int)sizeof(sender.settings.name), sender.settings.name,
                        sender.settings.pluginId, sender.sFrom.c_str());
                SetInvalid();
            }
        }
    }
    
    // Special handling for rotational dataRefs:
    // These are the only ones we move ourselves as they need to keep going every frame
    // for good looks.
    // Idea: Turn them based on defined speed
    if (GetTireRotRpm() > 1.0f) SetTireRotAngle(std::fmod(GetTireRotAngle() + GetTireRotRpm()/60.0f * _elapsed * 360.0f, 360.0f));
    if (GetEngineRotRpm() > 1.0f) SetEngineRotAngle(std::fmod(GetEngineRotAngle() + GetEngineRotRpm()/60.0f * _elapsed * 360.0f, 360.0f));
    for (size_t i = 1; i <= 4; i++)
        if (GetEngineRotRpm(i) > 1.0f) SetEngineRotAngle(i, std::fmod(GetEngineRotAngle(i) + GetEngineRotRpm(i)/60.0f * _elapsed * 360.0f, 360.0f));
    if (GetPropRotRpm() > 1.0f) SetPropRotAngle(std::fmod(GetPropRotAngle() + GetPropRotRpm()/60.0f * _elapsed * 360.0f, 360.0f));
}

//
// MARK: Sender Administration
//

// uses memcmp to compare sender addresses
bool SenderAddrTy::operator< (const SenderAddrTy& o) const
{
    return memcmp(this, &o, sizeof(*this)) < 0;
}

// Constructor copies values and sets lastMsg to now
SenderTy::SenderTy (const std::string& _from, const XPMP2::RemoteMsgSettingsTy& _s) :
sFrom(_from), bLocal(_s.bLocalSender),
settings(_s), lastMsg(std::chrono::steady_clock::now())
{
    // Remove port number from IP adress
    const size_t posColon = sFrom.rfind(':');
    if (posColon != std::string::npos)
        sFrom.erase(posColon);
}

// Find a sender based on plugin id and IP address
SenderTy* SenderTy::Find (XPLMPluginID _id, const std::uint32_t _from[4])
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
std::atomic_flag gbSkipAcMaintenance = ATOMIC_FLAG_INIT;

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
void ClientProcSettings (const std::uint32_t from[4],
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
                (_msgSettings.bLocalSender ? sFrom + " (local)" : sFrom).c_str());
        rcGlob.gmapSender.emplace(std::piecewise_construct,
                                  std::forward_as_tuple(_msgSettings.pluginId, from),
                                  std::forward_as_tuple(sFrom, _msgSettings));
        
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

    // Acquire the data access lock once and keep it while the flight loop is running
    if (!glockDataMain)
        glockDataMain.lock();
    
    // Every 10 seconds clean up outdated senders
    static float lastSenderCleanup = 0;
    if (GetMiscNetwTime() - lastSenderCleanup > 10.0f) {
        lastSenderCleanup = GetMiscNetwTime();
        for (mapSenderTy::iterator sIter = rcGlob.gmapSender.begin();
             sIter != rcGlob.gmapSender.end();)
        {
            const SenderTy& sdr = sIter->second;
            if (nowFlightLoop - sdr.lastMsg > std::chrono::seconds(2 * XPMP2::REMOTE_SEND_SETTINGS_INTVL)) {
                LOG_MSG(logINFO, INFO_SENDER_PLUGIN_LOST,
                        (int)sizeof(sdr.settings.name), sdr.settings.name,
                        sdr.settings.pluginId, sdr.sFrom.c_str());
                sIter = rcGlob.gmapSender.erase(sIter);
            }
            else
                sIter++;
        }
    }

    // If needed create new or remove deleted aircraft that have been prepared in the meantime
    if (gbSkipAcMaintenance.test_and_set()) {
        for (auto& s: rcGlob.gmapSender) {              // loop all senders
            // loop all a/c of that sender
            mapRemoteAcTy::iterator acIter = s.second.mapAc.begin();
            while (acIter != s.second.mapAc.end()) {
                // a/c to be deleted?
                if (acIter->second.IsToBeDeleted())
                    acIter = s.second.mapAc.erase(acIter);
                // create a/c if not created
                else if (acIter->second.GetModeS_ID() == 0)
                    (acIter++)->second.Create();
                else
                    acIter++;
            }
        }
    }
}

// Called at the end of each flight loop processing: Release the data lock
void ClientFlightLoopEnds () noexcept
{
    // Release the data access lock
    try { glockDataMain.unlock(); }
    catch(...) { /* ignore all errors */ }
}

/// @brief Handle A/C Details messages, called by XPMP2 via callback
/// @details 1. If the aircraft does not exist create it
///          2. Else update it's data
void ClientProcAcDetails (const std::uint32_t _from[4], size_t _msgLen,
                          const XPMP2::RemoteMsgAcDetailTy& _msgAcDetails)
{
    // Find the sender, bail if we don't know it
    SenderTy* pSender = SenderTy::Find(_msgAcDetails.pluginId, _from);
    if (!pSender) return;
    pSender->lastMsg  = std::chrono::steady_clock::now();

    // Loop over all aircraft contained in the message
    const size_t numAc = _msgAcDetails.NumElem(_msgLen);
    for (size_t i = 0; i < numAc; ++i) {
        const XPMP2::RemoteAcDetailTy& acDetails = _msgAcDetails.arr[i];
        // Is the aircraft known?
        mapRemoteAcTy::iterator iAc = pSender->mapAc.find(acDetails.modeS_id);
        // Now require access (for each plane, because we want to give the main thread's flight loop a better change to grab the lock if needed)
        std::unique_lock<std::mutex> lk(gmutexData);
        // Is the aircraft known?
        if (iAc == pSender->mapAc.end()) {
            // new aircraft, create an object for it, but not yet the actual plane (as this is the network thread)
            pSender->mapAc.emplace(std::piecewise_construct,
                                   std::forward_as_tuple(acDetails.modeS_id),
                                   std::forward_as_tuple(*pSender, acDetails));
            // tell the main thread that it shall process new a/c
            gbSkipAcMaintenance.clear();
        } else {
            // known aircraft, update its data
            iAc->second.Update(acDetails);
        }
        lk.unlock();
    }
}

/// Handle A/C Position Update message, called by XPMP2 via callback
void ClientProcAcPosUpdate (const std::uint32_t _from[4], size_t _msgLen,
                            const XPMP2::RemoteMsgAcPosUpdateTy& _msgAcPosUpdate)
{
    // Find the sender, bail if we don't know it
    SenderTy* pSender = SenderTy::Find(_msgAcPosUpdate.pluginId, _from);
    if (!pSender) return;
    pSender->lastMsg  = std::chrono::steady_clock::now();

    // Loop over all aircraft contained in the message
    const size_t numAc = _msgAcPosUpdate.NumElem(_msgLen);
    for (size_t i = 0; i < numAc; ++i) {
        const XPMP2::RemoteAcPosUpdateTy& acPosUpd = _msgAcPosUpdate.arr[i];
        // Is the aircraft known?
        mapRemoteAcTy::iterator iAc = pSender->mapAc.find(XPMPPlaneID(acPosUpd.modeS_id));
        if (iAc != pSender->mapAc.end()) {
            // Now require access (for each plane, because we want to give the main thread's flight loop a better change to grab the lock if needed)
            std::lock_guard<std::mutex> lk(gmutexData);
            // known aircraft, update its data
            iAc->second.Update(acPosUpd);
        }
    }

}

/// Handle A/C Animation dataRef messages, called by XPMP2 via callback
void ClientProcAcAnim (const std::uint32_t _from[4], size_t _msgLen,
                       const XPMP2::RemoteMsgAcAnimTy& _msgAcAnim)
{
    // Find the sender, bail if we don't know it
    SenderTy* pSender = SenderTy::Find(_msgAcAnim.pluginId, _from);
    if (!pSender) return;
    pSender->lastMsg  = std::chrono::steady_clock::now();

    // Loop all animation data elements in the message
    for (const XPMP2::RemoteAcAnimTy* pAnim = _msgAcAnim.next(_msgLen);
         pAnim;
         pAnim = _msgAcAnim.next(_msgLen, pAnim))
    {
        // Is the aircraft known?
        mapRemoteAcTy::iterator iAc = pSender->mapAc.find(XPMPPlaneID(pAnim->modeS_id));
        if (iAc != pSender->mapAc.end()) {
            // Now require access (for each plane, because we want to give the main thread's flight loop a better change to grab the lock if needed)
            std::lock_guard<std::mutex> lk(gmutexData);
            // known aircraft, update its data
            iAc->second.Update(*pAnim);
        }
    }
}

/// Handle A/C Removal message, called by XPMP2 via callback
void ClientProcAcRemove (const std::uint32_t _from[4], size_t _msgLen,
                         const XPMP2::RemoteMsgAcRemoveTy& _msgAcRemove)
{
    // Find the sender, bail if we don't know it
    SenderTy* pSender = SenderTy::Find(_msgAcRemove.pluginId, _from);
    if (!pSender) return;
    pSender->lastMsg  = std::chrono::steady_clock::now();

    // Loop over all aircraft contained in the message
    const size_t numAc = _msgAcRemove.NumElem(_msgLen);
    for (size_t i = 0; i < numAc; ++i) {
        const XPMP2::RemoteAcRemoveTy& acRemove = _msgAcRemove.arr[i];
        // Is the aircraft known?
        mapRemoteAcTy::iterator iAc = pSender->mapAc.find(XPMPPlaneID(acRemove.modeS_id));
        if (iAc != pSender->mapAc.end()) {
            // Now require access (for each plane, because we want to give the main thread's flight loop a better change to grab the lock if needed)
            std::lock_guard<std::mutex> lk(gmutexData);
            // mark this a/c for deletion (which must happen in XP's main thread)
            iAc->second.MarkForDeletion();
            // tell the main thread that it shall process removed a/c
            gbSkipAcMaintenance.clear();
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
    gbSkipAcMaintenance.test_and_set();
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
        // Try already to get TCAS control so we have it before others can snatch it away.
        // This is not in accordance with what is laid out in "TCAS Override"
        // https://developer.x-plane.com/article/overriding-tcas-and-providing-traffic-information/#Plugin_coordination
        // but we serve a good deed here: We can combine several plugins' TCAS needs,
        // but only if we are in control:
        ClientTryGetAI();

        // Start the listener to receive message
        XPMP2::RemoteCBFctTy rmtCBFcts = {
            ClientFlightLoopBegins,         // before flight loop processing starts
            ClientFlightLoopEnds,           // after flight loop processing ends
            ClientProcSettings,             // Settings
            ClientProcAcDetails,            // Aircraft Details
            ClientProcAcPosUpdate,          // Aircraft Position Update
            ClientProcAcAnim,               // Aircraft Animarion dataRef values
            ClientProcAcRemove,             // Aircraft Removal
        };
        XPMP2::RemoteRecvStart(rmtCBFcts);
    }
    // Need to stop?
    else if (nForce < 0 ||
             XPMP2::RemoteGetStatus() != XPMP2::REMOTE_OFF)
    {
        // Release the data access lock, just a safety measure so that we don't hinder shutdown in case we f--- up
        try { glockDataMain.unlock(); }
        catch (...) { /* ignore all errors */ }
        // Stop the listener
        XPMP2::RemoteRecvStop();
        // Shut down everything
        ClientReleaseAI();
        rcGlob.gmapSender.clear();      // removes aircraft in turn
        LOG_MSG(logINFO, INFO_DEACTIVATED);
    }
}
