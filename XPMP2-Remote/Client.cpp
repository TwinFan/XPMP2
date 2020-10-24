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

#define INFO_NEW_SENDER_PLUGIN  "First data received from %.*s @ %s"
#define INFO_GOT_AI_CONTROL     "Have TCAS / AI control now"
#define INFO_DEACTIVATED        "Deactivated, stopped listening to network"

//
// MARK: Aircraft Administration
//

//
// MARK: Sender Administration
//

// TODO: Identify stale connections and drop them

bool SenderAddrTy::operator< (const SenderAddrTy& o) const
{
    return memcmp(this, &o, sizeof(*this)) < 0;
}

/// Constructor copies values and sets lastMsg to now
SenderTy::SenderTy (const std::string& _from, const XPMP2::RemoteMsgSettingsTy& _s) :
sFrom(_from), settings(_s), lastMsg(std::chrono::steady_clock::now())
{}

//
// MARK: Process Messages
//

void ClientTryGetAI ();
bool bWaitingForAI = false;

/// Callback for requesting TCAS: Maybe now we have a chance?
void ClientCBRetryGetAI (void*)
{
    bWaitingForAI = false;
    
    // If we still want to have TCAS we just try again
    if (rcGlob.mergedS.bHaveTCASControl && !XPMPHasControlOfAIAircraft())
        ClientTryGetAI();
}

/// Try getting TCAS/AI control
void ClientTryGetAI ()
{
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

/// Stop TCAS/AI control
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
    LOG_MSG(logDEBUG, "Received settings from %.*s (%u) @ %s",
            (int)sizeof(_msgSettings.name), _msgSettings.name,
            _msgSettings.hdr.pluginId, sFrom.c_str());
    
    // 1. Store sender information
    SenderAddrTy senderId (_msgSettings.hdr.pluginId, from);
    mapSenderTy::iterator itSender = rcGlob.gmapSender.find(senderId);
    if (itSender == rcGlob.gmapSender.end()) {
        // Seeing this plugin the first time!
        LOG_MSG(logINFO, INFO_NEW_SENDER_PLUGIN,
                (int)sizeof(_msgSettings.name), _msgSettings.name,
                sFrom.c_str());
        auto p = rcGlob.gmapSender.emplace(senderId, SenderTy(sFrom, _msgSettings));
        itSender = p.first;
        
        // If this one also happens to be the first plugin at all then it defines
        // some settings that cannot be well merged
        if (rcGlob.gmapSender.size() == 1) {
            // packed config structure does not have zero-termination
            std::string defAc  (_msgSettings.defaultIcao, sizeof(_msgSettings.defaultIcao));
            std::string defCar (_msgSettings.carIcaoType, sizeof(_msgSettings.carIcaoType));
            XPMPSetDefaultPlaneICAO(defAc.c_str(), defCar.c_str());
        }
    }
    else
    {
        // plugin is known, update its settings
        itSender->second.settings = _msgSettings;
        itSender->second.lastMsg  = std::chrono::steady_clock::now();
    }
    
    // 2. Consolidate/merge the settings of all plugins
    itSender = rcGlob.gmapSender.begin();
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
    XPMPEnableMap(rcGlob.mergedS.bMapEnabled,
                  rcGlob.mergedS.bMapLabels);
    if (rcGlob.mergedS.bHaveTCASControl)
        ClientTryGetAI();
    else
        ClientReleaseAI();
}

//
// MARK: Global Functions
//

// Toggles the cient's activitiy status, as based on menu command
void ClientToggleActive ()
{
    if (XPMP2::RemoteGetStatus() == XPMP2::REMOTE_OFF)
    {
        // Start the listener to receive message
        XPMP2::RemoteCBFctTy rmtCBFcts;
        rmtCBFcts.pfMsgSettings = ClientProcSettings;
        XPMP2::RemoteRecvStart(rmtCBFcts);
    }
    else
    {
        // Stop the listener
        XPMP2::RemoteRecvStop();
        // Shut down everything
        ClientReleaseAI();
        rcGlob.gmapSender.clear();      // removes aircraft in turn
        LOG_MSG(logINFO, INFO_DEACTIVATED);
    }
}
