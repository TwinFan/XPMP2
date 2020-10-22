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

//
// MARK: Process Messages
//

void ClientProcSettings (const XPMP2::RemoteMsgSettingsTy& _msgSettings)
{
    // TODO: The sender's IP address is yet missing
    LOG_MSG(logDEBUG, "Received settings from %.*s (%u)",
            (int)sizeof(_msgSettings.name), _msgSettings.name, _msgSettings.hdr.pluginId);
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
    }
}
