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

/// Representation of a remote aircraft
class RemoteAC : public XPMP2::Aircraft {
protected:
    /// We keep 2 historic positions to be able to calculate simple linear extrapolation
    XPLMDrawInfo_t histPos[2];
    
public:
    /// Constructor takes same arguments as XPMP2::Aircraft::Aircraft()
    RemoteAC (const std::string& _icaoType,
              const std::string& _icaoAirline,
              const std::string& _livery,
              XPMPPlaneID _modeS_id = 0,
              const std::string& _modelId = "");
    
    /// Called by XPMP2 for position updates, extrapolates from historic positions
    void UpdatePosition (float, int) override;
};

/// Map of remote aircraft; key is the plane id as sent by the sending plugin (while the _modeS_id of the local copy could differ)
typedef std::map<XPMPPlaneID,RemoteAC> mapRemoteAcTy;

//
// MARK: Sender Administration
//

/// Uniquely identifies a sending plugin
struct SenderAddrTy {
    XPLMPluginID pluginId;              ///< sender's plugin id within X-Plane (first to speed up memcmp as usually this will already differ between plugins)
    std::uint32_t from[4];              ///< numerical part of sender's IP address (only differentiating in rare cases of two plugins on different PCs having the same plugin id)
    bool operator< (const SenderAddrTy& o) const;   ///< uses memcmp to compare this
    /// Constructor just fills values
    SenderAddrTy (XPLMPluginID _id, std::uint32_t _from[4]) :
    pluginId(_id) { memcpy(from, _from, sizeof(from)); }
};

struct SenderTy {
    std::string sFrom;                  ///< string representaton of the sender's IP address
    XPMP2::RemoteMsgSettingsTy settings;///< that plugin's settings
    std::chrono::time_point<std::chrono::steady_clock> lastMsg;
    mapRemoteAcTy mapAc;                ///< map of aircraft sent by this plugin
    
    /// Constructor copies values and sets lastMsg to now
    SenderTy (const std::string& _from, const XPMP2::RemoteMsgSettingsTy& _s);
};

/// smart pointer to such a sender information
typedef std::unique_ptr<RemoteAC> RempteACPtr;

typedef std::map<SenderAddrTy,SenderTy> mapSenderTy;

//
// MARK: Global Functions
//

/// Toggles the cient's activitiy status, as based on menu command
void ClientToggleActive ();
