/// @file       AIMultiplayer.cpp
/// @brief      Implementation of serving multiplayer dataRef updates
/// @details    Updates the up to 19 slots of X-Plane's defined AI aircraft,
///             which in turn many other plugins (like TCAS implementations or map tools)
///             read to learn about any multiplayer aircraft.\n
///             Also serves shared dataRefs for publishing text information,
///             which is not X-Plane standard but originally suggested by FSTramp.
///             However, the post and file, in which he suggested this addition,
///             is no longer available on forums.x-plane.org. Still, XPMP2
///             fullfills the earlier definition.
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

#define WARN_NO_AI_CONTROL      "Could NOT acquire AI/Multiplayer plane control! Some other multiplayer plugin controls them. Therefore, our planes will NOT appear on TCAS or maps."
#define DEBUG_AI_SLOT_ASSIGN    "Aircraft %llu: ASSIGNING AI Slot %d (%s, %s, %s)"
#define DEBUG_AI_SLOT_RELEASE   "Aircraft %llu: RELEASING AI Slot %d (%s, %s, %s)"

namespace XPMP2 {

//
// MARK: Handling of AI/multiplayer Planes for TCAS, maps, camera plugins...
//

/// Don't dare using NAN...but with this coordinate for x/y/z a plane should be far out and virtually invisible
constexpr float FAR_AWAY_VAL_GL = 9999999.9f;
/// How often do we reassign AI slots? [seconds]
constexpr int AISLOT_CHANGE_PERIOD = 30;
/// How much distance does each AIPrio add?
constexpr int AI_PRIO_MULTIPLIER = 10 * M_per_NM;
/// A constant array of zero values supporting quick array initialization
float F_NULL[10] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

/// Keeps the dataRef handles for one of the up to 19 AI/Multiplayer slots
struct multiDataRefsTy {
    XPLMDataRef X;          // position
    XPLMDataRef Y;
    XPLMDataRef Z;
    
    XPLMDataRef v_x;        // cartesian velocities
    XPLMDataRef v_y;
    XPLMDataRef v_z;
    
    XPLMDataRef pitch;      // theta
    XPLMDataRef roll;       // phi
    XPLMDataRef heading;    // psi
    
    XPLMDataRef gear;       // gear_deploy[10]
    XPLMDataRef flap;       // flap_ratio
    XPLMDataRef flap2;      // flap_ratio2
    XPLMDataRef spoiler;    // spoiler_ratio
    XPLMDataRef speedbrake; // speedbrake
    XPLMDataRef slat;       // slat_ratio
    XPLMDataRef wingSweep;  // wing_sweep
    XPLMDataRef throttle;   // throttle[8]
    XPLMDataRef yoke_pitch; // yolk_pitch
    XPLMDataRef yoke_roll;  // yolk_roll
    XPLMDataRef yoke_yaw;   // yolk_yaw
    
    XPLMDataRef bcnLights;  // beacon_lights_on
    XPLMDataRef landLights; // landing_lights_on
    XPLMDataRef navLights;  // nav_lights_on
    XPLMDataRef strbLights; // strobe_lights_on
    XPLMDataRef taxiLights; // taxi_light_on
    
    // Shared data for providing textual info (see XPMPInfoTexts_t)
    XPLMDataRef infoTailNum;        // tailnum
    XPLMDataRef infoIcaoAcType;     // ICAO
    XPLMDataRef infoManufacturer;   // manufacturer
    XPLMDataRef infoModel;          // model
    XPLMDataRef infoIcaoAirline;    // ICAOairline
    XPLMDataRef infoAirline;        // airline
    XPLMDataRef infoFlightNum;      // flightnum
    XPLMDataRef infoAptFrom;        // apt_from
    XPLMDataRef infoAptTo;          // apt_to
    
    bool        bSlotTaken = false; // during drawing: is this multiplayer plane idx used or not?
    
