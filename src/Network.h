/// @file       Network.h
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

#ifndef Network_h
#define Network_h

#include <sys/types.h>
#if IBM
#include <winsock2.h>
#include <ws2ipdef.h>           // required for sockaddr_in6 (?)
#include <iphlpapi.h>           // for GetAdaptersAddresses
#include <MSWSock.h>            // for LPFN_WSARECVMSG 
#else
#define __APPLE_USE_RFC_3542    // both required for newer constants like IPV6_RECVPKTINFO
#define _GNU_SOURCE
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#endif
#include <stdexcept>

namespace XPMP2 {

#if IBM != 1
typedef int SOCKET;             ///< Windows defines SOCKET, so we define it for non-Windows manually
constexpr SOCKET INVALID_SOCKET = -1;
#endif

/// Helper definition for all these IPv4/6 differences
struct SockAddrTy
{
    union {
        sockaddr        sa;             // unspecific
        sockaddr_in     sa_in;          // AF_INET  / IPv4
        sockaddr_in6    sa_in6;         // AF_INET6 / IPv6
    };
    
    /// Constructor zeroes out everying
    SockAddrTy() { memset(this, 0, sizeof(SockAddrTy)); }
    /// Constructor copies given socket info
    SockAddrTy (const sockaddr* pSa);
    
    uint8_t family() const { return (uint8_t) sa.sa_family; }      ///< return the protocol famaily
    bool isIp4() const { return family() == AF_INET; }                  ///< is this an IPv4 address?
    bool isIp6() const { return family() == AF_INET6; }                 ///< is this an IPv6 address?
    socklen_t size () const                                             ///< expected structure size as per protocol family
    { return
        sa.sa_family == AF_INET     ? sizeof(sa_in) :
        sa.sa_family == AF_INET6    ? sizeof(sa_in6) : sizeof(sa); }
};

/// @brief Exception raised by XPMP2::SocketNetworking objects
/// @details This exception is raised when the address
///          and port combinaison cannot be resolved or if the socket cannot be
///          opened.
class NetRuntimeError : public std::runtime_error
{
public:
    std::string errTxt;             ///< OS text for what `errno` says, output of strerror_s()
    std::string fullWhat;           ///< combines `w` and `errTxt`
    NetRuntimeError(const std::string& w); ///< Constructor sets the above texts
    /// Return the full message, ie. `fullWhat`
    const char* what() const noexcept override { return fullWhat.c_str(); }
};

/// @brief Base class for any socket-based networking
/// @exception XPMP2::NetRuntimeError in case of any errors
class SocketNetworking
{
protected:
    SOCKET              f_socket        = INVALID_SOCKET;
    int                 f_port          = 0;
    std::string         f_addr;
    
    // the data receive buffer
    char*               buf             = NULL;
    size_t              bufSize         = 512;

#if IBM
    /// Pointer to WSARecvMsg function of Winsock2        
    LPFN_WSARECVMSG     pWSARecvMsg     = nullptr;
#endif
public:
    /// Default constructor is not doing anything
    SocketNetworking() {}
    /// Constructor creates a socket and binds it to the given address
    SocketNetworking(const std::string& _addr, int _port, size_t _bufSize = 512,
                     unsigned _timeOut_ms = 0, bool _bBroadcast = false);
    /// Destructor makes sure the socket is closed
    virtual ~SocketNetworking();

    /// Creates a socket and binds it to the given local address
    virtual void Open(const std::string& _addr, int _port, size_t _bufSize = 512,
                      unsigned _timeOut_ms = 0, bool _bBroadcast = false);
    /// Creates a socket and connects it to the given remote server
    virtual void Connect(const std::string& _addr, int _port, size_t _bufSize,
                         unsigned _timeOut_ms = 0);
    /// Thread-safely close the connection(s) and frees the buffer
    virtual void Close();
    /// Is a socket open?
    inline bool isOpen() const { return (f_socket != INVALID_SOCKET); }
    /// (Re)Sets the buffer size (or clears it if `_bufSize==0`)
    void SetBufSize (size_t _bufSize);
    /// Returns a human-readable text for the last error
    static std::string GetLastErr();
    
