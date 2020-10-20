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

//
// MARK: Thread for Remote Data
//

#define DEBUG_MC_LISTENING  "Listening to %s:%d, waiting for someone interested in our data..."
#define INFO_MC_LISTENING   "Listening on the network, waiting for someone interested in our data..."
#define INFO_MC_RCVD        "Received word from %s, will start sending aircraft data"
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

void RmtMain()
{
    // This is a thread main function, set thread's name
    SET_THREAD_NAME("XPMP2_Remote");
    
    try {
        LOG_ASSERT(gpMc != nullptr);
        
        // Set global status to: we are "waiting" for some action on the multicast group
        glob.remoteStatus = REMOTE_WAITING;

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
            LOG_MSG(logDEBUG, DEBUG_MC_LISTENING, glob.remoteMCGroup.c_str(), glob.remotePort)
        else
            LOG_MSG(logINFO, INFO_MC_LISTENING)
        
        while (!gbStopMCThread && glob.remoteStatus == REMOTE_WAITING && gpMc->isOpen())
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
                LOG_MSG(logINFO, INFO_MC_RCVD, from.c_str());

                // Set global status to: we are about to send data, also exits listening loop
                glob.remoteStatus = REMOTE_SENDING;
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
void RmtStartListenThread()
{
    // Can only do that if currently off
    if (glob.remoteStatus != REMOTE_OFF)
        return;
    
    // Is or was there a thread running?
    if (gThrMC.joinable()) {
        if (!gbStopMCThread)             // it still is running
            return;                     // keep it running
        gThrMC.join();                   // wait for the current thread to exit (which it should because bStopMCListen is true)
        gThrMC = std::thread();
    }
    
    // seen too many errors already?
    if (gCntMCErr >= MC_MAX_ERR)
        return;
    
    // Start the listener thread
    gbStopMCThread = false;
    gThrMC = std::thread(RmtMain);
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

// Compares current vs. expected status and takes appropriate action
void RemoteUpdateStatus ()
{
    RemoteStatusTy expected = REMOTE_OFF;
    
    // only anything else than off if there are planes
    if (XPMPCountPlanes() > 0) {
        switch (glob.remoteCfg) {
            // if conditionally on: Test if we are in a networked setup
            case REMOTE_CFG_CONDITIONALLY:
                if (glob.bXPNetworkedSetup)
                    expected = glob.remoteStatus == REMOTE_SENDING ? REMOTE_SENDING : REMOTE_WAITING;
                break;
            // forcibly on?
            case REMOTE_CFG_ON:
                expected = glob.remoteStatus == REMOTE_SENDING ? REMOTE_SENDING : REMOTE_WAITING;
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
        RmtStartListenThread();
}

// Regularly called from the flight loop callback
void RemoteUpdateAc ()
{
    // Can only do anything reasonable if we are to send data
    if (glob.remoteStatus != REMOTE_SENDING)
        return;
}

};
