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
/// @warning    The "TCAS hack" depends on the drawing callback functionality of X-Plane via
///             [XPLMRegisterDrawCallback](https://developer.x-plane.com/sdk/XPLMDisplay/#XPLMRegisterDrawCallback).
///             which is "likely [to] be removed during the X-Plane 11 run as part of the transition to Vulkan/Metal/etc."
///             Ping-ponging the number of AI planes between 1 (so X-Plane won't draw
///             AI planes at our positions) and the actual number would need to move elsewhere then.
///             See AIControlPlaneCount().
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

#define INFO_AI_CONTROL         "Have control now over %d AI/Multiplayer planes"
#define INFO_AI_CONTROL_ENDS    "Released control of AI/Multiplayer planes"
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
constexpr float AISLOT_CHANGE_PERIOD = 30.0f;
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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
/// Keeps the dataRef handles for all of the up to 19 AI/Multiplayer slots (accepted as a global variable requiring an exit-time destructor)
static std::vector<multiDataRefsTy>        gMultiRef;
/// Vector of AI/multiplayer plane models _before_ we change them, so we can restore models before releasing AI control
static std::vector<std::string> gvecAIOrigModels;
#pragma clang diagnostic pop

/// Map of Aircrafts, sorted by (priority-biased) distance
typedef std::map<float,Aircraft&> mapAcByDistTy;

// Resets all actual values of the AI/multiplayer dataRefs of one plane to something initial
void AIMultiClearDataRefs (multiDataRefsTy& mdr, bool bDeactivateToZero = false);

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

/// @brief Initializes all AI planes for our usage (also additionally defined AI planes during runtime)
/// @param total Total number of planes as returned by XPLMCountAircraft
/// @details Disables AI and specifically sets the "NoPlane" aircraft model,
///          so that no actual plane will be rendered.
///          If the user has configured "NoPlane" as the AI plane type already,
///          then no further action is done to avoid delays due to loading of models.
///          Cleans all dataRefs, so that we can start fresh with defined values.
void AIMultiInitPlanes (int total)
{
    // Only needed if the given total is different from what
    // has already been initialized
    if (total != glob.nAIPlanesInitialized)
    {
        char pathOld[512], nameOld[256];
        LOG_ASSERT(gvecAIOrigModels.size() >= size_t(total));
        
        // start either at beginning or where left off last time
        // (to catch additionally defined AI planes during runtime)
        for (int i = std::max(1, glob.nAIPlanesInitialized);
             i < total; i++)
        {
            // Disable AI for all the planes..._we_ move them now.
            XPLMDisableAIForPlane(i);

            // Save current model, in order to restore it when exiting,
            // and set our own "NoPlane.acf" model. (There's a config switch to skip this.)
            if (!glob.bSkipAssignNoPlane)
            {
                bool bIsNoPlaneAlready = false;
                if (gvecAIOrigModels[size_t(i)].empty()) {
                    XPLMGetNthAircraftModel(i, nameOld, pathOld);
                    bIsNoPlaneAlready = !strcmp(nameOld, RSRC_NO_PLANE);    // is the model already configured to be "NoPlane"?
                    if (!bIsNoPlaneAlready && strlen(pathOld) > 5)          // sometimes we get just a single garbage char...don't store garbage
                        gvecAIOrigModels[size_t(i)] = pathOld;
                }

                // And set the "NoPlane" model so that they are no actually rendered,
                // but appear on TCAS.
                if (!bIsNoPlaneAlready)
                    XPLMSetAircraftModel(i, glob.pathNoPlane.c_str());
            }
        }

        // Init done
        LOG_MSG(logINFO, INFO_AI_CONTROL, total-1);
        glob.nAIPlanesInitialized = total;
    }
}