    // attribute access
    SOCKET       getSocket() const   { return f_socket; }   ///< the socket
    int          getPort() const     { return f_port; }     ///< the port
    std::string  getAddr() const     { return f_addr; }     ///< the interface address

    /// return the buffer
    const char* getBuf () const  { return buf ? buf : ""; }
    
    /// @brief Set blocking mode
    void setBlocking (bool bBlock);
    
    /// Waits to receive a message, ensures zero-termination in the buffer
    long                recv();
    /// Waits to receive a message with timeout, ensures zero-termination in the buffer
    long                timedRecv(int max_wait_ms);
    
    /// Sends a broadcast message
    bool broadcast (const char* msg);
    
    /// Convert addresses to string
    static std::string GetAddrString (const SockAddrTy& sa, bool withPort = true);
    
protected:
    /// Subclass to tell which addresses to look for
    virtual void GetAddrHints (struct addrinfo& hints) = 0;
#if IBM
    /// @brief Wrapper around Windows' weird way of hiding WSAREcvMsg: Finds the pointer to the function, then calls it
    /// @see https://learn.microsoft.com/en-us/windows/win32/api/mswsock/nc-mswsock-lpfn_wsarecvmsg
    /// @see https://stackoverflow.com/a/37334943
    int WSARecvMsg(
        LPWSAMSG lpMsg,
        LPDWORD lpdwNumberOfBytesRecvd,
        LPWSAOVERLAPPED lpOverlapped,
        LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
#endif
};


/// Receives UDP messages
class UDPReceiver : public SocketNetworking
{
public:
    /// Default constructor is not doing anything
    UDPReceiver() : SocketNetworking() {}
    /// Constructor creates a socket and binds it to the given address
    UDPReceiver(const std::string& _addr, int _port, size_t _bufSize = 512,
                unsigned _timeOut_ms = 0) :
        SocketNetworking(_addr,_port,_bufSize,_timeOut_ms) {}
    
protected:
    /// Sets flags to AI_PASSIVE, AF_INET, SOCK_DGRAM, IPPROTO_UDP
    virtual void GetAddrHints (struct addrinfo& hints);
};


/// @brief UDP Multicast, always binding to INADDR_ANY
/// @exception XPMP2::NetRuntimeError in case of any errors
class UDPMulticast : public SocketNetworking
{
protected:
    std::string multicastAddr;          ///< the multicast address
    struct addrinfo* pMCAddr = nullptr; ///< the multicast address
    
    bool bSendToAll = true;             ///< Send out multicast datagrams to all interfaces
    uint32_t oneIntfIdx = 0;            ///< When sending to one interface only, which one?

public:
    /// Default constructor is not doing anything
    UDPMulticast() : SocketNetworking() {}
    /// @brief Constructor creates a socket and binds it to INADDR_ANY and connects to the given multicast address
    UDPMulticast(const std::string& _multicastAddr, int _port, int _ttl=8,
                 size_t _bufSize = 512, unsigned _timeOut_ms = 0);
    /// makes sure pMCAddr is cleared up
    virtual ~UDPMulticast();
    
    /// Return formatted multicast address, including port
    const std::string& GetMCAddr() const { return multicastAddr; }
    
    /// @brief Connect to the multicast group
    /// @exception XPMP2::NetRuntimeError in case of any errors
    void Join (const std::string& _multicastAddr, int _port, int _ttl=8,
               size_t _bufSize = 512, unsigned _timeOut_ms = 0);
    
    /// Protocol family
    uint8_t GetFamily () const { return pMCAddr ? (uint8_t)pMCAddr->ai_family : AF_UNSPEC; }
    bool IsIPv4 () const { return GetFamily() == AF_INET; }     ///< IPv4?
    bool IsIPv6 () const { return GetFamily() == AF_INET6; }    ///< IPv6?
    
    /// Send future datagrams on default interfaces only (default)
    void SendToDefault ();
    /// Send future datagrams on _all_ interfaces
    void SendToAll ();
    /// Send future datagrams on this particular interface only
    void SendToIntf (uint32_t idx);

    /// @brief Send a multicast
    /// @param _bufSend Data to send
    /// @param _bufSendSize Size of the provided buffer
    /// @return Number of bytes sent
    size_t SendMC (const void* _bufSend, size_t _bufSendSize);
    
