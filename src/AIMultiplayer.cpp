/// @file       AIMultiplayer.cpp
/// @brief      Implementation of serving TCAS target dataRef updates
/// @details    Updates the up to 63 slots of X-Plane's defined TCAS targets,
///             which are in turn synced with the 19 "legacy" multiplayer slots,
///             which in turn many other plugins (like TCAS implementations or map tools)
///             read to learn about any multiplayer aircraft.\n
///             Also serves shared dataRefs for publishing text information,
///             which is not X-Plane standard but originally suggested by FSTramp.
///             However, the post and file, in which he suggested this addition,
///             is no longer available on forums.x-plane.org. Still, XPMP2
///             fullfills the earlier definition.
/// @see        X-Plane 11.50 Beta 8 introduced a new way of informing X-Plane
///             of TCAS targets, see here a blog entry for details:
///             https://developer.x-plane.com/article/overriding-tcas-and-providing-traffic-information/
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

#define INFO_AI_CONTROL         "Have control now over AI/Multiplayer planes"
#define INFO_AI_CONTROL_ENDS    "Released control of AI/Multiplayer planes"
#define WARN_NO_AI_CONTROL      "Could NOT acquire AI/Multiplayer plane control! %s controls them. Therefore, our planes will NOT appear on TCAS or maps."
#define DEBUG_AI_SLOT_ASSIGN    "Aircraft %llu: ASSIGNING AI Slot %d (%s, %s, %s)"
#define DEBUG_AI_SLOT_RELEASE   "Aircraft %llu: RELEASING AI Slot %d (%s, %s, %s)"

