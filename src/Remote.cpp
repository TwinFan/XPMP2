/// @file       Remote.cpp
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

#include "XPMP2.h"

#if APL == 1 || LIN == 1
#include <unistd.h>                     // for pipe
#include <sys/fcntl.h>                  // for F_SETFL, O_NONBLOCK
#endif

namespace XPMP2 {

#define ERR_MC_THREAD       "Exception in multicast handling: %s"
#define ERR_MC_MAX          "Too many errors, I give up on remote functionality!"
constexpr int MC_MAX_ERR=5;             ///< after this many errors we no longer try listening

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
std::thread gThrMC;                     ///< remote listening/sending thread
#pragma clang diagnostic pop
UDPMulticast* gpMc = nullptr;           ///< multicast socket for listening/sending (destructor uses locks, which don't work during modul shutdown, so can't create a global object due to its exit-time destructor)
volatile bool gbStopMCThread = false;   ///< Shall the listener thread stop?
int gCntMCErr = 0;                      ///< error counter for listener thread

#if APL == 1 || LIN == 1
/// the self-pipe to shut down the APRS thread gracefully
SOCKET gSelfPipe[2] = { INVALID_SOCKET, INVALID_SOCKET };
#endif

//
// MARK: Helper Functions
//

/// Extracts the numerical address and puts it into addr for generic address use
void SockaddrToArr (const sockaddr* sa, std::uint32_t addr[4])
{
    switch (sa->sa_family) {
        case AF_INET:
            addr[0] = ((sockaddr_in*)sa)->sin_addr.s_addr;
            addr[1] = addr[2] = addr[3] = 0;
            break;
            
        case AF_INET6:
            memcpy(addr, ((sockaddr_in6*)sa)->sin6_addr.s6_addr, sizeof(addr[4]));
            break;
            
        default:
            break;
    }
}

//
// MARK: SENDING Remote Data
//

#define DEBUG_MC_SEND_WAIT  "Listening to %s:%d, waiting for someone interested in our data..."
#define INFO_MC_SEND_WAIT   "Listening on the network, waiting for someone interested in our data..."
#define INFO_MC_SEND_RCVD   "Received word from %s, will start sending aircraft data"

/// Timestamp when we sent our settings the last time
float gSendSettingsLast = 0.0f;
/// How often to send settings?
constexpr float REMOTE_SEND_SETTINGS_INTVL = 20.0f;

/// Fill the message header
void RmtFillMsgHeader (RemoteMsgHeaderTy& hdr, RemoteMsgTy _ty, std::uint8_t _ver)
{
    hdr.msgTy       = _ty;
    hdr.msgVer      = _ver;
    hdr.filler      = 0;
    hdr.pluginId    = std::uint8_t(glob.pluginId & 0xFFFF);
}

/// Send our settings
void RmtSendSettings ()
{
    RemoteMsgSettingsTy s;
    // Message header
    RmtFillMsgHeader (s.hdr, RMT_MSG_SETTINGS, RMT_VER_SETTINGS);
    // copy all strings
    strncpy(s.name, glob.pluginName.c_str(), sizeof(s.name));
    strncpy(s.defaultIcao, glob.defaultICAO.c_str(), sizeof(s.defaultIcao));
    strncpy(s.carIcaoType, glob.carIcaoType.c_str(), sizeof(s.carIcaoType));
    // copy further fields
    s.maxLabelDist              = glob.maxLabelDist;
    s.logLvl                    = (std::uint8_t)glob.logLvl;
    s.bLogMdlMatch              = glob.bLogMdlMatch;
    s.bObjReplDataRefs          = glob.bObjReplDataRefs;
    s.bObjReplTextures          = glob.bObjReplTextures;
    s.bLabelCutOffAtVisibility  = glob.bLabelCutOffAtVisibility;
    s.bMapEnabled               = glob.bMapEnabled;
    s.bMapLabels                = glob.bMapLabels;
    s.bHaveTCASControl          = XPMPHasControlOfAIAircraft();
    // send the data out
    assert(gpMc);
    if (gpMc->SendMC(&s, sizeof(s)) != sizeof(s))
        throw NetRuntimeError("Could not send Settings multicast");
}

/// Sending function, ie. we are actively sending data out
void RmtSendLoop ()
{
    while (!gbStopMCThread && glob.remoteStatus == REMOTE_SENDING && gpMc->isOpen())
    {
        // TODO: Implement a proper wait mechanism via condition variable
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Every once in a while send out or configuration
        if (CheckEverySoOften(gSendSettingsLast, REMOTE_SEND_SETTINGS_INTVL))
            RmtSendSettings();
    }
}

/// Thread main function for the sender
void RmtSendMain()
{
    // This is a thread main function, set thread's name
    SET_THREAD_NAME("XPMP2_Send");
    
    try {
        LOG_ASSERT(gpMc != nullptr);
        
        // Set global status to: we are "waiting" for some action on the multicast group
        glob.remoteStatus = REMOTE_SEND_WAITING;

        // Create a multicast socket and listen if there is any traffic in our multicast group
        gpMc->Join(glob.remoteMCGroup, glob.remotePort, glob.remoteTTL,glob.remoteBufSize);
        int maxSock = (int)gpMc->getSocket() + 1;
#if APL == 1 || LIN == 1
        // the self-pipe to shut down the TCP socket gracefully
        if (pipe(gSelfPipe) < 0)
            throw NetRuntimeError("Couldn't create self-pipe");
        fcntl(gSelfPipe[0], F_SETFL, O_NONBLOCK);
        maxSock = std::max(maxSock, gSelfPipe[0]+1);
#endif
        
        // *** Main listening loop ***
        
        if (glob.logLvl == logDEBUG)
            LOG_MSG(logDEBUG, DEBUG_MC_SEND_WAIT, glob.remoteMCGroup.c_str(), glob.remotePort)
        else
            LOG_MSG(logINFO, INFO_MC_SEND_WAIT)
        
        while (!gbStopMCThread && glob.remoteStatus == REMOTE_SEND_WAITING && gpMc->isOpen())
        {
            // wait for some signal on either socket (multicast or self-pipe)
            fd_set sRead;
            FD_ZERO(&sRead);
            FD_SET(gpMc->getSocket(), &sRead);      // check our socket
#if APL == 1 || LIN == 1
            FD_SET(gSelfPipe[0], &sRead);        // check the self-pipe
#endif
            int retval = select(maxSock, &sRead, NULL, NULL, NULL);

            // short-cut if we are to shut down (return from 'select' due to closed socket)
            if (gbStopMCThread)
                break;

            // select call failed???
            if (retval == -1)
                throw NetRuntimeError("'select' failed");

            // select successful - there is multicast data!
            if (retval > 0 && FD_ISSET(gpMc->getSocket(), &sRead))
            {
                // We aren't actually interested in the data as such,
                // the fact that there was _any_ traffic already means:
                // there is someone out there interested in our data.
                // We just read received multicast to clear out the buffers.
                std::string from;
                gpMc->RecvMC(&from);
                LOG_MSG(logINFO, INFO_MC_SEND_RCVD, from.c_str());

                // Set global status to: we are about to send data, also exits listening loop
                glob.remoteStatus = REMOTE_SENDING;
            }
        }
        
        // Left the listening loop because we are to send data?
        if (!gbStopMCThread && glob.remoteStatus == REMOTE_SENDING && gpMc->isOpen())
            RmtSendLoop();
    }
    catch (std::runtime_error& e) {
        ++gCntMCErr;
        LOG_MSG(logERR, ERR_MC_THREAD, e.what());
    }
    
    // close the multicast socket
    gpMc->Close();

#if APL == 1 || LIN == 1
    // close the self-pipe sockets
    for (SOCKET &s: gSelfPipe) {
        if (s != INVALID_SOCKET) close(s);
        s = INVALID_SOCKET;
    }
#endif
    
    // Error count too high?
    if (gCntMCErr >= MC_MAX_ERR) {
        LOG_MSG(logFATAL, ERR_MC_MAX);
    }
    
    // make sure the end of the thread is recognized and joined
    glob.remoteStatus = REMOTE_OFF;
    gbStopMCThread = true;
}

//
// MARK: RECEIVING Remote Data
//

#define DEBUG_MC_RECV_BEGIN "Receiver started listening to %s:%d"
#define INFO_MC_RECV_BEGIN  "Receiver started listening on the network..."
#define INFO_MC_RECV_RCVD   "Receiver received data from %.*s on %s, will start message processing"

/// [s] How often to send an Interest Beacon?
constexpr int REMOTE_RECV_BEACON_INTVL = 15;

/// The callback function pointers the remote client plugin provided
RemoteCBFctTy gRmtCBFcts;

/// Sends an Interest Beacon
void RmtSendBeacon()
{
    RemoteMsgHeaderTy hdr;
    RmtFillMsgHeader(hdr, RMT_MSG_INTEREST_BEACON, RMT_VER_BEACON);
    if (gpMc->SendMC(&hdr, sizeof(hdr)) != sizeof(hdr))
        throw NetRuntimeError("Could not send Interest Beacon multicast");
}

/// Thread main function for the receiver
void RmtRecvMain()
{
    // This is a thread main function, set thread's name
    SET_THREAD_NAME((glob.logAcronym + "_Recv").c_str());
    
    try {
        LOG_ASSERT(gpMc != nullptr);
        
        // Set global status to: we are "waiting" for data, but will send the interest beacon every once in a while
        glob.remoteStatus = REMOTE_RECV_WAITING;

        // Create a multicast socket
        gpMc->Join(glob.remoteMCGroup, glob.remotePort, glob.remoteTTL,glob.remoteBufSize);
        int maxSock = (int)gpMc->getSocket() + 1;
#if APL == 1 || LIN == 1
        // the self-pipe to shut down the TCP socket gracefully
        if (pipe(gSelfPipe) < 0)
            throw NetRuntimeError("Couldn't create self-pipe");
        fcntl(gSelfPipe[0], F_SETFL, O_NONBLOCK);
        maxSock = std::max(maxSock, gSelfPipe[0]+1);
#endif
        
        // Send out a first Interest Beacon
        RmtSendBeacon();
        
        // *** Main listening loop ***
        
        if (glob.logLvl == logDEBUG)
            LOG_MSG(logDEBUG, DEBUG_MC_RECV_BEGIN, glob.remoteMCGroup.c_str(), glob.remotePort)
        else
            LOG_MSG(logINFO, INFO_MC_RECV_BEGIN)
        
        // Timeout is 60s, ie. we listen for 60 seconds, then send a beacon, then listen again
        struct timeval timeout = { REMOTE_RECV_BEACON_INTVL, 0 };
        while (!gbStopMCThread && glob.RemoteIsListener() && gpMc->isOpen())
        {
            // wait for some data on either socket (multicast or self-pipe)
            fd_set sRead;
            FD_ZERO(&sRead);
            FD_SET(gpMc->getSocket(), &sRead);      // check our socket
#if APL == 1 || LIN == 1
            FD_SET(gSelfPipe[0], &sRead);        // check the self-pipe
#endif
            int retval = select(maxSock, &sRead, NULL, NULL, &timeout);

            // short-cut if we are to shut down (return from 'select' due to closed socket)
            if (gbStopMCThread)
                break;

            // select call failed???
            if (retval == -1)
                throw NetRuntimeError("'select' failed");
            
            // Timeout? Then send (another) interest beacon...maybe sometimes someone responds
            else if (retval == 0)
                RmtSendBeacon();

            // select successful - there is multicast data!
            else if (retval > 0 && FD_ISSET(gpMc->getSocket(), &sRead))
            {
                // Receive the data (if we are still waiting then we're interested in the sender's address purely for logging purposes)
                sockaddr saFrom;
                std::string sFrom;
                const size_t recvSize = gpMc->RecvMC(glob.remoteStatus == REMOTE_RECV_WAITING ? &sFrom : nullptr,
                                                     &saFrom);
                if (recvSize >= sizeof(RemoteMsgHeaderTy))
                {
                    std::uint32_t from[4];                  // extract the numerical address
                    SockaddrToArr(&saFrom, from);
                    const RemoteMsgHeaderTy& hdr = *(RemoteMsgHeaderTy*)gpMc->getBuf();
                    switch (hdr.msgTy) {
                        // just ignore any interest beacons
                        case RMT_MSG_INTEREST_BEACON:
                            break;
                            
                        // Settings
                        case RMT_MSG_SETTINGS:
                            if (hdr.msgVer == RMT_VER_SETTINGS && recvSize == sizeof(RemoteMsgSettingsTy))
                            {
                                const RemoteMsgSettingsTy& s = *(RemoteMsgSettingsTy*)gpMc->getBuf();
                                // Is this the first set of settings we received? Then we switch status!
                                if (glob.remoteStatus == REMOTE_RECV_WAITING) {
                                    glob.remoteStatus = REMOTE_RECEIVING;
                                    LOG_MSG(logINFO, INFO_MC_RECV_RCVD,
                                            (int)sizeof(s.name), s.name,
                                            sFrom.c_str());
                                }
                                // Let the plugin process this message
                                if (gRmtCBFcts.pfMsgSettings)
                                    gRmtCBFcts.pfMsgSettings(from, sFrom, s);
                            } else {
                                LOG_MSG(logWARN, "Cannot process Settings message: %lu bytes, version %u", recvSize, hdr.msgVer);
                            }
                    }
                    
                } else {
                    LOG_MSG(logWARN, "Received too small message with just %lu bytes", recvSize);
                }
            }
        }
    }
    catch (std::runtime_error& e) {
        ++gCntMCErr;
        LOG_MSG(logERR, ERR_MC_THREAD, e.what());
    }
    
    // close the multicast socket
    gpMc->Close();

#if APL == 1 || LIN == 1
    // close the self-pipe sockets
    for (SOCKET &s: gSelfPipe) {
        if (s != INVALID_SOCKET) close(s);
        s = INVALID_SOCKET;
    }
#endif
    
    // Error count too high?
    if (gCntMCErr >= MC_MAX_ERR) {
        LOG_MSG(logFATAL, ERR_MC_MAX);
    }
    
    // make sure the end of the thread is recognized and joined
    glob.remoteStatus = REMOTE_OFF;
    gbStopMCThread = true;
}

//
// MARK: Internal functions
//

/// Start the background thread to listen to multicast, to see if anybody is interested in our data
void RmtStartMCThread(bool bSender)
{
    // Can only do that if currently off
    if (glob.remoteStatus != REMOTE_OFF)
        return;
    
    // Is or was there a thread running?
    if (gThrMC.joinable()) {
        if (!gbStopMCThread)            // it still is running
            return;                     // keep it running
        gThrMC.join();                  // wait for the current thread to exit (which it should because bStopMCListen is true)
        gThrMC = std::thread();
    }
    
    // seen too many errors already?
    if (gCntMCErr >= MC_MAX_ERR)
        return;
    
    // Start the thread
    gbStopMCThread = false;
    gThrMC = std::thread(bSender ? RmtSendMain : RmtRecvMain);
}

/// Stop all threads and communication with the network
void RmtStopAll()
{
    // Listener thread
    if (gThrMC.joinable()) {
        gbStopMCThread = true;
#if APL == 1 || LIN == 1
        // Mac/Lin: Try writing something to the self-pipe to stop gracefully
        if (gSelfPipe[1] == INVALID_SOCKET ||
            write(gSelfPipe[1], "STOP", 4) < 0)
            // if the self-pipe didn't work:
#endif
            if (gpMc)
                gpMc->Close();
        gThrMC.join();
        gThrMC = std::thread();
    }
}

//
// MARK: Global public functions
//

// Initialize the module
void RemoteInit ()
{
    // Create the global multicast object
    if (!gpMc)
        gpMc = new UDPMulticast();
}

// Grace cleanup, stops all threads
void RemoteCleanup ()
{
    RmtStopAll();               // stop everything (multicast listening/sending, thread)
    if (gpMc) {                 // remove the multicast object
        delete gpMc;
        gpMc = nullptr;
    }
}

// Returns the current Remote status
RemoteStatusTy RemoteGetStatus()
{
    return glob.remoteStatus;
}

// Compares current vs. expected status and takes appropriate action
void RemoteSenderUpdateStatus ()
{
    // Don't change anything if in fact we are in listening mode
    if (glob.RemoteIsListener())
        return;
        
    // only anything else than off if there are planes
    RemoteStatusTy expected = REMOTE_OFF;
    if (XPMPCountPlanes() > 0) {
        switch (glob.remoteCfg) {
            // if conditionally on: Test if we are in a networked setup
            case REMOTE_CFG_CONDITIONALLY:
                if (glob.bXPNetworkedSetup)
                    expected = glob.remoteStatus == REMOTE_SENDING ? REMOTE_SENDING : REMOTE_SEND_WAITING;
                break;
            // forcibly on?
            case REMOTE_CFG_ON:
                expected = glob.remoteStatus == REMOTE_SENDING ? REMOTE_SENDING : REMOTE_SEND_WAITING;
                break;
            // forcibly off?
            case REMOTE_CFG_OFF:
                break;
        }
    }
    
    // So no change then?
    if (expected == glob.remoteStatus)
        return;
    
    // Switch off?
    if (expected == REMOTE_OFF)
        RmtStopAll();
    else
        RmtStartMCThread(true);
}

// Regularly called from the flight loop callback
void RemoteSendAc ()
{
    // Can only do anything reasonable if we are to send data
    if (glob.remoteStatus != REMOTE_SENDING)
        return;
}

// Starts the listener, will call provided callback functions with received messages
void RemoteRecvStart (const RemoteCBFctTy& _rmtCBFcts)
{
    gRmtCBFcts = _rmtCBFcts;
    RmtStartMCThread(false);
}

// Stops the receiver
void RemoteRecvStop ()
{
    RmtStopAll();
}

};