/// Restores original AI plane models, typically shortly before releasing AI control
void AIMultiRestorePlanes ()
{
    // Restore all known plane types to their former values
    for (size_t i = 1; i < gvecAIOrigModels.size(); i++) {
        if (!gvecAIOrigModels[i].empty()) {
            XPLMSetAircraftModel(int(i), gvecAIOrigModels[i].c_str());
            gvecAIOrigModels[i].clear();
        }
    }
    // Nothing initiallized any longer
    glob.nAIPlanesInitialized = 0;
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
    
    // (Potentially) initialize the AI planes for usage
    AIMultiInitPlanes(numAIPlanes);
    
    // For further steps: first plane is the user plane, here we don't count that
    numAIPlanes--;

    // reset our bookkeeping on used multiplay idx
    for (multiDataRefsTy& iter: gMultiRef)
        iter.bSlotTaken = false;
    
    // Time of last slot switching activity
    static float tLastSlotSwitching = 0.0f;
    const float now = GetMiscNetwTime();
    // only every few seconds rearrange slots, ie. add/remove planes or
    // move planes between lower and upper section of AI slots:
    if (CheckEverySoOften(tLastSlotSwitching, AISLOT_CHANGE_PERIOD, now))
    {
        // Sort all planes by prioritized distance
        mapAcByDistTy mapAcByDist;
        for (const auto& pair: glob.mapAc) {
            Aircraft& ac = *pair.second;
            // only consider planes that require being shown as AI aircraft
            // (these excludes invisible planes and those with transponder off)
            if (ac.ShowAsAIPlane())
                // Priority distance means that we add artificial distance for higher-numbered AI priorities
                mapAcByDist.emplace(ac.GetCameraDist() + ac.aiPrio * AI_PRIO_MULTIPLIER,
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
                        pair.second.GetCameraDist());
            
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
    glob.maxMultiIdxUsed = 0;   // we figure out the max idx during the loop
    
    for (const auto& pair: glob.mapAc) {
        Aircraft& ac = *pair.second;
        
        // skip planes not having an AI slot
        if (ac.GetMultiIdx() < 0)
            continue;
        const size_t multiIdx = (size_t)ac.GetMultiIdx();
        // should not happen...but we don't want runtime errors either
        if (multiIdx >= gMultiRef.size()) {
            LOG_MSG(logDEBUG, "Aircraft %llu: ERROR, too high an AI slot: %ld",
                    (long long unsigned)ac.GetPlaneID(),
                    multiIdx);
            continue;
        }
        
        // this slot taken (safety measure...should not really be needed)
        gMultiRef[multiIdx].bSlotTaken = true;
        const multiDataRefsTy& mdr = gMultiRef[multiIdx];

        // This plane's position
        XPLMSetDataf(mdr.X, ac.drawInfo.x);
        XPLMSetDataf(mdr.Y, ac.drawInfo.y - ac.GetVertOfs());  // align with original altitude
        XPLMSetDataf(mdr.Z, ac.drawInfo.z);
        // attitude
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
        if (now >= ac.prev_ts + 1.0f)
        {
            // do we have any prev x/y/z values at all?
            if (ac.prev_ts > 0.0001f) {
                // yes, so we can calculate velocity
                const float d_s = now - ac.prev_ts;                 // time that had passed in seconds
                XPLMSetDataf(mdr.v_x, (ac.drawInfo.x - ac.prev_x) / d_s);
                XPLMSetDataf(mdr.v_y, (ac.drawInfo.y - ac.prev_y) / d_s);
                XPLMSetDataf(mdr.v_z, (ac.drawInfo.z - ac.prev_z) / d_s);
            }
            ac.prev_x = ac.drawInfo.x;
            ac.prev_y = ac.drawInfo.y;
            ac.prev_z = ac.drawInfo.z;
            ac.prev_ts = now;

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
        if (multiIdx > glob.maxMultiIdxUsed)
            glob.maxMultiIdxUsed = multiIdx;
        
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

    // Set the number of planes
    // "+2" because one plane is the user's plane, and also maxMultiIdxUsed is zero based
    const int nCount = 2 + int(glob.maxMultiIdxUsed);
    XPLMSetActiveAircraftCount(nCount);
}

//
// MARK: Control Functions
//

/// Resets all actual values of the AI/multiplayer dataRefs of one plane to something initial
void AIMultiClearDataRefs (multiDataRefsTy& mdr,
                           bool bDeactivateToZero)
{
    mdr.bSlotTaken = false;
    // either a "far away" location or standard 0, which is, however, a valid location somewhere!
    XPLMSetDataf(mdr.X, bDeactivateToZero ? 0.0f : FAR_AWAY_VAL_GL);
    XPLMSetDataf(mdr.Y, bDeactivateToZero ? 0.0f : FAR_AWAY_VAL_GL);
    XPLMSetDataf(mdr.Z, bDeactivateToZero ? 0.0f : FAR_AWAY_VAL_GL);
    
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

/// Reset all (controlled) multiplayer dataRef values of all planes
void AIMultiInitAllDataRefs(bool bDeactivateToZero)
{
    if (XPMPHasControlOfAIAircraft())
        for (multiDataRefsTy& mdr : gMultiRef)
            AIMultiClearDataRefs(mdr, bDeactivateToZero);
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
    int n=1;
    for (n=1; true; n++)
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
    
    // Properly size the vector that is to hold original model names
    gvecAIOrigModels.assign(size_t(n),std::string());
}

// Grace cleanup
/// @details Make sure we aren't in control, then Unshare the shared dataRefs
void AIMultiCleanup ()
{
    // Make sure we are no longer in control
    XPMPMultiplayerDisable();
    
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
    
    // Cleanup other arrays properly before shutdown
    gMultiRef.clear();
    gvecAIOrigModels.clear();
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

        // Initialize available AI planes.
        // (Note: If this is called during X-Plane's startup,
        //        e.g. from XPluginStart/Enable, then total will not be known
        //        and be just 1. But AIInitPlanes is called again from within
        //        the flight loop and handles later additions of AI planes.
        AIMultiInitPlanes(total);
        
        // No Planes yet started, initialize all dataRef values for a clean start
        XPLMSetActiveAircraftCount(1);
        AIMultiInitAllDataRefs(false);

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
    XPLMSetActiveAircraftCount(1);      // no active AI plane
    AIMultiRestorePlanes();             // restore previously set models (might stall for loading them!)
    AIMultiInitAllDataRefs(true);       // reset all dataRef values to zero
    
    // Reset the last index used
    for (auto& pair: glob.mapAc)
        pair.second->ResetMultiIdx();

    // Then fully release AI/multiplayer planes
    XPLMReleasePlanes();
    glob.bHasControlOfAIAircraft = false;
    LOG_MSG(logINFO, INFO_AI_CONTROL_ENDS);
}

// Do we control AI planes?
bool XPMPHasControlOfAIAircraft()
{
    return glob.bHasControlOfAIAircraft;
}