namespace XPMP2 {

//
// MARK: Handling of AI/multiplayer Planes for TCAS, maps, camera plugins...
//

/// Don't dare using NAN...but with this coordinate for x/y/z a plane should be far out and virtually invisible
constexpr float FAR_AWAY_VAL_GL = 9999999.9f;
/// How often do we reassign AI slots? [seconds]
constexpr float AISLOT_CHANGE_PERIOD = 10.0f;
/// How much distance does each AIPrio add?
constexpr int AI_PRIO_MULTIPLIER = 10 * M_per_NM;
/// A constant array of zero values supporting quick array initialization
float F_NULL[10] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

//
// MARK: Legacy multiplayer dataRefs
//

/// Keeps the dataRef handles for one of the up to 19 legacy AI/Multiplayer slots ("sim/multiplayer/position")
struct multiDataRefsTy {
    XPLMDataRef X = nullptr;            // position
    XPLMDataRef Y = nullptr;
    XPLMDataRef Z = nullptr;
    // all OK?
    inline operator bool () const { return X && Y && Z; }
};

/// Keeps the dataRef handles for one of the up to 63 shared data slots ("sim/multiplayer/position/plane#...")
struct infoDataRefsTy {
    // Shared data for providing textual info (see XPMPInfoTexts_t)
    XPLMDataRef infoTailNum         = nullptr;  // tailnum
    XPLMDataRef infoIcaoAcType      = nullptr;  // ICAO
    XPLMDataRef infoManufacturer    = nullptr;  // manufacturer
    XPLMDataRef infoModel           = nullptr;  // model
    XPLMDataRef infoIcaoAirline     = nullptr;  // ICAOairline
    XPLMDataRef infoAirline         = nullptr;  // airline
    XPLMDataRef infoFlightNum       = nullptr;  // flightnum
    XPLMDataRef infoAptFrom         = nullptr;  // apt_from
    XPLMDataRef infoAptTo           = nullptr;  // apt_to
};

//
// MARK: TCAS Target dataRefs
//

static XPLMDataRef drTcasOverride   = nullptr;      ///< sim/operation/override/override_TCAS               int
static XPLMDataRef drMapOverride    = nullptr;      ///< sim/operation/override/override_multiplayer_map_layer int
static XPLMDataRef drTcasModeS      = nullptr;      ///< sim/cockpit2/tcas/targets/modeS_id                 int[64]
static XPLMDataRef drTcasModeC      = nullptr;      ///< sim/cockpit2/tcas/targets/modeC_code               int[64]
static XPLMDataRef drTcasFlightId   = nullptr;      ///< sim/cockpit2/tcas/targets/flight_id                byte[512]
static XPLMDataRef drTcasIcaoType   = nullptr;      ///< sim/cockpit2/tcas/targets/icao_type                byte[512]
static XPLMDataRef drTcasX          = nullptr;      ///< sim/cockpit2/tcas/targets/position/x               float[64]
static XPLMDataRef drTcasY          = nullptr;      ///< sim/cockpit2/tcas/targets/position/y               float[64]
static XPLMDataRef drTcasZ          = nullptr;      ///< sim/cockpit2/tcas/targets/position/z               float[64]
static XPLMDataRef drTcasVX         = nullptr;      ///< sim/cockpit2/tcas/targets/position/vx              float[64]
static XPLMDataRef drTcasVY         = nullptr;      ///< sim/cockpit2/tcas/targets/position/vy              float[64]
static XPLMDataRef drTcasVZ         = nullptr;      ///< sim/cockpit2/tcas/targets/position/vz              float[64]
static XPLMDataRef drTcasVertSpeed  = nullptr;      ///< sim/cockpit2/tcas/targets/position/vertical_speed  float[64]
static XPLMDataRef drTcasHeading    = nullptr;      ///< sim/cockpit2/tcas/targets/position/psi             float[64]
static XPLMDataRef drTcasPitch      = nullptr;      ///< sim/cockpit2/tcas/targets/position/the             float[64]
static XPLMDataRef drTcasRoll       = nullptr;      ///< sim/cockpit2/tcas/targets/position/phi             float[64]
static XPLMDataRef drTcasGear       = nullptr;      ///< sim/cockpit2/tcas/targets/position/gear_deploy     float[64]
static XPLMDataRef drTcasFlap       = nullptr;      ///< sim/cockpit2/tcas/targets/position/flap_ratio      float[64]
static XPLMDataRef drTcasFlap2      = nullptr;      ///< sim/cockpit2/tcas/targets/position/flap_ratio2     float[64]
static XPLMDataRef drTcasSpeedbrake = nullptr;      ///< sim/cockpit2/tcas/targets/position/speedbrake_ratio float[64]
static XPLMDataRef drTcasSlat       = nullptr;      ///< sim/cockpit2/tcas/targets/position/slat_ratio      float[64]
static XPLMDataRef drTcasWingSweep  = nullptr;      ///< sim/cockpit2/tcas/targets/position/wing_sweep      float[64]
static XPLMDataRef drTcasThrottle   = nullptr;      ///< sim/cockpit2/tcas/targets/position/throttle        float[64]
static XPLMDataRef drTcasYokePitch  = nullptr;      ///< sim/cockpit2/tcas/targets/position/yolk_pitch [sic!]   float[64]
static XPLMDataRef drTcasYokeRoll   = nullptr;      ///< sim/cockpit2/tcas/targets/position/yolk_roll [sic!]    float[64]
static XPLMDataRef drTcasYokeYaw    = nullptr;      ///< sim/cockpit2/tcas/targets/position/yolk_yaw [sic!]     float[64]
static XPLMDataRef drTcasLights     = nullptr;      ///< sim/cockpit2/tcas/targets/position/lights          int[64] (bitfield: beacon=1, land=2, nav=4, strobe=8, taxi=16)

/// Arry size of sim/cockpit2/tcas/targets/modeS_id
static size_t numTcasTargets = 0;

/// A structure simplifying communicaton with sim/cockpit2/tcas/targets/position/lights
union TcasLightsTy {
    int i;                                          ///< the full integer value
    struct BitsTy {
        bool beacon : 1;                            ///< Beacon lights
        bool land   : 1;                            ///< Landing lights
        bool nav    : 1;                            ///< Navigation lights
        bool strobe : 1;                            ///< Strobe lights
        bool taxi   : 1;                            ///< Taxi lights
    } b;
};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
/// Keeps the dataRef handles for one of the up to 19 legacy AI/Multiplayer slots ("sim/multiplayer/position") (accepted as a global variable requiring an exit-time destructor)
static std::vector<multiDataRefsTy> gMultiRef;
/// Keeps the dataRef handles for one of the up to 63 shared data slots ("sim/multiplayer/position/plane#...") (accepted as a global variable requiring an exit-time destructor)
static std::vector<infoDataRefsTy>  gInfoRef;

/// Map of Aircrafts, sorted by (priority-biased) distance
typedef std::map<float,XPMPPlaneID> mapAcByDistTy;
/// Map of Aircrafts, sorted by (priority-biased) distance
mapAcByDistTy gMapAcByDist;
#pragma clang diagnostic pop

// Resets all actual values of the AI/multiplayer dataRefs of one plane to something initial
void AIMultiClearAIDataRefs (multiDataRefsTy& mdr, bool bDeactivateToZero = false);

//              Updates all TCAS target dataRefs, both standard X-Plane,
//              as well as additional shared dataRefs for text publishing
void AIMultiUpdate ()
{
    // If we don't control AI aircraft we bail out
    if (!XPMPHasControlOfAIAircraft())
        return;
    
    // Time of last slot switching activity
    static float tLastSlotSwitching = 0.0f;
    const float now = GetMiscNetwTime();
    // only every few seconds rearrange slots, ie. add/remove planes or
    // move planes between lower and upper section of AI slots:
    if (CheckEverySoOften(tLastSlotSwitching, AISLOT_CHANGE_PERIOD, now) ||
        glob.mapAc.size() != gMapAcByDist.size())
    {
        // Sort all planes by prioritized distance
        gMapAcByDist.clear();
        for (const auto& pair: glob.mapAc) {
            const Aircraft& ac = *pair.second;
            // only consider planes that require being shown as AI aircraft
            // (these excludes invisible planes and those with transponder off)
            if (ac.ShowAsAIPlane())
                // Priority distance means that we add artificial distance for higher-numbered AI priorities
                gMapAcByDist.emplace(ac.GetCameraDist() + ac.aiPrio * AI_PRIO_MULTIPLIER,
                                     ac.GetModeS_ID());
        }
    }
    
    // Start filling up TCAS targets, ordered by distance,
    // so that the closest planes are in the lower slots,
    // mirrored to the legacy multiplayer slots
    std::vector<int>   vModeS;      vModeS.reserve(gMapAcByDist.size());
    std::vector<int>   vModeC;      vModeC.reserve(gMapAcByDist.size());
    std::vector<float> vX;          vX.reserve(gMapAcByDist.size());
    std::vector<float> vY;          vY.reserve(gMapAcByDist.size());
    std::vector<float> vZ;          vZ.reserve(gMapAcByDist.size());
    std::vector<float> vVertSpeed;  vVertSpeed.reserve(gMapAcByDist.size());
    std::vector<float> vHeading;    vHeading.reserve(gMapAcByDist.size());
    std::vector<float> vPitch;      vPitch.reserve(gMapAcByDist.size());
    std::vector<float> vRoll;       vRoll.reserve(gMapAcByDist.size());
    std::vector<float> vGear;       vGear.reserve(gMapAcByDist.size());
    std::vector<float> vFlap;       vFlap.reserve(gMapAcByDist.size());
    std::vector<float> vSpeedbrake; vSpeedbrake.reserve(gMapAcByDist.size());
    std::vector<float> vSlat;       vSlat.reserve(gMapAcByDist.size());
    std::vector<float> vWingSweep;  vWingSweep.reserve(gMapAcByDist.size());
    std::vector<float> vThrottle;   vThrottle.reserve(gMapAcByDist.size());
    std::vector<float> vYokePitch;  vYokePitch.reserve(gMapAcByDist.size());
    std::vector<float> vYokeRoll;   vYokeRoll.reserve(gMapAcByDist.size());
    std::vector<float> vYokeYaw;    vYokeYaw.reserve(gMapAcByDist.size());
    std::vector<int>   vLights;     vLights.reserve(gMapAcByDist.size());

    for (const auto& p: gMapAcByDist)
    {
        // We keep the planeId in our distance-sorted map,
        // but the plane might be gone now, so be carefully looking for it!
        mapAcTy::const_iterator iterAc;
        iterAc = glob.mapAc.find(p.second);
        if (iterAc == glob.mapAc.end() ||       // not found any longer?
            !iterAc->second->ShowAsAIPlane())   // or no longer to be shown on TCAS?
        {
            tLastSlotSwitching = 0.f;           // ensure we reinit the map next time
            continue;
        }
        
        // Plane exist!
        Aircraft& ac = *iterAc->second;
        vModeS.push_back(int(ac.GetModeS_ID()));
        vModeC.push_back(int(ac.acRadar.code));
        
        // This plane's position
        vX.push_back(ac.drawInfo.x);
        vY.push_back(ac.drawInfo.y - ac.GetVertOfs());  // align with original altitude
        vZ.push_back(ac.drawInfo.z);
        
        // attitude
        vPitch.push_back(ac.drawInfo.pitch);
        vRoll.push_back(ac.drawInfo.roll);
        vHeading.push_back(ac.drawInfo.heading);
        
        // configuration
        vGear.push_back(ac.v[V_CONTROLS_GEAR_RATIO]);
        vFlap.push_back(ac.v[V_CONTROLS_FLAP_RATIO]);
        vSpeedbrake.push_back(ac.v[V_CONTROLS_SPEED_BRAKE_RATIO]);
        vSlat.push_back(ac.v[V_CONTROLS_SLAT_RATIO]);
        vWingSweep.push_back(ac.v[V_CONTROLS_WING_SWEEP_RATIO]);
        vThrottle.push_back(ac.v[V_CONTROLS_THRUST_RATIO]);
        
        // Yoke
        vYokePitch.push_back(ac.v[V_CONTROLS_YOKE_PITCH_RATIO]);
        vYokeRoll.push_back(ac.v[V_CONTROLS_YOKE_ROLL_RATIO]);
        vYokeYaw.push_back(ac.v[V_CONTROLS_YOKE_HEADING_RATIO]);
        
        // lights
        TcasLightsTy l = {0};
        l.b.beacon  = ac.v[V_CONTROLS_BEACON_LITES_ON] > 0.5f;
        l.b.land    = ac.v[V_CONTROLS_LANDING_LITES_ON] > 0.5f;
        l.b.nav     = ac.v[V_CONTROLS_NAV_LITES_ON] > 0.5f;
        l.b.strobe  = ac.v[V_CONTROLS_STROBE_LITES_ON] > 0.5f;
        l.b.taxi    = ac.v[V_CONTROLS_TAXI_LITES_ON] > 0.5f;
        vLights.push_back(l.i);
        
        // For performance reasons and because differences (cartesian velocity)
        // are smoother if calculated over "longer" time frames,
        // the following updates are done about every second only
        if (now >= ac.prev_ts + 1.0f)
        {
            // do we have any prev x/y/z values at all?
            if (ac.prev_ts > 0.0001f) {
                // yes, so we can calculate velocity
                const float d_t = now - ac.prev_ts;                 // time that had passed in seconds
                const float d_x = ac.drawInfo.x - ac.prev_x;
                const float d_y = ac.drawInfo.y - ac.prev_y;
                const float d_z = ac.drawInfo.z - ac.prev_z;
                float f = d_x / d_t;
                XPLMSetDatavf(drTcasVX, &f, (int)vModeS.size(), 1);
                f = d_y / d_t;
                XPLMSetDatavf(drTcasVY, &f, (int)vModeS.size(), 1);
                f = d_z / d_t;
                XPLMSetDatavf(drTcasVZ, &f, (int)vModeS.size(), 1);
                
                // vertical speed (roughly...y is not exact, but let's keep things simple here),
                // convert from m/s to ft/min
                f = (d_y / d_t) * (60.0f / float(M_per_FT));
                XPLMSetDatavf(drTcasVertSpeed, &f, (int)vModeS.size(), 1);
            }
            ac.prev_x = ac.drawInfo.x;
            ac.prev_y = ac.drawInfo.y;
            ac.prev_z = ac.drawInfo.z;
            ac.prev_ts = now;
            
            // Flight or tail number as FlightID
            char s[8];
            memset(s, 0, sizeof(s));
            memcpy(s,
                   ac.acInfoTexts.flightNum[0] ? ac.acInfoTexts.flightNum : ac.acInfoTexts.tailNum,
                   sizeof(s)-1);
            XPLMSetDatab(drTcasFlightId, s, (int)(vModeS.size() * sizeof(s)), sizeof(s));

            // Icao Type code
            memset(s, 0, sizeof(s));
            memcpy(s, ac.acInfoTexts.icaoAcType, sizeof(ac.acInfoTexts.icaoAcType));
            XPLMSetDatab(drTcasIcaoType, s, (int)(vModeS.size() * sizeof(s)), sizeof(s));

            // Shared data for providing textual info (see XPMPInfoTexts_t)
            const infoDataRefsTy drI = gInfoRef[vModeS.size()];
            XPLMSetDatab(drI.infoTailNum,       ac.acInfoTexts.tailNum,       0, sizeof(XPMPInfoTexts_t::tailNum));
            XPLMSetDatab(drI.infoIcaoAcType,    ac.acInfoTexts.icaoAcType,    0, sizeof(XPMPInfoTexts_t::icaoAcType));
            XPLMSetDatab(drI.infoManufacturer,  ac.acInfoTexts.manufacturer,  0, sizeof(XPMPInfoTexts_t::manufacturer));
            XPLMSetDatab(drI.infoModel,         ac.acInfoTexts.model,         0, sizeof(XPMPInfoTexts_t::model));
            XPLMSetDatab(drI.infoIcaoAirline,   ac.acInfoTexts.icaoAirline,   0, sizeof(XPMPInfoTexts_t::icaoAirline));
            XPLMSetDatab(drI.infoAirline,       ac.acInfoTexts.airline,       0, sizeof(XPMPInfoTexts_t::airline));
            XPLMSetDatab(drI.infoFlightNum,     ac.acInfoTexts.flightNum,     0, sizeof(XPMPInfoTexts_t::flightNum));
            XPLMSetDatab(drI.infoAptFrom,       ac.acInfoTexts.aptFrom,       0, sizeof(XPMPInfoTexts_t::aptFrom));
            XPLMSetDatab(drI.infoAptTo,         ac.acInfoTexts.aptTo,         0, sizeof(XPMPInfoTexts_t::aptTo));
        }

        // maximum number of TCAS planes defined? -> leave loop early, can't accomodate all planes
        if (vModeS.size() >= numTcasTargets)
            break;
    }
    
    // now we know how many planes we'll feed (+1 for the user's plane)
    XPLMSetActiveAircraftCount(1+(int)vX.size());
    
    // Feed the dataRefs to X-Plane for TCAS target tracking
#define SET_DR(ty, dr) XPLMSetData##ty(drTcas##dr, v##dr.data(), 1, (int)v##dr.size())
    SET_DR(vi, ModeS);
    SET_DR(vi, ModeC);
    SET_DR(vf, X);
    SET_DR(vf, Y);
    SET_DR(vf, Z);
    SET_DR(vf, VertSpeed);
    SET_DR(vf, Heading);
    SET_DR(vf, Pitch);
    SET_DR(vf, Roll);
    SET_DR(vf, Gear);
    SET_DR(vf, Flap);
    XPLMSetDatavf(drTcasFlap2, vFlap.data(), 1, (int)vFlap.size());
    SET_DR(vf, Speedbrake);
    SET_DR(vf, Slat);
    SET_DR(vf, WingSweep);
    SET_DR(vf, Throttle);
    SET_DR(vf, YokePitch);
    SET_DR(vf, YokeRoll);
    SET_DR(vf, YokeYaw);
    SET_DR(vi, Lights);

    // Cleanup unused legacy multiplayer datarefs
    for (size_t i = vX.size(); i < gMultiRef.size(); i++)
        AIMultiClearAIDataRefs(gMultiRef[i]);
}

//
// MARK: Control Functions
//

/// Resets all actual values of the AI/multiplayer dataRefs of one plane to something initial
void AIMultiClearAIDataRefs (multiDataRefsTy& drM,
                             bool bDeactivateToZero)
{
    // either a "far away" location or standard 0, which is, however, a valid location somewhere!
    XPLMSetDataf(drM.X, bDeactivateToZero ? 0.0f : FAR_AWAY_VAL_GL);
    XPLMSetDataf(drM.Y, bDeactivateToZero ? 0.0f : FAR_AWAY_VAL_GL);
    XPLMSetDataf(drM.Z, bDeactivateToZero ? 0.0f : FAR_AWAY_VAL_GL);
}

/// Clears the shared info dataRefs
void AIMultiClearInfoDataRefs (infoDataRefsTy& drI)
{
    // Shared data for providing textual info (see XPMPInfoTexts_t)
    char allNulls[100];
    memset (allNulls, 0, sizeof(allNulls));
    XPLMSetDatab(drI.infoTailNum,       allNulls, 0, sizeof(XPMPInfoTexts_t::tailNum));
    XPLMSetDatab(drI.infoIcaoAcType,    allNulls, 0, sizeof(XPMPInfoTexts_t::icaoAcType));
    XPLMSetDatab(drI.infoManufacturer,  allNulls, 0, sizeof(XPMPInfoTexts_t::manufacturer));
    XPLMSetDatab(drI.infoModel,         allNulls, 0, sizeof(XPMPInfoTexts_t::model));
    XPLMSetDatab(drI.infoIcaoAirline,   allNulls, 0, sizeof(XPMPInfoTexts_t::icaoAirline));
    XPLMSetDatab(drI.infoAirline,       allNulls, 0, sizeof(XPMPInfoTexts_t::airline));
    XPLMSetDatab(drI.infoFlightNum,     allNulls, 0, sizeof(XPMPInfoTexts_t::flightNum));
    XPLMSetDatab(drI.infoAptFrom,       allNulls, 0, sizeof(XPMPInfoTexts_t::aptFrom));
    XPLMSetDatab(drI.infoAptTo,         allNulls, 0, sizeof(XPMPInfoTexts_t::aptTo));
}

/// Clears the key (mode_s) of the TCAS target dataRefs
void AIMultiClearTcasDataRefs ()
{
    std::vector<int> nullArr (numTcasTargets, 0);
    XPLMSetDatavi(drTcasModeS, nullArr.data(), 1, (int)numTcasTargets);
}

/// Reset all (controlled) multiplayer dataRef values of all planes
void AIMultiInitAllDataRefs(bool bDeactivateToZero)
{
    if (XPMPHasControlOfAIAircraft()) {
        // TCAS target keys
        AIMultiClearTcasDataRefs();
        
        // Legacy multiplayer dataRefs
        for (multiDataRefsTy& drM: gMultiRef)
            AIMultiClearAIDataRefs(drM, bDeactivateToZero);
        
        // Shared info dataRefs
        for (infoDataRefsTy& drI: gInfoRef)
            AIMultiClearInfoDataRefs(drI);
    }
}


// Initialize the module
/// @details Fetches all dataRef handles for all dataRefs of all up to 19 AI/multiplayer slots
void AIMultiInit ()
{
    // *** TCAS Target dataRefs ***
    
    // Many of these will only be available with X-Plane 11.50b8 or later
    drTcasOverride      = XPLMFindDataRef("sim/operation/override/override_TCAS");
    drMapOverride       = XPLMFindDataRef("sim/operation/override/override_multiplayer_map_layer");

    // This is probably the most important one, serving as a key,
    // but for us also as an indicator if we are in the right XP version
    drTcasModeS         = XPLMFindDataRef("sim/cockpit2/tcas/targets/modeS_id");
    if (!drTcasModeS) return;           // we can stop here...not the right XP version
    // Let's establish array size (we shall not assume it is 64, though initially it is)
    numTcasTargets = (size_t)XPLMGetDatavi(drTcasModeS, nullptr, 0, 0);
    if (numTcasTargets < 2) {           // What???
        drTcasModeS = nullptr;          // We can't work with less than 2 TCAS target either (#0 is the user's plane!)
        return;
    }
    // Reduce by one: The user plane is off limits
    numTcasTargets--;
    
    // Now fetch all the other dataRefs...they should work
    drTcasModeC         = XPLMFindDataRef("sim/cockpit2/tcas/targets/modeC_code");
    drTcasFlightId      = XPLMFindDataRef("sim/cockpit2/tcas/targets/flight_id");
    drTcasIcaoType      = XPLMFindDataRef("sim/cockpit2/tcas/targets/icao_type");
    drTcasX             = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/x");
    drTcasY             = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/y");
    drTcasZ             = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/z");
    drTcasVX            = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/vx");
    drTcasVY            = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/vy");
    drTcasVZ            = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/vz");
    drTcasVertSpeed     = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/vertical_speed");
    drTcasHeading       = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/psi");
    drTcasPitch         = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/the");
    drTcasRoll          = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/phi");
    drTcasGear          = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/gear_deploy");
    drTcasFlap          = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/flap_ratio");
    drTcasFlap2         = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/flap_ratio2");
    drTcasSpeedbrake    = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/speedbrake_ratio");
    drTcasSlat          = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/slat_ratio");
    drTcasWingSweep     = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/wing_sweep");
    drTcasThrottle      = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/throttle");
    drTcasYokePitch     = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/yolk_pitch");
    drTcasYokeRoll      = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/yolk_roll");
    drTcasYokeYaw       = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/yolk_yaw");
    drTcasLights        = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/lights");
    
    // *** Legacy Multiplayer DataRefs ***
    
    // We need them only for proper initialization at the beginning
    // and when not filling all (19) slots
    char        buf[100];
    gMultiRef.clear();                      // just a safety measure against multi init
    multiDataRefsTy drM;                    // one set of dataRefs for one plane
    gMultiRef.push_back(drM);               // add an empty record at position 0 (user's plane)

    // Code for finding a standard X-Plane dataRef
#define FIND_PLANE_DR(membVar, dataRefTxt, PlaneNr)                         \
    snprintf(buf,sizeof(buf),"sim/multiplayer/position/plane%u_" dataRefTxt,PlaneNr);    \
    drM.membVar = XPLMFindDataRef(buf);

    // We don't know how many multiplayer planes there are - fetch as many as we can.
    // Loop over all possible AI/multiplayer slots
    for (unsigned n = 1; true; n++)
    {
        // position
        FIND_PLANE_DR(X, "x", n);
        FIND_PLANE_DR(Y, "y", n);
        FIND_PLANE_DR(Z, "z", n);
        if (!drM) break;                    // break out of loop once the slot doesn't exist
        gMultiRef.push_back(drM);
    }
    
    // *** Shared dataRefs for providing additional text info ***
    
    // While these had been defined to be 19 only,
    // we extend it to as many as there are TCAS targets
    gInfoRef.clear();
    infoDataRefsTy  drI;            // one set of dataRefs for one plane
    gInfoRef.push_back(drI);        // add an empty record at position 0 (user's plane)

    // Code for finding a non-standard shared dataRef for text information sharing
#define SHARE_PLANE_DR(membVar, dataRefTxt, PlaneNr)                        \
    snprintf(buf,sizeof(buf),"sim/multiplayer/position/plane%u_" dataRefTxt,PlaneNr);    \
    if (XPLMShareData(buf, xplmType_Data, NULL, NULL))                      \
        drI.membVar = XPLMFindDataRef(buf);                                   \
    else drI.membVar = NULL;
    
    // We add as many shared info dataRefs as there are TCAS targets (probably 63)
    for (unsigned n = 1; n <= numTcasTargets; n++)
    {
        // Shared data for providing textual info (see XPMPInfoTexts_t)
        SHARE_PLANE_DR(infoTailNum,         "tailnum",              n);
        SHARE_PLANE_DR(infoIcaoAcType,      "ICAO",                 n);
        SHARE_PLANE_DR(infoManufacturer,    "manufacturer",         n);
        SHARE_PLANE_DR(infoModel,           "model",                n);
        SHARE_PLANE_DR(infoIcaoAirline,     "ICAOairline",          n);
        SHARE_PLANE_DR(infoAirline,         "airline",              n);
        SHARE_PLANE_DR(infoFlightNum,       "flightnum",            n);
        SHARE_PLANE_DR(infoAptFrom,         "apt_from",             n);
        SHARE_PLANE_DR(infoAptTo,           "apt_to",               n);
        gInfoRef.push_back(drI);
    }
}

// Grace cleanup
/// @details Make sure we aren't in control, then Unshare the shared dataRefs
void AIMultiCleanup ()
{
    // Make sure we are no longer in control
    XPMPMultiplayerDisable();
    
    // Unshare shared data
#define UNSHARE_PLANE_DR(membVar, dataRefTxt, PlaneNr)                         \
    snprintf(buf,sizeof(buf),"sim/multiplayer/position/plane%u_" dataRefTxt,PlaneNr);       \
    XPLMUnshareData(buf, xplmType_Data, NULL, NULL);                           \
    drI.membVar = NULL;
    
    char        buf[100];
    for (unsigned n = 1; n <= gInfoRef.size(); n++)
    {
        infoDataRefsTy& drI = gInfoRef[(size_t)n-1];
        UNSHARE_PLANE_DR(infoTailNum,         "tailnum",        n);
        UNSHARE_PLANE_DR(infoIcaoAcType,      "ICAO",           n);
        UNSHARE_PLANE_DR(infoManufacturer,    "manufacturer",   n);
        UNSHARE_PLANE_DR(infoModel,           "model",          n);
        UNSHARE_PLANE_DR(infoIcaoAirline,     "ICAOairline",    n);
        UNSHARE_PLANE_DR(infoAirline,         "airline",        n);
        UNSHARE_PLANE_DR(infoFlightNum,       "flightnum",      n);
        UNSHARE_PLANE_DR(infoAptFrom,         "apt_from",       n);
        UNSHARE_PLANE_DR(infoAptTo,           "apt_to",         n);
    }
    gInfoRef.clear();
    
    // Cleanup other arrays properly before shutdown
    gMultiRef.clear();
}

}  // namespace XPMP2

