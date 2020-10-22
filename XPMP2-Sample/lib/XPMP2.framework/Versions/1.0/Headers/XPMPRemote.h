/// @file       XPMPRemote.h
/// @brief      Semi-public remote network functionality for master/client operations
/// @details    Technically, this is public functionality from a library
///             point of view. But it is intended for `XPMPRemoteClient` only,
///             not for a standard plugin.
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

#ifndef _XPMPRemote_h_
#define _XPMPRemote_h_

#include <cstdint>

namespace XPMP2 {

//
// MARK: Network Data Definitions
//

/// Message type
enum RemoteMsgTy : std::uint8_t {
    RMT_MSG_INTEREST_BEACON = 0,    ///< beacon sent by a remote client to signal interest in data
    RMT_MSG_SETTINGS,               ///< a sender's id and its settings
    RMT_MSG_AC_DETAILED,            ///< aircraft full details, needed to create new a/c objects and to re-synch all remote data
    RMT_MSG_AC_DIFF,                ///< aircraft differences only
    RMT_MSG_AC_ANIM,                ///< aircraft animation values (dataRef values) only
};

/// Message header, identical for all message types
struct RemoteMsgHeaderTy {
    RemoteMsgTy  msgTy  : 4;        ///< message type
    std::uint8_t msgVer : 4;        ///< message version
    std::uint8_t filler    = 0;     ///< yet unsed
    std::uint16_t pluginId = 0;     ///< lower 16 bit of the sending plugin's id
};

/// Interest Beacon message version number
constexpr std::uint8_t RMT_VER_BEACON = 0;

/// Setttings message version number
constexpr std::uint8_t RMT_VER_SETTINGS = 0;
/// Settings message, identifying a sending plugin, regularly providing its settings
struct RemoteMsgSettingsTy {
    RemoteMsgHeaderTy hdr;          ///< message header
    char            name[12];       ///< plugin's name, not necessarily zero-terminated if using full 12 chars
    char            defaultIcao[4]; ///< Default ICAO aircraft type designator if no match can be found
    char            carIcaoType[4]; ///< Ground vehicle type identifier
    std::uint8_t logLvl             :3; ///< logging level
    bool bLogMdlMatch               :1; ///< Debug model matching?
    bool bObjReplDataRefs           :1; ///< Replace dataRefs in `.obj` files on load?
    bool bObjReplTextures           :1; ///< Replace textures in `.obj` files on load if needed?
    bool bLabelCutOffAtVisibility   :1; ///< Cut off labels at XP's reported visibility mit?
    bool bMapEnabled                :1; ///< Do we feed X-Plane's maps with our aircraft positions?
    bool bMapLabels                 :1; ///< Do we show labels with the aircraft icons?
    std::uint32_t filler           :23; ///< fill up to 4 byte
    float           maxLabelDist;   ///< Maximum distance for drawing labels? [m]
};

/// Function prototypes for callback functions to handle the received messages
struct RemoteCBFctTy {
    void (*pfMsgSettings) (const RemoteMsgSettingsTy&) = nullptr;       ///< processes settings messages
};

//
// MARK: Miscellaneous
//

/// State of remote communcations
enum RemoteStatusTy : unsigned {
    REMOTE_OFF = 0,                 ///< no remote connectivtiy, not listening, not sending
    REMOTE_SEND_WAITING,            ///< listening for a request to send data, but not actively sending data
    REMOTE_SENDING,                 ///< actively sending aircraft data out to the network
    REMOTE_RECV_WAITING,            ///< waiting to receive data, periodically sending a token of interest
    REMOTE_RECEIVING,               ///< actively receiving data
};

/// Returns the current Remote status
RemoteStatusTy RemoteGetStatus();

/// Starts the listener, will call provided callback functions with received messages
void RemoteRecvStart (const RemoteCBFctTy& _rmtCBFcts);

/// Stops the receiver
void RemoteRecvStop ();

}


#endif
