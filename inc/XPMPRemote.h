/// @file       XPMPRemote.h
/// @brief      Semi-public remote network functionality for master/client operations
/// @details    Technically, this is public functionality from a library
///             point of view. But it is intended for the "XPMP Remote Client" only,
///             not for a standard plugin.\n
///             Network messages are packed for space efficiency, but also to avoid
///             layout differences between compilers/platforms.
///             However, manual layout tries to do reasonable alignment of numeric values
///             and 8-byte-alignment of each structure, so that arrays of structures
///             also align well.
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
#include <cmath>
#include <array>
#include <algorithm>

namespace XPMP2 {

//
// MARK: Global Helpers
//

/// @brief Produces a reproducible(!) hash value for strings
/// @details Result is the same if the same string is provided, across platform
///          and across executions, unlike what std::hash requires.\n
///          It is implemented as a 16-bit version of the PJW hash:
/// @see https://en.wikipedia.org/wiki/PJW_hash_function
std::uint16_t PJWHash16 (const char *s);

/// @brief Find a model by package name hash and short id
/// @details This approach is used by the remote client to safe network bandwith.
///          If an exact match with `pkgHash` _and_ `shortID` is not found,
///          then a model matching the short id alone is returned if available.
CSLModel* CSLModelByPkgShortId (std::uint16_t _pkgHash,
                                const char* _shortId, size_t _shortIdMaxSize);

//
// MARK: Network Data Definitions
//

// To ensure best network capacity usage as well as identical alignment across platform we enforce tightly packed structures.
// The approach is very different between Visual C++ and clang / gcc, though:
#ifdef _MSC_VER                                 // Visual C++
#pragma pack(push,1)                            // set packing once (ie. not per struct)
#define PACKED
#elif defined(__clang__) || defined(__GNUC__)   // Clang (Mac XCode etc) or GNU (Linux)
#define PACKED __attribute__ ((__packed__)) 
#else
#error Unhandled Compiler!
#endif

/// Message type
enum RemoteMsgTy : std::uint8_t {
    RMT_MSG_INTEREST_BEACON = 0,    ///< beacon sent by a remote client to signal interest in data
    RMT_MSG_SEND,                   ///< internal indicator telling to send out all pending messages
    RMT_MSG_SETTINGS,               ///< a sender's id and its settings
    RMT_MSG_AC_DETAILED,            ///< aircraft full details, needed to create new a/c objects and to re-synch all remote data
    RMT_MSG_AC_DIFF,                ///< aircraft differences only
    RMT_MSG_AC_ANIM,                ///< aircraft animation values (dataRef values) only
    RMT_MSG_AC_REMOVE,              ///< aircraft is removed
};

/// Definition for how to map dataRef values to (u)int8, ie. to an integer range of 8 bits
struct RemoteDataRefPackTy {
    float       minV  = 0.0f;       ///< minimum transferred value
    float       range = 0.0f;       ///< range of transferred value = maxV - minV

    /// Constructor sets minimum value and range
    RemoteDataRefPackTy (float _min, float _max) : minV(_min), range(_max - _min) { assert(range != 0.0f); }
    
