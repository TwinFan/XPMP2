/// @file       Network.cpp
/// @brief      Low-level network communications, especially for TCP/UDP
/// @see        Some inital ideas and pieces of code take from
///             https://linux.m2osw.com/c-implementation-udp-clientserver
/// @details    XPMP2::SocketNetworking: Any network socket connection\n
///             XPMP2::UDPReceiver: listens to and receives UDP datagram\n
///             XPMP2::UDPMulticast: sends and receives multicast UDP datagrams\n
///             XPMP2::TCPConnection: receives incoming TCP connection\n
/// @author     Birger Hoppe
/// @copyright  (c) 2019-2024 Birger Hoppe
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

// All includes are collected in one header
#include "XPMP2.h"

#if IBM
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SSIZE_T ssize_t;
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <net/if.h>
#endif

namespace XPMP2 {

#if IBM
#undef errno
#define errno WSAGetLastError()     // https://docs.microsoft.com/en-us/windows/desktop/WinSock/error-codes-errno-h-errno-and-wsagetlasterror-2
#define close closesocket
typedef USHORT in_port_t;
typedef int socklen_t;              // in Winsock, an int is passed as length argument
#endif

constexpr int SERR_LEN = 100;                   // size of buffer for IO error texts (strerror_s)

/// List of local IP addresses
std::vector<InetAddrTy> gaddrLocal;
/// List of IPv4 multicast-enabled interface addresses (one address per interface)
std::vector<struct in_addr> gIp4IntfAddr;
/// Map of IPv6 multicast-enabled interface indexes to potentially several addresses
std::multimap<uint32_t,struct in6_addr> gIp6IntfIdx;


// Constructor copies given socket info
SockAddrTy::SockAddrTy (const sockaddr* pSa)
{
    memset(this, 0, sizeof(SockAddrTy));
    if (pSa) {
        switch (pSa->sa_family) {
            case AF_INET:
                memcpy(&sa_in, pSa, sizeof(sa_in));
                break;
            case AF_INET6:
                memcpy(&sa_in6, pSa, sizeof(sa_in6));
                break;
            default:
                memcpy(&sa, pSa, sizeof(sa));
        }
    }
}


//
// MARK: SocketNetworking
//

/// Mutex to ensure closing is done in one thread only to avoid race conditions on deleting the buffer
std::recursive_mutex mtxSocketClose;

NetRuntimeError::NetRuntimeError(const std::string& w) :
std::runtime_error(w), fullWhat(w)
{
    // make network error available
    char sErr[SERR_LEN];
    strerror_s(sErr, sizeof(sErr), errno);
    errTxt = sErr;              // copy
    // And the complete message to be returned by what readily prepared:
    fullWhat += ": ";
    fullWhat += errTxt;
}

SocketNetworking::SocketNetworking(const std::string& _addr, int _port,
                                   size_t _bufSize, unsigned _timeOut_ms,
                                   bool _bBroadcast) :
f_port(_port), f_addr(_addr)
{
    // open the socket
    Open(_addr, _port, _bufSize, _timeOut_ms, _bBroadcast);
}

// cleanup: make sure the socket is closed and all memory cleanup up
SocketNetworking::~SocketNetworking()
{
    Close();
}

void SocketNetworking::Open(const std::string& _addr, int _port,
                       size_t _bufSize, unsigned _timeOut_ms, bool _bBroadcast)
{
    struct addrinfo *   addrinfo      = NULL;
    try {
        // store member values
        f_port = _port;
        f_addr = _addr;
        const std::string decimal_port(std::to_string(f_port));

        // get a valid address based on inAddr/port
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        GetAddrHints(hints);            // ask subclasses
        
        // Any address?
        if (f_addr.empty())
            f_addr = hints.ai_family == AF_INET6 ? "::" : "0.0.0.0";
        
        // Need to get
        int r = getaddrinfo(f_addr.c_str(),
                            decimal_port.c_str(), &hints, &addrinfo);
        if(r != 0 || addrinfo == NULL)
            throw NetRuntimeError("invalid address or port for socket: \"" + f_addr + ":" + decimal_port + "\"");
        
        // get a socket
        f_socket = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
        if(f_socket == INVALID_SOCKET)
            throw NetRuntimeError("could not create socket for: \"" + f_addr + ":" + decimal_port + "\"");
        
        // Reuse address and port to allow others to connect, too
        int setToVal = 1;
#if IBM
        if (setsockopt(f_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&setToVal, sizeof(setToVal)) < 0)
            throw NetRuntimeError("could not setsockopt SO_REUSEADDR for: \"" + f_addr + ":" + decimal_port + "\"");
#else
        if (setsockopt(f_socket, SOL_SOCKET, SO_REUSEADDR, &setToVal, sizeof(setToVal)) < 0)
            throw NetRuntimeError("could not setsockopt SO_REUSEADDR for: \"" + f_addr + ":" + decimal_port + "\"");
        if (setsockopt(f_socket, SOL_SOCKET, SO_REUSEPORT, &setToVal, sizeof(setToVal)) < 0)
            throw NetRuntimeError("could not setsockopt SO_REUSEPORT for: \"" + f_addr + ":" + decimal_port + "\"");
#endif

        // define receive timeout
#if IBM
        DWORD wsTimeout = _timeOut_ms;
        if (setsockopt(f_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&wsTimeout, sizeof(wsTimeout)) < 0)
            throw NetRuntimeError("could not setsockopt SO_RCVTIMEO for: \"" + f_addr + ":" + decimal_port + "\"");
#else
        struct timeval timeout;
        timeout.tv_sec = _timeOut_ms / 1000;
        timeout.tv_usec = (_timeOut_ms % 1000) * 1000;
        if (setsockopt(f_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
            throw NetRuntimeError("could not setsockopt SO_RCVTIMEO for: \"" + f_addr + ":" + decimal_port + "\"");
#endif
        
        // if requested allow for sending broadcasts
        if (_bBroadcast) {
            setToVal = 1;
#if IBM
            if (setsockopt(f_socket, SOL_SOCKET, SO_BROADCAST, (char*)&setToVal, sizeof(setToVal)) < 0)
                throw NetRuntimeError("could not setsockopt SO_BROADCAST for: \"" + f_addr + ":" + decimal_port + "\"");
#else
            if (setsockopt(f_socket, SOL_SOCKET, SO_BROADCAST, &setToVal, sizeof(setToVal)) < 0)
                throw NetRuntimeError("could not setsockopt SO_BROADCAST for: \"" + f_addr + ":" + decimal_port + "\"");
#endif
        }

        // bind the socket to the address:port
        r = bind(f_socket, addrinfo->ai_addr, (socklen_t)addrinfo->ai_addrlen);
        if(r != 0)
            throw NetRuntimeError("could not bind UDP socket with: \"" + f_addr + ":" + decimal_port + "\"");

        // free adress info
        freeaddrinfo(addrinfo);
        addrinfo = NULL;

        // reserve receive buffer
        SetBufSize(_bufSize);
    }
    catch (...) {
        // free adress info
        if (addrinfo) {
            freeaddrinfo(addrinfo);
            addrinfo = NULL;
        }
        // make sure everything is closed
        Close();
        // re-throw
        throw;
    }
}

// Connect to a server
void SocketNetworking::Connect(const std::string& _addr, int _port,
                               size_t _bufSize, unsigned _timeOut_ms)
{
    struct addrinfo *   addrinfo      = NULL;
    try {
        // store member values
        f_port = _port;
        f_addr = _addr;
        const std::string decimal_port(std::to_string(f_port));

        // get a valid address based on inAddr/port
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        GetAddrHints(hints);            // ask subclasses, but then overwrite some stuff
        hints.ai_flags &= ~AI_PASSIVE;
        
        int r = getaddrinfo(f_addr.c_str(), decimal_port.c_str(), &hints, &addrinfo);
        if(r != 0 || addrinfo == NULL)
            throw NetRuntimeError("invalid address or port for socket: \"" + f_addr + ":" + decimal_port + "\"");
        
        // get a socket
        f_socket = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
        if(f_socket == INVALID_SOCKET)
            throw NetRuntimeError("could not create socket for: \"" + f_addr + ":" + decimal_port + "\"");
        
        // define receive timeout
#if IBM
        DWORD wsTimeout = _timeOut_ms;
        if (setsockopt(f_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&wsTimeout, sizeof(wsTimeout)) < 0)
            throw NetRuntimeError("could not setsockopt SO_RCVTIMEO for: \"" + f_addr + ":" + decimal_port + "\"");
#else
        struct timeval timeout;
        timeout.tv_sec = _timeOut_ms / 1000;
        timeout.tv_usec = (_timeOut_ms % 1000) * 1000;
        if (setsockopt(f_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
            throw NetRuntimeError("could not setsockopt SO_RCVTIMEO for: \"" + f_addr + ":" + decimal_port + "\"");
#endif
        
        // actually connect
        r = connect(f_socket, addrinfo->ai_addr, (socklen_t)addrinfo->ai_addrlen);
        if(r != 0)
            throw NetRuntimeError("could not connect to: \"" + f_addr + ":" + decimal_port + "\"");

        // free adress info
        freeaddrinfo(addrinfo);
        addrinfo = NULL;

        // reserve receive buffer
        SetBufSize(_bufSize);
    }
    catch (...) {
        // free adress info
        if (addrinfo) {
            freeaddrinfo(addrinfo);
            addrinfo = NULL;
        }
        // make sure everything is closed
        Close();
        // re-throw
        throw;
    }
}

/** \brief Clean up the UDP server.
 *
 // Close: This function frees the address info structures and close the socket.
 */
void SocketNetworking::Close()
{
    std::lock_guard<std::recursive_mutex> lock(mtxSocketClose);
    // cleanup
    if (f_socket != INVALID_SOCKET) {
        close(f_socket);
        f_socket = INVALID_SOCKET;
    }
    
    // release buffer
    SetBufSize(0);
}

// allocates the receiving buffer
void SocketNetworking::SetBufSize(size_t _bufSize)
{
    std::lock_guard<std::recursive_mutex> lock(mtxSocketClose);
    // remove existing buffer
    if (buf) {
        delete[] buf;
        buf = NULL;
        bufSize = 0;
    }
    
    // create a new one
    if (_bufSize > 0) {
        buf = new char[bufSize=_bufSize];
        memset(buf, 0, bufSize);
    }
}

// updates the error text and returns it
std::string SocketNetworking::GetLastErr()
{
    char sErr[SERR_LEN];
#if IBM
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,   // flags
        NULL,                                                                   // lpsource
        WSAGetLastError(),                                                      // message id
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),                              // languageid
        sErr,                                                                   // output buffer
        sizeof(sErr),                                                           // size of msgbuf, bytes
        NULL);
#else
    strerror_s(sErr, sizeof(sErr), errno);
#endif
    return std::string(sErr);
}


/** \brief Wait on a message.
 *
 * This function waits until a message is received on this UDP server.
 * There are no means to return from this function except by receiving
 * a message. Remember that UDP does not have a connect state so whether
 * another process quits does not change the status of this UDP server
 * and thus it continues to wait forever.
 *
 * Note that you may change the type of socket by making it non-blocking
 * (use the get_socket() to retrieve the socket identifier) in which
 * case this function will not block if no message is available. Instead
 * it returns immediately.
 *
 * \return The number of bytes read or -1 if an error occurs.
 */
long SocketNetworking::recv()
{
    if (!buf) {
#if IBM
        WSASetLastError(WSA_NOT_ENOUGH_MEMORY);
#else
        errno = ENOMEM;
#endif
        return -1;
    }
    
    long ret = ::recv(f_socket, buf, (socklen_t)bufSize-1, 0);
    if (ret >= 0)  {                    // we did receive something
        buf[ret] = 0;                   // zero-termination
    } else {
        buf[0] = 0;                     // empty string
    }
    return ret;
}

/** \brief Wait for data to come in.
 *
 * This function waits for a given amount of time for data to come in. If
 * no data comes in after max_wait_ms, the function returns with -1 and
 * errno set to EAGAIN.
 *
 * The socket is expected to be a blocking socket (the default,) although
 * it is possible to setup the socket as non-blocking if necessary for
 * some other reason.
 *
 * This function blocks for a maximum amount of time as defined by
 * max_wait_ms. It may return sooner with an error or a message.
 *
 * \param[in] max_wait_ms  The maximum number of milliseconds to wait for a message.
 *
 * \return -1 if an error occurs or the function timed out, the number of bytes received otherwise.
 */
long SocketNetworking::timedRecv(int max_wait_ms)
{
    fd_set sRead, sErr;
    struct timeval timeout;

    FD_ZERO(&sRead);
    FD_SET(f_socket, &sRead);           // check our socket
    FD_ZERO(&sErr);                     // also for errors
    FD_SET(f_socket, &sErr);

    timeout.tv_sec = max_wait_ms / 1000;
    timeout.tv_usec = (max_wait_ms % 1000) * 1000;
    int retval = select((int)f_socket + 1, &sRead, NULL, &sErr, &timeout);
    if(retval == -1)
    {
        // select() set errno accordingly
        buf[0] = 0;                     // empty string
        return -1;
    }
    if(retval > 0)
    {
        // was it an error that triggered?
        if (FD_ISSET(f_socket,&sErr)) {
            return -1;
        }
        
        // our socket has data
        if (FD_ISSET(f_socket, &sRead))
            return recv();
    }
    
    // our socket has no data
    buf[0] = 0;                     // empty string
#if IBM
    WSASetLastError(WSAEWOULDBLOCK);
#else
    errno = EAGAIN;
#endif
    return -1;
}

// sends the message as a broadcast
bool SocketNetworking::broadcast (const char* msg)
{
    int index=0;
    int length = (int)strlen(msg);
    
    struct sockaddr_in s;
    memset(&s, '\0', sizeof(struct sockaddr_in));
    s.sin_family = AF_INET;
    s.sin_port = htons((in_port_t)f_port);
    s.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    
    while (index<length) {
        int count = (int)::sendto(f_socket, msg + index, (socklen_t)(length - index), 0,
                                  (struct sockaddr *)&s, sizeof(s));
        if (count<0) {
            if (errno==EINTR) continue;
            LOG_MSG(logERR, "%s (%s)",
                    ("sendto failed: \"" + f_addr + ":" + std::to_string(f_port) + "\"").c_str(),
                    GetLastErr().c_str());
            return false;
        } else {
            index+=count;
        }
    }
    return true;
}


// return a string for a IPv4 and IPv6 address
std::string SocketNetworking::GetAddrString (const SockAddrTy& sa, bool withPort)
{
    std::string ret;
    char host[INET6_ADDRSTRLEN] = "", service[NI_MAXSERV] = "";
    
    int gai_err = getnameinfo(&sa.sa, sa.size(),
                              host, sizeof(host),
                              service, sizeof(service), NI_NUMERICSERV | NI_NUMERICHOST);
    if (gai_err)
        throw std::runtime_error(gai_strerror(gai_err));

    // combine host and service as requested
    if (withPort) {
        ret = '[';
        ret += host;
        ret += "]:";
        ret += service;
    } else {
        ret = host;
    }
    return ret;
}

//
// MARK: UDPReceiver
//

// UDP only allows UDP
void UDPReceiver::GetAddrHints (struct addrinfo& hints)
{
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
}

//
// MARK: UDPMulticast
//

// Constructor
UDPMulticast::UDPMulticast(const std::string& _multicastAddr, int _port, int _ttl,
                           size_t _bufSize, unsigned _timeOut_ms) :
    SocketNetworking()
{
    Join(_multicastAddr, _port, _ttl, _bufSize, _timeOut_ms);
}

// makes sure pMCAddr is cleared up
UDPMulticast::~UDPMulticast()
{
    Cleanup();
}

// Connect to the multicast group
void UDPMulticast::Join (const std::string& _multicastAddr, int _port, int _ttl,
                         size_t _bufSize, unsigned _timeOut_ms)
{
    // Already connected? -> disconnect first
    if (isOpen())
        Close();
    
    // save all values
    multicastAddr = _multicastAddr;
    f_port = _port;
    const std::string decimal_port(std::to_string(f_port));
    
    // Resolve the multicast address, also determines AI Family
    Cleanup();
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    if (getaddrinfo(multicastAddr.c_str(), decimal_port.c_str(), &hints, &pMCAddr) != 0)
        throw NetRuntimeError ("getaddrinfo failed for " + multicastAddr);
    if (pMCAddr == nullptr)
        throw NetRuntimeError ("No address info found for " + multicastAddr);
    if (pMCAddr->ai_family != AF_INET && pMCAddr->ai_family != AF_INET6)
        throw NetRuntimeError ("Multicast address " + multicastAddr + " was not resolved as IPv4 or IPv6 address but as " + std::to_string(pMCAddr->ai_family));
    hints.ai_family = pMCAddr->ai_family;
    
    // open the socket
    Open("", _port, _bufSize, _timeOut_ms);

    // Have address info ready for the "any" interface
    struct addrinfo* pAnyAddrInfo = nullptr;
    hints.ai_flags |= AI_PASSIVE;
    if (getaddrinfo(nullptr, "0", &hints, &pAnyAddrInfo) != 0)
        throw NetRuntimeError ("getaddrinfo failed for NULL address");

    
    // Actually join the multicast group
    if (IsIPv4())
    {
        // Setup the v4 option values and ip_mreq structure
        ip_mreq mreqv4 = {};
        mreqv4.imr_multiaddr.s_addr = ((sockaddr_in*)pMCAddr->ai_addr)->sin_addr.s_addr;
        mreqv4.imr_interface.s_addr = in_addr{INADDR_ANY}.s_addr;
        if (setsockopt(f_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*) &mreqv4, sizeof(mreqv4)) < 0)
            throw NetRuntimeError ("setsockopt(IP_ADD_MEMBERSHIP/IPV6_JOIN_GROUP) failed");
        if (setsockopt(f_socket, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&_ttl, sizeof(_ttl)) < 0)
            throw NetRuntimeError ("setsockopt(IP_MULTICAST_TTL) failed");
    }
    else    // AF_INET6
    {
        int on = 1;
        setsockopt(f_socket, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (const char*) &on, sizeof(on));
        if (setsockopt(f_socket, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (char*)&_ttl, sizeof(_ttl)) < 0)
            throw NetRuntimeError ("setsockopt(IPV6_MULTICAST_HOPS) failed");

        // Setup the v6 option values and ipv6_mreq structure
        ipv6_mreq mreqv6 = {};
        mreqv6.ipv6mr_multiaddr = ((sockaddr_in6*)pMCAddr->ai_addr)->sin6_addr;
        
        // Need to join for all interface separately
        NetwGetLocalAddresses();                            // find all addresses, populates gIp6IntfIdx
        int cntSucces = 0;
        mreqv6.ipv6mr_interface = 0;
        for (const auto& p: gIp6IntfIdx) {
            if (p.first != mreqv6.ipv6mr_interface) {
                mreqv6.ipv6mr_interface = p.first;
                if (setsockopt(f_socket, IPPROTO_IPV6, IPV6_JOIN_GROUP, (const char*) &mreqv6, sizeof(mreqv6)) < 0) {
                    char sErr[SERR_LEN];
                    strerror_s(sErr, sizeof(sErr), errno);
                    LOG_MSG(logWARN, "MC %s: setsockopt(IPV6_JOIN_GROUP) failed for intf idx %u: %d - %s",
                            multicastAddr.c_str(), mreqv6.ipv6mr_interface,
                            errno, sErr);
                } else {
                    ++cntSucces;
                }
            }
        }
        // not a single interface accepted me?
        if (!cntSucces)
            throw NetRuntimeError ("setsockopt(IPV6_JOIN_GROUP) failed for all interfaces!");
    }
    
    // Set the send interface to default
    SendToDefault();

    // free local address info
    freeaddrinfo(pAnyAddrInfo);
}

// Send future datagrams on default interfaces only (default)
void UDPMulticast::SendToDefault ()
{
    aIf4.clear();
    aIf6.clear();
    
    if (!pMCAddr) throw NetRuntimeError("Multicast address not yet defined");
    
    // Set the send interface to default
    if (IsIPv4())                                   // IPv4
        SetSendInterface(in_addr{INADDR_ANY});
    else  {                                         // IPv6 doesn't allow assigning `0` generally, so we pick the first from our list
        if (!gIp6IntfIdx.empty())
            SetSendInterface(gIp6IntfIdx.begin()->first);
    }
    
    LOG_MSG(logDEBUG, "MC %s: Sending via default interface",
            multicastAddr.c_str());
}

// Send future datagrams on _all_ interfaces
void UDPMulticast::SendToAll ()
{
    aIf4.clear();
    aIf6.clear();
    
    NetwGetLocalAddresses ();               // makes sure lists of multicast interfaces are filled, too
    
    aIf4 = gIp4IntfAddr;                    // copy all IPv4 interface addresses
    
    uint32_t prevIdx = 0;                   // copy all IPv6 interfaces idx, but each only once
    for(const auto& p: gIp6IntfIdx)
        if (p.first != prevIdx)
            aIf6.push_back(prevIdx = p.first);
    
    LOG_MSG(logDEBUG, "MC %s: Sending on ALL %lu interfaces",
            multicastAddr.c_str(),
            !pMCAddr || (pMCAddr->ai_family == AF_INET) ? (long unsigned)aIf4.size() : (long unsigned)aIf6.size());
}

// Send future datagrams on this particular interface only
void UDPMulticast::SendToAddr (const SockAddrTy& sa)
{
    if (!pMCAddr) throw NetRuntimeError("Multicast address not yet defined");
    if (sa.family() != pMCAddr->ai_family)
        throw NetRuntimeError("Send Address' family differs from MC's family");
    
    // indicates 'no looping' to SendMC()
    aIf4.clear();
    aIf6.clear();

    // Set the send interface as requested
    // IPv4
    if (sa.isIp4()) {
        // TODO: This will probably not work...this is not the address of the receiving interface but the address of the sender...
        SetSendInterface(sa.sa_in.sin_addr);
        LOG_MSG(logINFO, "MC %s: Sending via interface %s only",
                multicastAddr.c_str(),
                GetAddrString(sa, false).c_str());
    }
    // IPv6
    else if (sa.isIp6())
    {
        SetSendInterface(sa.sa_in6.sin6_scope_id);
    }
}


// Send a multicast
size_t UDPMulticast::SendMC (const void* _bufSend, size_t _bufSendSize)
{
    ssize_t bytesSent = 0;
    if (!pMCAddr || !isOpen())
        throw NetRuntimeError("SendMC: Multicast socket not open");
    
    // We want one loop only, but we need to loop over two different types of vectors (IPv4 vs IPv6)
    // or over nothing in case the vectors have zero size
    size_t count = IsIPv4() ? aIf4.size() : aIf6.size();
    if (!count) count = 1;                          // we need to send at least once
    for (size_t i = 0; i < count; ++i) {
        // Set the sending interface if we are looping over all interfaces
        if (IsIPv4() && i < aIf4.size())
            SetSendInterface(aIf4[i]);
        else if (IsIPv6() && i < aIf6.size())
            SetSendInterface(aIf6[i]);
        
        // Do the actual sending
        bytesSent =
    #if IBM
        (ssize_t)sendto(f_socket, (const char*)_bufSend, (int)_bufSendSize, 0, pMCAddr->ai_addr, (int)pMCAddr->ai_addrlen);
    #else
        sendto(f_socket, _bufSend, _bufSendSize, 0, pMCAddr->ai_addr, pMCAddr->ai_addrlen);
    #endif
        if (bytesSent < 0)
            throw NetRuntimeError("SendMC: sendto failed to send " + std::to_string(_bufSendSize) + " bytes");
    }
    
    return size_t(bytesSent);
}

// @brief Receive a multicast
size_t UDPMulticast::RecvMC (std::string* _pFromAddr, SockAddrTy* _pFromSockAddr)
{

    if (!pMCAddr || !buf || !isOpen())
        throw NetRuntimeError("RecvMC: Multicast socket not initialized");
    
    SockAddrTy sa;
    sa.sa.sa_family = (decltype(sa.sa.sa_family))pMCAddr->ai_family;
    socklen_t fromlen = sa.size();

    memset(buf, 0, bufSize);
    ssize_t bytesRcvd =
#if IBM
    (ssize_t) recvfrom(f_socket, buf, (int)bufSize, 0, &sa.sa, &fromlen);
#else
    recvfrom(f_socket, buf, bufSize, 0, &sa.sa, &fromlen);
#endif
    if (bytesRcvd < 0)
        throw NetRuntimeError("RecvMC: recvfrom failed");

    // Sender address wanted?
    if (_pFromAddr)
        *_pFromAddr = GetAddrString(sa);
    if (_pFromSockAddr)
        *_pFromSockAddr = sa;
    
    return size_t(bytesRcvd);
}

// frees pMCAddr
void UDPMulticast::Cleanup ()
{
    std::lock_guard<std::recursive_mutex> lock(mtxSocketClose);
    if (pMCAddr) {
        freeaddrinfo(pMCAddr);
        pMCAddr = nullptr;
    }
}


// returns information from `*pMCAddr`
void UDPMulticast::GetAddrHints (struct addrinfo& hints)
{
    assert(pMCAddr);                // must have called Join first! This will break in the debugger and we can investigate
    if (!pMCAddr) throw NetRuntimeError ("pMCAddr is NULL, didn't call Join before?");  // this kinda protects the release version

    hints.ai_family = pMCAddr->ai_family;
    hints.ai_socktype = pMCAddr->ai_socktype;
    hints.ai_protocol = pMCAddr->ai_protocol;
}

// Set multicast send interface (IPv4)
void UDPMulticast::SetSendInterface (const in_addr& addr)
{
    if (setsockopt(f_socket, IPPROTO_IP, IP_MULTICAST_IF,
                   (const char*) &addr, sizeof(addr)) < 0)
        throw NetRuntimeError ("setsockopt(IP_MULTICAST_IF) failed");
}

// Set multicast send interface (IPv6)
void UDPMulticast::SetSendInterface (uint32_t ifIdx)
{
    if (setsockopt(f_socket, IPPROTO_IPV6, IPV6_MULTICAST_IF,
                   (const char*) &ifIdx, sizeof(ifIdx)) < 0)
        throw NetRuntimeError ("setsockopt(IPV6_MULTICAST_IF) failed");
}


//
// MARK: TCPConnection
//

void TCPConnection::Close()
{
    // close listener first
    CloseListenerOnly();

    // also close session connection
    if (f_session_socket != INVALID_SOCKET) {
        close(f_session_socket);
        f_session_socket = INVALID_SOCKET;
    }
}

// only closes the listening socket, but not a session connection
void TCPConnection::CloseListenerOnly()
{
#if APL == 1 || LIN == 1
    // Mac/Lin: Try writing something to the self-pipe to stop gracefully
    if (selfPipe[1] == INVALID_SOCKET ||
        write(selfPipe[1], "STOP", 4) < 0)
    {
        // if the self-pipe didn't work:
#endif
        // just call the base class Close, bypassing our own virtual function
        SocketNetworking::Close();
#if APL == 1 || LIN == 1
    }
#endif
}

// Listen for and accept connections
void TCPConnection::listen (int numConnections)
{
    if (::listen(f_socket, numConnections) < 0)
        throw NetRuntimeError("Can't listen on socket " + f_addr + ":" + std::to_string(f_port));
}

bool TCPConnection::accept (bool bUnlisten)
{
    socklen_t addrLen = sizeof(f_session_addr);
    memset (&f_session_addr, 0, sizeof(f_session_addr));
    
    // potentially blocking call
    f_session_socket = ::accept (f_socket, (struct sockaddr*)&f_session_addr, &addrLen);
    
    // if we are to "unlisten" then we close the listening socket
    if (f_session_socket != INVALID_SOCKET && bUnlisten) {
        CloseListenerOnly();
    }
    
    // successful?
    return f_session_socket != INVALID_SOCKET;
}

// just combines the above and includes a `select` on a self-pipe
// to be able to interrupt waiting at any time
bool TCPConnection::listenAccept (int numConnections)
{
    try {
        listen(numConnections);

#if APL == 1 || LIN == 1
        // the self-pipe to shut down the listener gracefully
        if (pipe(selfPipe) < 0)
            throw NetRuntimeError("Couldn't create self-pipe");
        const SOCKET readPipe = selfPipe[0];
        fcntl(readPipe, F_SETFL, O_NONBLOCK);
        
        // wait for an incoming connection or a signal on the pipe
        fd_set sRead;
        FD_ZERO(&sRead);
        FD_SET(f_socket, &sRead);       // check our listen sockets
        FD_SET(readPipe, &sRead);       // and the self-pipe
        const int maxSock = std::max(f_socket, selfPipe[0]) + 1;
        for(;;) {
            int retval = select(maxSock, &sRead, NULL, NULL, NULL);
            
            // select call failed?
            if(retval < 0)
                throw NetRuntimeError("'select' failed");
            
            // if anything is available
            if (retval > 0) {
                // something is available now, so we no longer need the self-pipe
                for (SOCKET &s: selfPipe) {
                    close(s);
                    s = INVALID_SOCKET;
                }

                // check for self-pipe...if so then just exit
                if (FD_ISSET(readPipe, &sRead))
                    return false;
            
                // exit loop if the listen-socket is ready to be read
                if (FD_ISSET(f_socket, &sRead))
                    break;
            }
        }
#endif
        // if we wait for exactly one connection then we "unlisten" once we accepted that one connection:
        return accept(numConnections == 1);
    }
    catch (std::exception& e) {
        LOG_MSG(logERR, "%s", e.what());
    }
    return false;
}

// write a message out
bool TCPConnection::send(const char* msg)
{
    // prefer the session socket set after waiting for a connection,
    // but try the main socket if session socket not set
    SOCKET f = f_session_socket == INVALID_SOCKET ? f_socket : f_session_socket;
    
    int index=0;
    int length = (int)strlen(msg);
    while (index<length) {
        int count = (int)::send(f, msg + index, (socklen_t)(length - index), 0);
        if (count<0) {
            if (errno==EINTR) continue;
            LOG_MSG(logERR, "%s (%s)",
                    ("send failed: \"" + f_addr + ":" + std::to_string(f_port) + "\"").c_str(),
                    GetLastErr().c_str());
            return false;
        } else {
            index+=count;
        }
    }
    return true;
}

// TCP only allows TCP
void TCPConnection::GetAddrHints (struct addrinfo& hints)
{
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
}

//
// MARK: IP address helpers
//

/// Extracts the numerical address and puts it into addr for generic address use
void InetAddrTy::CopyFrom (const SockAddrTy& sa)
{
    switch (sa.family()) {
        case AF_INET: {
            addr[0] = sa.sa_in.sin_addr.s_addr;
            addr[1] = addr[2] = addr[3] = 0;
            break;
        }
            
        case AF_INET6: {
            memcpy(addr, sa.sa_in6.sin6_addr.s6_addr, sizeof(addr));
            break;
        }
            
        default:
            memset(addr, 0, sizeof(addr));
            break;
    }
}


/// Return all local addresses (also cached locally)
const std::vector<InetAddrTy>& NetwGetLocalAddresses ()
{
    // Need to fill the cache?
    if (gaddrLocal.empty()) {
#if IBM
        // Fetch all local interface addresses
        ULONG ret = NO_ERROR;
        ULONG bufLen = 15000;
        IP_ADAPTER_ADDRESSES* pAddresses = NULL;
        do {
            // allocate a return buffer
            pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(bufLen);
            if (!pAddresses) {
                LOG_MSG(logERR, "malloc failed for %lu bytes", bufLen);
                return gaddrLocal;
            }
            // try fetching all local addresses
            ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses, &bufLen);
            switch (ret) {
            case ERROR_NO_DATA:                 // no addresses found (?)
                free(pAddresses);
                return gaddrLocal;
            case ERROR_BUFFER_OVERFLOW:         // buffer too small, but bufLen has the required size now, try again
                free(pAddresses);
                pAddresses = NULL;
                break;
            case NO_ERROR:                      // success
                break;
            default:
                LOG_MSG(logERR, "GetAdaptersAddresses failed with error: %ld", ret);
                free(pAddresses);
                return gaddrLocal;
            }
        } while (ret == ERROR_BUFFER_OVERFLOW);

        // Walk the list of addresses and add to our own vector
        for (const IP_ADAPTER_ADDRESSES* pAddr = pAddresses;
             pAddr;
             pAddr = pAddr->Next)
        {
            // Walk the list of unicast addresses
            bool bNeedIp4Addr = pAddr->Ipv4Enabled && !pAddr->NoMulticast;
            const bool bNeedIp6Addr = pAddr->Ipv6Enabled && !pAddr->NoMulticast;
            for (const IP_ADAPTER_UNICAST_ADDRESS* pUni = pAddr->FirstUnicastAddress;
                 pUni;
                 pUni = pUni->Next)
            {
                const sockaddr* pSAddr = pUni->Address.lpSockaddr;
                if (!pSAddr) continue;
                gaddrLocal.emplace_back(pSAddr);
                
                // Store one and only one IP4 address for use with multicast sending
                if (bNeedIp4Addr && pSAddr->sa_family == AF_INET) {
                    const sockaddr_in* pSInAddr = reinterpret_cast<const sockaddr_in*>(pSAddr);
                    gIp4IntfAddr.push_back(pSInAddr->sin_addr);
                    bNeedIp4Addr = false;
                }
                
                // For IPv6 store all interfaces addresses with their interface idx
                if (bNeedIp6Addr && pSAddr->sa_family == AF_INET6) {
                    const sockaddr_in6* pSaIn6 = reinterpret_cast<const sockaddr_in6*>(pSAddr);
                    gIp6IntfIdx.emplace(pAddr->Ipv6IfIndex,pSaIn6->sin6_addr);
                }
            }
        }

        // free the memory
        free(pAddresses);
        pAddresses = NULL;
#else
        // List of IPv4 multicast-enabled interface indexes
        std::set<uint32_t> ip4IntfIdx;
        gIp4IntfAddr.clear();
        gIp6IntfIdx.clear();

        // Fetch all local interface addresses
        struct ifaddrs *ifaddr = nullptr;
        if (getifaddrs(&ifaddr) == -1) {
            LOG_MSG(logERR, "getifaddrs failed: %s",
                    SocketNetworking::GetLastErr().c_str());
            return gaddrLocal;
        }

        // add valid addresses to our local cache array
        for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            // ignore empty addresses and everything that is not IP
            if (ifa->ifa_addr == NULL)
                continue;
            
            const struct sockaddr& sa = *ifa->ifa_addr;
            if (ifa->ifa_addr->sa_family != AF_INET &&
                sa.sa_family != AF_INET6)
                continue;
            // the others add to our list of local addresses
            gaddrLocal.emplace_back(ifa->ifa_addr);
            
            // If multicast-enabled add to our list of multicast interfaces
            if ((ifa->ifa_flags & (IFF_UP | IFF_RUNNING | IFF_MULTICAST)) == (IFF_UP | IFF_RUNNING | IFF_MULTICAST))
            {
                // interface index
                const uint32_t idx = if_nametoindex(ifa->ifa_name);
                // IP4
                if (sa.sa_family == AF_INET) {
                    if (idx && ip4IntfIdx.insert(idx).second)  {    // not yet added an address of this interface
                        const sockaddr_in* pSaIn = reinterpret_cast<const sockaddr_in*>(ifa->ifa_addr);
                        gIp4IntfAddr.push_back(pSaIn->sin_addr);
                    }
                }
                // IP6
                else {
                    const sockaddr_in6* pSaIn6 = reinterpret_cast<const sockaddr_in6*>(ifa->ifa_addr);
                    gIp6IntfIdx.emplace(idx,pSaIn6->sin6_addr);                        // std::set will insert each index once only
                }
            }
        }
        
        // free the return list
        freeifaddrs(ifaddr);
#endif
    }
    return gaddrLocal;
}

/// Is given address a local one?
bool NetwIsLocalAddr (const InetAddrTy& addr)
{
    const std::vector<InetAddrTy>& loc = NetwGetLocalAddresses();
    return std::find(loc.cbegin(),loc.cend(),addr) != loc.cend();
}


} // namespace XPMP2