    // all OK?
    inline operator bool () const { return X && Y && Z && pitch && roll && heading; }
};

/// Keeps the dataRef handles for all of the up to 19 AI/Multiplayer slots
std::vector<multiDataRefsTy>        gMultiRef;

/// Map of Aircrafts, sorted by (priority-biased) distance
typedef std::map<float,Aircraft&> mapAcByDistTy;

// Resets all actual values of the AI/multiplayer dataRefs of one plane to something initial
void AIMultiClearDataRefs (multiDataRefsTy& mdr);

// Find and reserve an AI slot for the given aircraft
int Aircraft::AISlotReserve ()
{
    // safeguard: already have a slot?
    if (multiIdx >= 0)
        return multiIdx;
    
    // search for a free slot
    for (size_t idx = 0; idx < gMultiRef.size(); idx++)
        if (!gMultiRef[idx].bSlotTaken) {
            // found one: take the slot
            gMultiRef[idx].bSlotTaken = true;
            multiIdx = (int)idx;
            break;
        }
    
    // Debug output
    if (multiIdx >= 0)
    {
        LOG_MSG(logDEBUG, DEBUG_AI_SLOT_ASSIGN, (long long unsigned)mPlane,
                multiIdx+1,
                acIcaoType.c_str(), acIcaoAirline.c_str(), acLivery.c_str());
    }
    
    // return the found slot
    return multiIdx;
}

// Clear the AI slot
void Aircraft::AISlotClear ()
{
    // Debug output
    if (multiIdx >= 0)
    {
        LOG_MSG(logDEBUG, DEBUG_AI_SLOT_RELEASE, (long long unsigned)mPlane,
                multiIdx+1,
                acIcaoType.c_str(), acIcaoAirline.c_str(), acLivery.c_str());
    }

    // Just remove our knowledge of the slot
    multiIdx = -1;
}

//              Updates all multiplayer dataRefs, both standard X-Plane,
//              as well as additional shared dataRefs for text publishing
/// @details    We want a plane to keep its index as long as possible. This eases
///             following it from other plugins.\n
///             The plane's multiplayer idx is in Aircraft::multiIdx.\n
///             There are usually 19 slots for multiplayer planes.
///             But the user might have set up less actual AI planes.
///             Some plugins (including XP's own map) consider this number,
///             others don't.\n
///             Our approach: We make sure the first `modelCount` slots (lower part)
///             are used by the closest a/c. The others (upper part) fill up with those
///             father away.\n
///             That certainly means that a/c still might switch AI slots
///             if they move from lower to upper part or vice versa.
///             To avoid too fast/too often switching of slots we allow change of slots
///             only every 30s.
void AIMultiUpdate ()
{
    // If we don't control AI aircraft we bail out
    if (!XPMPHasControlOfAIAircraft())
        return;
    
    // `numAIPlanes` is the number of AI aircraft _configured_ by the user in X-Plane's AI-Planes settings
    int numAIPlanes, active, plugin;
    XPLMCountAircraft(&numAIPlanes, &active, &plugin);
    numAIPlanes--;               // first one's the user plane, here we don't count that

    // reset our bookkeeping on used multiplay idx
    for (multiDataRefsTy& iter: gMultiRef)
        iter.bSlotTaken = false;
    
    // Time of last slot switching activity
    static std::chrono::steady_clock::time_point tLastSlotSwitching;
    const std::chrono::steady_clock::time_point tNow = std::chrono::steady_clock::now();
    // only every few seconds rearrange slots, ie. add/remove planes or
    // move planes between lower and upper section of AI slots:
    if (tNow - tLastSlotSwitching > std::chrono::seconds(AISLOT_CHANGE_PERIOD))
    {
        tLastSlotSwitching = tNow;
        
        // Sort all planes by prioritized distance
        mapAcByDistTy mapAcByDist;
        for (const auto& pair: glob.mapAc) {
            Aircraft& ac = *pair.second;
            // only consider planes that require being shown as AI aircraft
            // (these excludes invisible planes and those with transponder off)
            if (ac.ShowAsAIPlane())
                // Priority distance means that we add artificial distance for higher-numbered AI priorities
                mapAcByDist.emplace(ac.GetDistToCamera() + ac.aiPrio * AI_PRIO_MULTIPLIER,
                                    ac);
        }
    
        // First loop over all relevant planes in order of prio/distance and
        // set those multiplayer idx used, which are taken,
        // and free up those slots no longer taken
        size_t numTcasPlanes = 0;      // counts TCAS-relevant planes
        for (const auto& pair: mapAcByDist)
        {
            // the multiplayer index stored with the plane:
            const int refIdx = pair.second.GetMultiIdx();

            // Still AI slots available?
            if (numTcasPlanes < gMultiRef.size())
            {
                // The lower part (close planes), shall display in the safe lower part of AI slots
                if (numTcasPlanes < (size_t)numAIPlanes)
                {
                    // current plane MUST get a slot in this range
                    if (0 <= refIdx && refIdx < numAIPlanes && !gMultiRef[(size_t)refIdx].bSlotTaken)
                        // it already has a reserved one, good
                        gMultiRef[(size_t)refIdx].bSlotTaken = true;
                    else
                        // it will get one later (we still might need to free up lower part slots)
                        pair.second.AISlotClear();
                } else {
                    // Planes father away (upper part):
                    // If the plane has a reserved slot in the upper part it keeps it
                    if (numAIPlanes <= refIdx && (size_t)refIdx < gMultiRef.size() && !gMultiRef[(size_t)refIdx].bSlotTaken)
                        gMultiRef[(size_t)refIdx].bSlotTaken = true;
                    else
                        // Otherwise free it (this might free up lower part slots)
                        pair.second.AISlotClear();
                }

                // one more plane, that requires an AI slot
                numTcasPlanes++;
            }
            // Ran out of AI slots
            else
                pair.second.AISlotClear();
        }           // loop all planes
        
        // Second loop: Now that we freed up some slots make sure
        //              that the first 19 planes do have slots
        numTcasPlanes = 0;
        for (const auto& pair: mapAcByDist)
        {
            // make sure the plane has or gets a slot
            pair.second.AISlotReserve();
            
            // Didn't get one??? That should not happen!
            if (pair.second.GetMultiIdx() < 0)
                LOG_MSG(logDEBUG, "Aircraft %llu: ERROR, got no AI slot, a/c ranks number %d with distance %.1f",
                        (long long unsigned)pair.second.GetPlaneID(), numTcasPlanes,
                        pair.second.GetDistToCamera());
            
            // exit once we have processed the highest priority planes,
            // as many as there are AI slots in total
            if (++numTcasPlanes >= gMultiRef.size())
                break;
        }
    }               // if time for AI slot switching
    else
    {
        // only quickly do bookkeeping on used slots
        // (even without re-slotting above, planes can have just vanished)
        for (const auto& pair: glob.mapAc) {
            const int idx = pair.second->GetMultiIdx();
            if (0 <= idx && (size_t)idx < gMultiRef.size())
                gMultiRef[(size_t)idx].bSlotTaken = true;
        }
    }
        
    // Fill multiplayer dataRefs for the planes relevant for TCAS
    size_t multiCount = 0;      // number of dataRefs set, just to exit loop early
    size_t maxMultiIdxUsed = 0; // maximum AI index used
    
    for (const auto& pair: glob.mapAc) {
        Aircraft& ac = *pair.second;
        
        // skip planes not having an AI slot
        if (ac.GetMultiIdx() < 0)
            continue;
        const size_t multiIdx = (size_t)ac.GetMultiIdx();
        // should not happen...but we don't want runtime errors either
        if (multiIdx >= gMultiRef.size()) {
            LOG_MSG(logDEBUG, "Aircraft %llu: ERROR, too high an AI slot: %d",
                    (long long unsigned)ac.GetPlaneID(),
                    ac.GetMultiIdx());
            continue;
        }
        
        // this slot taken (safety measure...should not really be needed)
        gMultiRef[multiIdx].bSlotTaken = true;
        const multiDataRefsTy& mdr = gMultiRef[multiIdx];

        // TODO: Verify we provide the correct position here, e.g. with ABC
        // HACK to reduce jitter in external camera applications:
        // The camera app's callback to retrieve camera position is called
        // _before_ any drawing happens. So what we do here is to provide
        // the _next_ position for the camera callback to retrieve before
        // the _next_ cycle.
        // This is one frame off of what we are drawing. Still _very_ close ;)
        XPLMSetDataf(mdr.X, ac.drawInfo.x);
        XPLMSetDataf(mdr.Y, ac.drawInfo.y);
        XPLMSetDataf(mdr.Z, ac.drawInfo.z);
        // attitude
        // TODO: Might also be taken from "next pos"
        XPLMSetDataf(mdr.pitch,   ac.drawInfo.pitch);
        XPLMSetDataf(mdr.roll,    ac.drawInfo.roll);
        XPLMSetDataf(mdr.heading, ac.drawInfo.heading);
        // configuration
        std::array<float,10> arrGear;               // gear ratio for any possible gear...10 are defined by X-Plane!
        arrGear.fill(ac.v[V_CONTROLS_GEAR_RATIO]);
        XPLMSetDatavf(mdr.gear, arrGear.data(), 0, 10);
        XPLMSetDataf(mdr.flap,  ac.v[V_CONTROLS_FLAP_RATIO]);
        XPLMSetDataf(mdr.flap2, ac.v[V_CONTROLS_FLAP_RATIO]);
        // [...]
        XPLMSetDataf(mdr.yoke_pitch, ac.v[V_CONTROLS_YOKE_PITCH_RATIO]);
        XPLMSetDataf(mdr.yoke_roll,  ac.v[V_CONTROLS_YOKE_ROLL_RATIO]);
        XPLMSetDataf(mdr.yoke_yaw,   ac.v[V_CONTROLS_YOKE_HEADING_RATIO]);
        
        // For performance reasons and because differences (cartesian velocity)
        // are smoother if calculated over "longer" time frames,
        // the following updates are done about every second only
        if (tNow - ac.prev_ts > std::chrono::seconds(1))
        {
            // do we have any prev x/y/z values at all?
            if (ac.prev_ts != std::chrono::steady_clock::time_point()) {
                // yes, so we can calculate velocity
                const std::chrono::duration<double, std::milli> dur_ms = tNow - ac.prev_ts;
                const double d_s = dur_ms.count() / 1000.0;     // seconds with fractions
                XPLMSetDataf(mdr.v_x, float((ac.drawInfo.x - ac.prev_x) / d_s));
                XPLMSetDataf(mdr.v_y, float((ac.drawInfo.y - ac.prev_y) / d_s));
                XPLMSetDataf(mdr.v_z, float((ac.drawInfo.z - ac.prev_z) / d_s));
            }
            ac.prev_x = ac.drawInfo.x;
            ac.prev_y = ac.drawInfo.y;
            ac.prev_z = ac.drawInfo.z;
            ac.prev_ts = tNow;

            // configuration (cont.)
            XPLMSetDataf(mdr.spoiler,       ac.v[V_CONTROLS_SPOILER_RATIO]);
            XPLMSetDataf(mdr.speedbrake,    ac.v[V_CONTROLS_SPEED_BRAKE_RATIO]);
            XPLMSetDataf(mdr.slat,          ac.v[V_CONTROLS_SLAT_RATIO]);
            XPLMSetDataf(mdr.wingSweep,     ac.v[V_CONTROLS_WING_SWEEP_RATIO]);
            std::array<float,8> arrThrottle;
            arrThrottle.fill(ac.v[V_CONTROLS_THRUST_RATIO]);
            XPLMSetDatavf(mdr.throttle,     arrThrottle.data(), 0, 8);
            // lights
            XPLMSetDatai(mdr.bcnLights,     ac.v[V_CONTROLS_BEACON_LITES_ON] > 0.5f);
            XPLMSetDatai(mdr.landLights,    ac.v[V_CONTROLS_LANDING_LITES_ON] > 0.5f);
            XPLMSetDatai(mdr.navLights,     ac.v[V_CONTROLS_NAV_LITES_ON] > 0.5f);
            XPLMSetDatai(mdr.strbLights,    ac.v[V_CONTROLS_STROBE_LITES_ON] > 0.5f);
            XPLMSetDatai(mdr.taxiLights,    ac.v[V_CONTROLS_TAXI_LITES_ON] > 0.5f);

            // Shared data for providing textual info (see XPMPInfoTexts_t)
            XPLMSetDatab(mdr.infoTailNum,       ac.acInfoTexts.tailNum,       0, sizeof(XPMPInfoTexts_t::tailNum));
            XPLMSetDatab(mdr.infoIcaoAcType,    ac.acInfoTexts.icaoAcType,    0, sizeof(XPMPInfoTexts_t::icaoAcType));
            XPLMSetDatab(mdr.infoManufacturer,  ac.acInfoTexts.manufacturer,  0, sizeof(XPMPInfoTexts_t::manufacturer));
            XPLMSetDatab(mdr.infoModel,         ac.acInfoTexts.model,         0, sizeof(XPMPInfoTexts_t::model));
            XPLMSetDatab(mdr.infoIcaoAirline,   ac.acInfoTexts.icaoAirline,   0, sizeof(XPMPInfoTexts_t::icaoAirline));
            XPLMSetDatab(mdr.infoAirline,       ac.acInfoTexts.airline,       0, sizeof(XPMPInfoTexts_t::airline));
            XPLMSetDatab(mdr.infoFlightNum,     ac.acInfoTexts.flightNum,     0, sizeof(XPMPInfoTexts_t::flightNum));
            XPLMSetDatab(mdr.infoAptFrom,       ac.acInfoTexts.aptFrom,       0, sizeof(XPMPInfoTexts_t::aptFrom));
            XPLMSetDatab(mdr.infoAptTo,         ac.acInfoTexts.aptTo,         0, sizeof(XPMPInfoTexts_t::aptTo));
        }
        
        // remember the highest idx used
        if (multiIdx > maxMultiIdxUsed)
            maxMultiIdxUsed = multiIdx;
        
        // count number of multiplayer planes reported...we can stop early if
        // all slots have been filled
        if (++multiCount >= gMultiRef.size())
            break;
    }

    // Cleanup unused multiplayer datarefs
    for (multiDataRefsTy& mdr : gMultiRef)
        // if not used reset all values
        if (!mdr.bSlotTaken)
            AIMultiClearDataRefs(mdr);

    // Tell X-Plane the highest slot number we are actually really using
    XPLMSetActiveAircraftCount(int(maxMultiIdxUsed+1));
}

//
// MARK: Control Functions
//

/// Resets all actual values of the AI/multiplayer dataRefs of one plane to something initial
void AIMultiClearDataRefs (multiDataRefsTy& mdr)
{
    mdr.bSlotTaken = false;
    XPLMSetDataf(mdr.X, FAR_AWAY_VAL_GL);       // a "far away" location
    XPLMSetDataf(mdr.Y, FAR_AWAY_VAL_GL);
    XPLMSetDataf(mdr.Z, FAR_AWAY_VAL_GL);
    
    XPLMSetDataf(mdr.v_x, 0.0f);                // zero speed
    XPLMSetDataf(mdr.v_y, 0.0f);
    XPLMSetDataf(mdr.v_z, 0.0f);
    
    XPLMSetDataf(mdr.pitch, 0.0f);              // level attitude
    XPLMSetDataf(mdr.roll, 0.0f);
    XPLMSetDataf(mdr.heading, 0.0f);
    
    XPLMSetDatavf(mdr.gear, F_NULL, 0, 10);     // all configuration reset to 0
    XPLMSetDataf(mdr.flap, 0.0f);
    XPLMSetDataf(mdr.flap2, 0.0f);
    XPLMSetDataf(mdr.spoiler, 0.0f);
    XPLMSetDataf(mdr.speedbrake, 0.0f);
    XPLMSetDataf(mdr.slat, 0.0f);
    XPLMSetDataf(mdr.wingSweep, 0.0f);
    XPLMSetDatavf(mdr.throttle, F_NULL, 0, 8);
    XPLMSetDataf(mdr.yoke_pitch, 0.0f);
    XPLMSetDataf(mdr.yoke_roll, 0.0f);
    XPLMSetDataf(mdr.yoke_yaw, 0.0f);
    
    XPLMSetDatai(mdr.bcnLights, 0);             // all lights off
    XPLMSetDatai(mdr.landLights, 0);
    XPLMSetDatai(mdr.navLights, 0);
    XPLMSetDatai(mdr.strbLights, 0);
    XPLMSetDatai(mdr.taxiLights, 0);
    
    // Shared data for providing textual info (see XPMPInfoTexts_t)
    char allNulls[100];
    memset (allNulls, 0, sizeof(allNulls));
    XPLMSetDatab(mdr.infoTailNum,       allNulls, 0, sizeof(XPMPInfoTexts_t::tailNum));
    XPLMSetDatab(mdr.infoIcaoAcType,    allNulls, 0, sizeof(XPMPInfoTexts_t::icaoAcType));
    XPLMSetDatab(mdr.infoManufacturer,  allNulls, 0, sizeof(XPMPInfoTexts_t::manufacturer));
    XPLMSetDatab(mdr.infoModel,         allNulls, 0, sizeof(XPMPInfoTexts_t::model));
    XPLMSetDatab(mdr.infoIcaoAirline,   allNulls, 0, sizeof(XPMPInfoTexts_t::icaoAirline));
    XPLMSetDatab(mdr.infoAirline,       allNulls, 0, sizeof(XPMPInfoTexts_t::airline));
    XPLMSetDatab(mdr.infoFlightNum,     allNulls, 0, sizeof(XPMPInfoTexts_t::flightNum));
    XPLMSetDatab(mdr.infoAptFrom,       allNulls, 0, sizeof(XPMPInfoTexts_t::aptFrom));
    XPLMSetDatab(mdr.infoAptTo,         allNulls, 0, sizeof(XPMPInfoTexts_t::aptTo));
}

// Initialize the module
/// @details Fetches all dataRef handles for all dataRefs of all up to 19 AI/multiplayer slots
void AIMultiInit ()
{
    // We don't know how many multiplayer planes there are - fetch as many as we can.
    gMultiRef.clear();                  // just a safety measure against multi init
    char        buf[100];
    multiDataRefsTy    d;
    
    // Code for finding a standard X-Plane dataRef
#define FIND_PLANE_DR(membVar, dataRefTxt, PlaneNr)                         \
    snprintf(buf,sizeof(buf),"sim/multiplayer/position/plane%d_" dataRefTxt,PlaneNr);    \
    d.membVar = XPLMFindDataRef(buf);

    // Code for finding a non-standard shared dataRef for text information sharing
#define SHARE_PLANE_DR(membVar, dataRefTxt, PlaneNr)                        \
    snprintf(buf,sizeof(buf),"sim/multiplayer/position/plane%d_" dataRefTxt,PlaneNr);    \
    if (XPLMShareData(buf, xplmType_Data, NULL, NULL))                      \
        d.membVar = XPLMFindDataRef(buf);                                   \
    else d.membVar = NULL;
    
    // Loop over all possible AI/multiplayer slots
    for (int n=1; true; n++)
    {
        // position
        FIND_PLANE_DR(X,                    "x",                    n);
        if (!d.X) break;                    // break out of loop early out of the loop once the slot doesn't exist
        FIND_PLANE_DR(Y,                    "y",                    n);
        FIND_PLANE_DR(Z,                    "z",                    n);
        // cartesian velocities
        FIND_PLANE_DR(v_x,                  "v_x",                  n);
        FIND_PLANE_DR(v_y,                  "v_y",                  n);
        FIND_PLANE_DR(v_z,                  "v_z",                  n);
        // attitude
        FIND_PLANE_DR(pitch,                "the",                  n);
        FIND_PLANE_DR(roll,                 "phi",                  n);
        FIND_PLANE_DR(heading,              "psi",                  n);
        // configuration
        FIND_PLANE_DR(gear,                 "gear_deploy",          n);
        FIND_PLANE_DR(flap,                 "flap_ratio",           n);
        FIND_PLANE_DR(flap2,                "flap_ratio2",          n);
        FIND_PLANE_DR(spoiler,              "spoiler_ratio",        n);
        FIND_PLANE_DR(speedbrake,           "speedbrake_ratio",     n);
        FIND_PLANE_DR(slat,                 "slat_ratio",           n); // _should_ expect this name
        if (!d.slat) {
            FIND_PLANE_DR(slat,             "sla1_ratio",           n); // but in reality it is this
        }
        FIND_PLANE_DR(wingSweep,            "wing_sweep",           n);
        FIND_PLANE_DR(throttle,             "throttle",             n);
        FIND_PLANE_DR(yoke_pitch,           "yolk_pitch",           n);
        FIND_PLANE_DR(yoke_roll,            "yolk_roll",            n);
        FIND_PLANE_DR(yoke_yaw,             "yolk_yaw",             n);
        // lights
        FIND_PLANE_DR(bcnLights,            "beacon_lights_on",     n);
        FIND_PLANE_DR(landLights,           "landing_lights_on",    n);
        FIND_PLANE_DR(navLights,            "nav_lights_on",        n);
        FIND_PLANE_DR(strbLights,           "strobe_lights_on",     n);
        FIND_PLANE_DR(taxiLights,           "taxi_light_on",        n);
        
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
        
        if (!d) break;
        gMultiRef.push_back(d);
    }
}

// Grace cleanup
/// @details Unshare the shared dataRefs
void AIMultiCleanup ()
{
    // Unshare shared data
#define UNSHARE_PLANE_DR(membVar, dataRefTxt, PlaneNr)                         \
    snprintf(buf,sizeof(buf),"sim/multiplayer/position/plane%d_" dataRefTxt,PlaneNr);       \
    XPLMUnshareData(buf, xplmType_Data, NULL, NULL);                           \
    d.membVar = NULL;
    
    char        buf[100];
    for (int n = 1; (size_t)n <= gMultiRef.size(); n++)
    {
        multiDataRefsTy& d = gMultiRef[(size_t)n-1];
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
}


}  // namespace XPMP2

//
// MARK: General API functions outside XPMP2 namespace
//

using namespace XPMP2;

// Acquire control of multiplayer aircraft
const char *    XPMPMultiplayerEnable()
{
    // short-cut if we are already in control
    if (XPMPHasControlOfAIAircraft())
        return "";
    
    // Attempt to grab multiplayer planes, then analyze.
    glob.bHasControlOfAIAircraft = XPLMAcquirePlanes(NULL, NULL, NULL) != 0;
    if (glob.bHasControlOfAIAircraft)
    {
        // Success, now we have control of AI aircraft!
        int total=0, active=0;
        XPLMPluginID who=0;
        XPLMCountAircraft(&total, &active, &who);

        // But so far, we have none published
        XPLMSetActiveAircraftCount(1);
        
        // Disable AI for all the planes...we move them now
        for (int i = 1; i < total; i++)
            XPLMDisableAIForPlane(i);

        // Cleanup multiplayer values...we are in control now
        XPMPInitMultiplayerDataRefs();
        
        // Success
        return "";
    }
    else
    {
        LOG_MSG(logWARN, WARN_NO_AI_CONTROL);
        return "Could not acquire AI/multiplayer planes";
    }
}

// Release control of multiplayer aircraft
void XPMPMultiplayerDisable()
{
    // short-cut if we are not in control
    if (!XPMPHasControlOfAIAircraft())
        return;
    
    // Cleanup our values
    XPLMSetActiveAircraftCount(1);
    XPMPInitMultiplayerDataRefs();

    // Then fully release AI/multiplayer planes
    XPLMReleasePlanes();
    glob.bHasControlOfAIAircraft = false;
}

// Do we control AI planes?
bool XPMPHasControlOfAIAircraft()
{
    return glob.bHasControlOfAIAircraft;
}

/// Reset all (controlled) multiplayer dataRef values of all planes
void XPMPInitMultiplayerDataRefs()
{
    if (XPMPHasControlOfAIAircraft())
        for (multiDataRefsTy& mdr : gMultiRef)
            AIMultiClearDataRefs(mdr);
}