    /// pack afloat value to integer
    std::uint8_t pack (float f) const   { return std::uint8_t(std::clamp<float>(f-minV,0.0f,range) * UINT8_MAX / range); }
    /// unpack an integer value to float
    float unpack (std::uint8_t i) const { return minV + range*float(i)/255.0f; }
};

/// An array holding all dataRef packing definitions
extern const std::array<RemoteDataRefPackTy,V_COUNT> REMOTE_DR_DEF;

/// Message header, identical for all message types
struct RemoteMsgBaseTy {
    RemoteMsgTy  msgTy  : 4;        ///< message type
    std::uint8_t msgVer : 4;        ///< message version
    std::uint8_t filler1   = 0;     ///< yet unsed
    std::uint16_t pluginId = 0;     ///< lower 16 bit of the sending plugin's id
    std::uint32_t filler2 = 0;      ///< yet unused, fills up to size 8
    /// Constructor just sets the values
    RemoteMsgBaseTy (RemoteMsgTy _ty, std::uint8_t _ver);
} PACKED;

/// Interest Beacon message version number
constexpr std::uint8_t RMT_VER_BEACON = 0;
/// "Beacon of Interest", ie. some message on the multicast just to wake up sender
struct RemoteMsgBeaconTy : public RemoteMsgBaseTy {
    // don't need additional data fields
    /// Constructor sets appropriate message type
    RemoteMsgBeaconTy();
} PACKED;

/// Setttings message version number
constexpr std::uint8_t RMT_VER_SETTINGS = 0;
/// Settings message, identifying a sending plugin, regularly providing its settings
struct RemoteMsgSettingsTy : public RemoteMsgBaseTy {
    char            name[16];           ///< plugin's name, not necessarily zero-terminated if using full 12 chars
    float           maxLabelDist;       ///< Maximum distance for drawing labels? [m]
    char            defaultIcao[4];     ///< Default ICAO aircraft type designator if no match can be found
    char            carIcaoType[4];     ///< Ground vehicle type identifier
    std::uint8_t logLvl             :3; ///< logging level
    bool bLogMdlMatch               :1; ///< Debug model matching?
    bool bObjReplDataRefs           :1; ///< Replace dataRefs in `.obj` files on load?
    bool bObjReplTextures           :1; ///< Replace textures in `.obj` files on load if needed?
    bool bLabelCutOffAtVisibility   :1; ///< Cut off labels at XP's reported visibility mit?
    bool bMapEnabled                :1; ///< Do we feed X-Plane's maps with our aircraft positions?
    bool bMapLabels                 :1; ///< Do we show labels with the aircraft icons?
    bool bHaveTCASControl           :1; ///< Do we have AI/TCAS control?
    std::uint16_t filler;               ///< yet unused, fills size up for a multiple of 8
    
    /// Constructor sets most values to zero
    RemoteMsgSettingsTy ();
    
} PACKED;

/// A/C detail message version number
constexpr std::uint8_t RMT_VER_AC_DETAIL = 0;
/// Number of dataRef values supported, it's rounded up to the next 8 bit boundary
constexpr size_t RMT_V_COUNT = XPMP2::V_COUNT + (XPMP2::V_COUNT%8 == 0 ? 0 : (8-XPMP2::V_COUNT%8) );
/// A/C details, packed into an array message
struct RemoteAcDetailTy {
    char            icaoType[4];        ///< icao a/c type
    char            icaoOp[4];          ///< icao airline code
    char            sShortId[20];       ///< CSL model's short id
    std::uint16_t   pkgHash;            ///< hash value of package name
    bool            bValid : 1;         ///< is this object valid? (Will be reset in case of exceptions)
    bool            bVisible : 1;       ///< Shall this plane be drawn at the moment?
    char            label[23];          ///< label
    std::uint8_t    modeS_id[3];        ///< plane's unique id at the sender side (might differ remotely in case of duplicates)
    std::uint8_t    labelCol[3];        ///< label color (RGB)
    float           alt_ft;             ///< [ft] current altitude
    // ^ the above has 64 bytes, so that these doubles start on an 8-byte bounday:
    double          lat;                ///< latitude
    double          lon;                ///< longitude
    std::int16_t    pitch;              ///< [0.01°] pitch/100
    std::uint16_t   heading;            ///< [0.01°] heading/100
    std::int16_t    roll;               ///< [0.01°] roll/100
    std::int16_t    aiPrio;             ///< priority for display in limited TCAS target slots, `-1` indicates "no TCAS display"
    ///< Array of _packed_ dataRef values for CSL model animation
    std::uint8_t    v[RMT_V_COUNT];

    /// Default Constructor sets all to zero
    RemoteAcDetailTy ();
    /// A/c copy constructor fills from passed-in XPMP2::Aircraft object
    RemoteAcDetailTy (const Aircraft& _ac);
    /// @brief Copies values from passed-in XPMP2::Aircraft object
    /// @note Main thread only! Uses XP SDK functions to convert coordinates
    void CopyFrom (const Aircraft& _ac);
    
