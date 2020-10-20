/// @file       Remote.h
/// @brief      Master/Client communication for aircraft synchronization
///             on remote networked instances
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

#ifndef _Remote_h_
#define _Remote_h_

namespace XPMP2 {

/// Configuration for remote communications support
enum RemoteCfgTy : int {
    REMOTE_CFG_OFF = -1,            ///< config: force off
    REMOTE_CFG_CONDITIONALLY = 0,   ///< config: on if in a netwoked/multiplayer setup
    REMOTE_CFG_ON = 1,              ///< config: force on
};

/// State of remote communcations
enum RemoteStatusTy : unsigned {
    REMOTE_OFF = 0,                 ///< no remote connectivtiy, not listening, not sending
    REMOTE_WAITING,                 ///< listening for a request to send data, but not actively sending data
    REMOTE_SENDING,                 ///< actively sending aircraft data out to the network
};

/// Initialize the module
void RemoteInit ();

/// Grace cleanup, stops all threads
void RemoteCleanup ();

/// Compares current vs. expected status and takes appropriate action
void RemoteUpdateStatus ();

/// Regularly called from the flight loop callback
void RemoteUpdateAc ();

}
#endif