    /// @brief Receive a multicast, received message is in XPMP2::SocketNetworking::GetBuf()
    /// @param bSwitchToRecvIntf Shall all future datagrams be _send_ out on the same interface as we received this message on?
    /// @param[out] _pFromAddr If given then the sender adress is written into this string
    /// @param[out] _pFromSockAddr If given then the sender adress is written into this string
    /// @param[out] _pbIntfChanged is set to `true` if the sending interface has been changed based on the received datagram
    /// @return Number of bytes received, `0` in case no data was waiting
    size_t RecvMC (bool bSwitchToRecvIntf,
                   std::string* _pFromAddr  = nullptr,
                   SockAddrTy* _pFromSockAddr = nullptr,
                   bool* _pbIntfChanged = nullptr);

protected:
    void Cleanup ();                    ///< frees pMCAddr
    /// returns information from `*pMCAddr`
    void GetAddrHints (struct addrinfo& hints) override;
    /// Set multicast send interface (IPv4 only)
    void SetSendInterface (const in_addr& addr);
    /// Set multicast send interface
    void SetSendInterface (uint32_t ifIdx);
};


/// Listens to TCP connections and opens a session socket upon connect
class TCPConnection : public SocketNetworking
{
protected:
    SOCKET              f_session_socket = INVALID_SOCKET;  ///< session socket, ie. the socket opened when a counterparty connects
    struct sockaddr_in  f_session_addr = {};                ///< address of the connecting counterparty
#if APL == 1 || LIN == 1
    /// the self-pipe to shut down the TCP listener gracefully
    SOCKET selfPipe[2] = { INVALID_SOCKET, INVALID_SOCKET };
#endif

public:
    /// Default constructor is not doing anything
    TCPConnection() : SocketNetworking() {}
    /// Constructor creates a socket and binds it to the given address
    TCPConnection(const std::string& _addr, int _port, size_t _bufSize = 512,
                  unsigned _timeOut_ms = 0) :
        SocketNetworking(_addr,_port,_bufSize,_timeOut_ms) {}
    
    virtual void Close();       ///< also close session connection
    void CloseListenerOnly();   ///< only closes the listening socket, but not a connected session

    void listen (int numConnections = 1);       ///< listen for incoming connections
    bool accept (bool bUnlisten = false);       ///< accept an incoming connections, optinally stop listening
    bool listenAccept (int numConnections = 1); ///< combines listening and accepting
    
    /// Connected to a counterparty?
    bool IsConnected () const { return f_session_socket != INVALID_SOCKET; };
    
    /// send messages on session connection
    bool send(const char* msg);

protected:
    virtual void GetAddrHints (struct addrinfo& hints);
};


/// Numerical IP address, used for both ipv4 and ipv6, for ease of handling
struct InetAddrTy {
    union {
        std::uint32_t   addr[4] = {0,0,0,0};
        struct in_addr  in_addr;
        struct in6_addr in6_addr;
    };
    
    /// Default does nothing
    InetAddrTy () {}
    /// Take over from structure
    InetAddrTy (const SockAddrTy& sa) { CopyFrom(sa); }
    /// Take over from structure
    InetAddrTy (const sockaddr* pSa) { CopyFrom(SockAddrTy(pSa)); }

    /// Take over address from structure
    void CopyFrom (const SockAddrTy& sa);

    /// Equality means: all elements are equal
    bool operator==(const InetAddrTy& o) const
    { return std::memcmp(this, &o, sizeof(*this)) == 0; }
    
    /// Comparison also uses memcmp for defined order
    bool operator<(const InetAddrTy& o) const
    { return std::memcmp(this, &o, sizeof(*this)) < 0; }

};

/// Is given address a local one?
bool NetwIsLocalAddr (const InetAddrTy& addr);
/// Is given address a local one?
inline bool NetwIsLocalAddr (const SockAddrTy& sa)
{ return NetwIsLocalAddr(InetAddrTy(sa)); }
/// Is given address a local one?
inline bool NetwIsLocalAddr (const sockaddr* pSa)
{ return NetwIsLocalAddr(InetAddrTy(pSa)); }


} // namespace XPMP2

#endif /* Network_h */