    // Getters/Setters for the packed members
    void SetModeSId (XPMPPlaneID _id);          ///< set modeS_id
    XPMPPlaneID GetModeSId () const;            ///< retrieve modeS_id

    void SetLabelCol (const float _col[4]);     ///< set the label color from a float array (4th number, alpha, is always considered 1.0)
    void GetLabelCol (float _col[4]) const;     ///< writes color out into a float array
    
    void SetPitch (float _p) { pitch = std::int16_t(_p*100.0f); }  ///< sets pitch from float
    float GetPitch () const { return float(pitch) / 100.0f; }                   ///< returns float pitch

    /// @brief Sets heading from float
    /// @note Only works well for `0 <= _h < 360`
    void SetHeading (float _h);
    float GetHeading () const { return float(heading) / 100.0f; }               ///< returns float heading

    void SetRoll (float _r) { roll = std::int16_t(std::lround(_r*100.0f)); }  ///< sets pitch from float
    float GetRoll () const { return float(roll) / 100.0f; }                   ///< returns float pitch
} PACKED;

/// A/C detail message, has an inherited header plus an array of XPMP2::RemoteAcDetailTy elements
struct RemoteMsgAcDetailTy : public RemoteMsgBaseTy {
    RemoteAcDetailTy arr[1];                ///< basis for the array of actual details
    
    /// Constructor sets expected message type and version
    RemoteMsgAcDetailTy () : RemoteMsgBaseTy(RMT_MSG_AC_DETAILED, RMT_VER_AC_DETAIL) {}
    /// Convert msg len to number of arr elements
    static size_t NumElem (size_t _msgLen) { return (_msgLen - sizeof(RemoteMsgBaseTy)) / sizeof(RemoteAcDetailTy); }
} PACKED;

// TODO: Packed Diff A/C message
// TODO: A/C animation message

/// A/C detail message version number
constexpr std::uint8_t RMT_VER_AC_REMOVE = 0;
/// A/C detail message, with all full details of an aircraft, required to start drawing that aircraft
struct RemoteMsgAcRemoveTy : public RemoteMsgBaseTy {
    std::uint32_t   modeS_id : 24;      ///< plane's unique id at the sender side (might differ remotely in case of duplicates)
} PACKED;

#ifdef _MSC_VER                                 // Visual C++
#pragma pack(pop)                               // reseting packing
#endif

// A few static validations just to make sure that no compiler fiddles with my network message layout.
// Note that each individual structure size is a multiple of 8 for good array alignment.
static_assert(sizeof(RemoteMsgBaseTy)       ==   8,     "RemoteMsgBaseTy doesn't have expected size");
static_assert(sizeof(RemoteMsgSettingsTy)   ==  40,     "RemoteMsgSettingsTy doesn't have expected size");
static_assert(sizeof(RemoteAcDetailTy)      ==  88+48,  "RemoteAcDetailTy doesn't have expected size");
static_assert(sizeof(RemoteMsgAcDetailTy)   ==  96+48,  "RemoteMsgAcDetailTy doesn't have expected size");

//
// MARK: Miscellaneous
//

/// Function prototypes for callback functions to handle the received messages
struct RemoteCBFctTy {
    /// Called in flight loop before processing first aircraft
    void (*pfBeforeFirstAc)() = nullptr;
    /// Called in flight loop after processing last aircraft
    void (*pfAfterLastAc)() = nullptr;
    /// Callback for processing Settings messages
    void (*pfMsgSettings) (std::uint32_t from[4],
                           const std::string& sFrom,
                           const RemoteMsgSettingsTy&) = nullptr;
    /// Callback for processing A/C Details messages
    void (*pfMsgACDetails) (std::uint32_t from[4], size_t msgLen,
                            const RemoteMsgAcDetailTy&) = nullptr;
};

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