//
// MARK: General API functions outside XPMP2 namespace
//

using namespace XPMP2;

// Acquire control of multiplayer aircraft
const char *    XPMPMultiplayerEnable()
{
    // We totally rely on the new "TCAS target" functionality,
    // which is only available as of X-Plane 11 beta 8
    if (!drTcasModeS)
        return "TCAS requires XP11.50b8 or later";
    
    // short-cut if we are already in control
    if (XPMPHasControlOfAIAircraft())
        return "";
    
    // Attempt to grab multiplayer planes, then analyze.
    glob.bHasControlOfAIAircraft = XPLMAcquirePlanes(NULL, NULL, NULL) != 0;
    if (glob.bHasControlOfAIAircraft)
    {
        // We definitely want to override TCAS and map!
        XPLMSetDatai(drTcasOverride, 1);
        XPLMSetDatai(drMapOverride, 1);

        // No Planes yet started, initialize all dataRef values for a clean start
        XPLMSetActiveAircraftCount(1);
        AIMultiInitAllDataRefs(false);

        // Success
        LOG_MSG(logINFO, INFO_AI_CONTROL);
        return "";
    }
    else
    {
        // Failed! Because of whom?
        int total=0, active=0;
        XPLMPluginID who=0;
        char whoName[256];
        XPLMCountAircraft(&total, &active, &who);
        XPLMGetPluginInfo(who, whoName, nullptr, nullptr, nullptr);
        
        // Write a proper message and return it also to caller
        static char szWarn[256];
        snprintf(szWarn, sizeof(szWarn), WARN_NO_AI_CONTROL, whoName);
        LOG_MSG(logWARN, szWarn);
        return szWarn;
    }
}

// Release control of multiplayer aircraft
void XPMPMultiplayerDisable()
{
    // short-cut if we are not in control
    if (!XPMPHasControlOfAIAircraft())
        return;
    
    // Cleanup our values
    XPLMSetActiveAircraftCount(1);      // no active AI plane
    AIMultiInitAllDataRefs(true);       // reset all dataRef values to zero
    
    // Reset the last index used of all planes
    for (auto& pair: glob.mapAc)
        pair.second->ResetTcasTargetIdx();

    // Then fully release AI/multiplayer planes
    XPLMSetDatai(drMapOverride, 0);
    XPLMSetDatai(drTcasOverride, 0);
    XPLMReleasePlanes();
    glob.bHasControlOfAIAircraft = false;
    LOG_MSG(logINFO, INFO_AI_CONTROL_ENDS);
}

// Do we control AI planes?
bool XPMPHasControlOfAIAircraft()
{
    return glob.bHasControlOfAIAircraft;
}

