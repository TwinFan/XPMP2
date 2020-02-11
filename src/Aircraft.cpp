/// @file       Aircraft.cpp
/// @brief      XPCAircraft represents an aircraft as managed by XPMP2
/// @note       This class bases on and is compatible to the XPCAircraft wrapper
///             class provided with the original libxplanemp.
///             In XPMP2, however, this class is not a wrapper but the actual
///             means of managing aircraft. Hence, it includes a lot more members.
/// @author     Birger Hoppe
/// @copyright  The original XPCAircraft.h file in libxplanemp had no copyright note.
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

using namespace XPMP2;

//
// MARK: Globals
//

#define WARN_MODEL_NOT_FOUND    "Named CSL Model '%s' not found"
#define ERR_CREATE_INSTANCE     "Aircraft %llu: Create Instance FAILED for CSL Model %s"
#define DEBUG_INSTANCE_CREATED  "Aircraft %llu: Instance created"
#define DEBUG_INSTANCE_DESTRYD  "Aircraft %llu: Instance destroyed"
#define INFO_MODEL_CHANGE       "Aircraft %llu: Changing model from %s to %s"

namespace XPMP2 {

/// The id of our flight loop callback
XPLMFlightLoopID gFlightLoopID = nullptr;

/// The list of dataRefs we support to be read by the CSL Model (for gear, flaps, lights etc.)
const char * DR_NAMES[] = {
    "libxplanemp/controls/gear_ratio",
    "libxplanemp/controls/flap_ratio",
    "libxplanemp/controls/spoiler_ratio",
    "libxplanemp/controls/speed_brake_ratio",
    "libxplanemp/controls/slat_ratio",
    "libxplanemp/controls/wing_sweep_ratio",
    "libxplanemp/controls/thrust_ratio",
    "libxplanemp/controls/yoke_pitch_ratio",
    "libxplanemp/controls/yoke_heading_ratio",
    "libxplanemp/controls/yoke_roll_ratio",
    "libxplanemp/controls/thrust_revers",
    
    "libxplanemp/controls/taxi_lites_on",
    "libxplanemp/controls/landing_lites_on",
    "libxplanemp/controls/beacon_lites_on",
    "libxplanemp/controls/strobe_lites_on",
    "libxplanemp/controls/nav_lites_on",
    
    "libxplanemp/gear/tire_vertical_deflection_mtr",
    "libxplanemp/gear/tire_rotation_angle_deg",
    "libxplanemp/gear/tire_rotation_speed_rpm",
    "libxplanemp/gear/tire_rotation_speed_rad_sec",
    
    "libxplanemp/engines/engine_rotation_angle_deg",
    "libxplanemp/engines/engine_rotation_speed_rpm",
    "libxplanemp/engines/engine_rotation_speed_rad_sec",       // PE defines this: https://www.pilotedge.net/pages/csl-authoring
    "libxplanemp/engines/prop_rotation_angle_deg",
    "libxplanemp/engines/prop_rotation_speed_rpm",
    "libxplanemp/engines/prop_rotation_speed_rad_sec",
    "libxplanemp/engines/thrust_reverser_deploy_ratio",
    
    "libxplanemp/misc/touch_down",
    
    // always last, marks the end in the call to XPLMCreateInstace:
    nullptr
};

/// How many dataRefs have been defined?
constexpr int DR_NAMES_COUNT = sizeof(DR_NAMES)/sizeof(DR_NAMES[0]) - 1;
static_assert(DR_NAMES_COUNT == V_COUNT, "Number of dataRef strings does not match number of enums and array length of `Aircraft::v");

/// Registered dataRefs (only the first element is guaranteed to be zero, but that's sufficient)
XPLMDataRef ahDataRefs[DR_NAMES_COUNT] = {0};

//
// MARK: XPMP2 New Definitions
//

// Legacy constructor creates a plane and puts it under control of XPlaneMP
Aircraft::Aircraft(const std::string& _icaoType,
                   const std::string& _icaoAirline,
                   const std::string& _livery,
                   const std::string& _modelName) :
mPlane(glob.NextPlaneId())              // assign the next synthetic plane id
{
    // Size of drawInfo
    drawInfo.structSize = sizeof(drawInfo);
    
    // if given try to find the CSL model to use by its name
    if (!_modelName.empty())
        AssignModel(_modelName);
    
    // Let Matching happen, if we still don't have a model
    if (!pCSLMdl)
        ChangeModel(_icaoType, _icaoAirline, _livery);
    LOG_ASSERT(pCSLMdl);
    
    // add the aircraft to our global map and inform observers
    glob.mapAc.emplace(mPlane,this);
    XPMPSendNotification(*this, xpmp_PlaneNotification_Created);
    
    // make sure the flight loop callback gets called if this was the first a/c
    if (glob.mapAc.size() == 1) {
        // Create the flight loop callback (unscheduled) if not yet there
        if (!gFlightLoopID) {
            XPLMCreateFlightLoop_t cfl = {
                sizeof(XPLMCreateFlightLoop_t),                 // size
                xplm_FlightLoop_Phase_BeforeFlightModel,        // phase
                FlightLoopCB,                                   // callback function
                (void*)xplm_FlightLoop_Phase_BeforeFlightModel  // refcon
            };
            gFlightLoopID = XPLMCreateFlightLoop(&cfl);
        }
        
        // Schedule the flight loop callback to be called next flight loop cycle
        XPLMScheduleFlightLoop(gFlightLoopID, -1.0f, 0);
        LOG_MSG(logDEBUG, "Flight loop callback started");
    }
}

// Destructor cleans up all resources acquired
Aircraft::~Aircraft ()
{
    // inform observers
    XPMPSendNotification(*this, xpmp_PlaneNotification_Destroyed);
    
    // Remove the instance
    DestroyInstances();
    
    // Decrease the reference counter of the CSL model
    pCSLMdl->DecRefCnt();

    // remove myself from the global map of planes
    glob.mapAc.erase(mPlane);
}


// (Potentially) change the plane's model after doing a new match attempt
int Aircraft::ChangeModel (const std::string& _icaoType,
                           const std::string& _icaoAirline,
                           const std::string& _livery)
{
    // Let matching happen
    CSLModel* pMdl = nullptr;
    int q = CSLModelMatching(_icaoType,
                             _icaoAirline,
                             _livery,
                             pMdl);
    // Found a model?
    if (pMdl) {
        // Is this a change to the currently used model?
        const bool bChangeExisting = (pCSLMdl && pMdl != pCSLMdl);
        if (bChangeExisting) {
            // remove the current instance (which is based on the previous model)
            LOG_MSG(logINFO, INFO_MODEL_CHANGE,
                    (long long unsigned)mPlane,
                    pCSLMdl->GetId().c_str(),
                    pMdl->GetId().c_str());
            DestroyInstances();
            // Decrease the reference counter of the CSL model
            pCSLMdl->DecRefCnt();
        }
        
        // save the newly selected model
        pCSLMdl         = pMdl;
        matchQuality    = q;
        acIcaoType      = _icaoType;
        acIcaoAirline   = _icaoAirline;
        acLivery        = _livery;

        // Increase the reference counter of the CSL model to track that the object is being used
        pCSLMdl->IncRefCnt();

        // Determin map icon based on icao type
        MapFindIcon();
        
        // inform observers in case this was an actual replacement change
        if (bChangeExisting)
            XPMPSendNotification(*this, xpmp_PlaneNotification_ModelChanged);
    }
    return q;
}


// Assigns the given model per name, returns if successful
bool Aircraft::AssignModel (const std::string& _modelName)
{
    // try finding the model by name
    CSLModel* pMdl = CSLModelByName(_modelName);
    if (!pMdl) {                            // nothing changes if not found
        LOG_MSG(logWARN, WARN_MODEL_NOT_FOUND, _modelName.c_str());
        return false;
    }

    // Is this a change to the currently used model?
    const bool bChangeExisting = (pCSLMdl && pMdl != pCSLMdl);
    if (bChangeExisting) {
        LOG_MSG(logINFO, INFO_MODEL_CHANGE,
                (long long unsigned)mPlane,
                pCSLMdl->GetId().c_str(),
                pMdl->GetId().c_str());
        DestroyInstances();                 // remove the current instance (which is based on the previous model)
        // Decrease the reference counter of the CSL model
        pCSLMdl->DecRefCnt();
    }
    pCSLMdl         = pMdl;             // save the newly selected model
    matchQuality    = 0;
    acIcaoType      = pCSLMdl->GetIcaoType();
    acIcaoAirline   = pCSLMdl->GetIcaoAirline();
    acLivery        = pCSLMdl->GetLivery();

    // Increase the reference counter of the CSL model to track that the object is being used
    pCSLMdl->IncRefCnt();

    // Determin map icon based on icao type
    MapFindIcon();
    
    // inform observers
    if (bChangeExisting)
        XPMPSendNotification(*this, xpmp_PlaneNotification_ModelChanged);

    return true;
}


// return the name of the CSL model in use
std::string Aircraft::GetModelName () const
{
    return pCSLMdl ? pCSLMdl->GetId() : "";
}


// Vertical offset, ie. the value that needs to be added to drawInfo.y to make the aircraft appear on the ground
float Aircraft::GetVertOfs () const
{
    return pCSLMdl ? pCSLMdl->GetVertOfs() * vertOfsRatio : 0.0f;
}

// Static: Flight loop callback function
float Aircraft::FlightLoopCB (float, float, int, void*)
{
    UPDATE_CYCLE_NUM;               // DEBUG only: Store current cycle number in glob.xpCycleNum
    
    // Update configuration
    glob.UpdateCfgVals();
    
    // Need the camera's position to calculate the a/c's distance to it
    XPLMCameraPosition_t posCamera;
    XPLMReadCameraPosition(&posCamera);
    
    // Update positional and configurational values
    for (mapAcTy::value_type& pair: glob.mapAc) {
        pair.second->UpdatePosition();
        // Update plane's distance
        pair.second->UpdateDistCamera(posCamera);
    }
    
    // Move all planes
    for (mapAcTy::value_type& pair: glob.mapAc)
        pair.second->DoMove();

    // Publish aircraft data on the AI/multiplayer dataRefs
    AIMultiUpdate();
            
    // Don't call me again if there are no more aircraft,
    if (glob.mapAc.empty()) {
        LOG_MSG(logDEBUG, "Flight loop callback ended");
        return 0.0f;
    } else {
        // call me next cycle if there are
        return -1.0f;
    }
}

// This puts the instance into XP's sky and makes it move
void Aircraft::DoMove ()
{
    // Only for visible planes
    if (bVisible) {
        // Already have instances? Or succeeded in now creating them?
        if (!listInst.empty() || CreateInstances()) {
            // Move the instances (this is probably the single most important line of code ;-) )
            for (XPLMInstanceRef hInst: listInst)
                XPLMInstanceSetPosition(hInst, &drawInfo, v.data());
        }
    }
}


// Internal: Update the plane's distance from the camera location
void Aircraft::UpdateDistCamera (const XPLMCameraPosition_t& posCam)
{
    distCamera = dist(posCam.x,   posCam.y,   posCam.z,
                      drawInfo.x, drawInfo.y, drawInfo.z);
}


// Create the instances, return if successful
bool Aircraft::CreateInstances ()
{
    // If we have instances already we just return
    if (!listInst.empty()) return true;

    // Let's see if we can get ALL object handles:
    LOG_ASSERT(pCSLMdl);
    std::list<XPLMObjectRef> listObj = pCSLMdl->GetAllObjRefs();
    if (listObj.empty())                    // we couldn't...
        return false;
    
    // OK, we got a complete list of objects, so let's instanciate them:
    for (XPLMObjectRef hObj: listObj) {
        // Create a (new) instance of this CSL Model object,
        // registering all the dataRef names we support
        XPLMInstanceRef hInst = XPLMCreateInstance (hObj, DR_NAMES);
        
        // Didn't work???
        if (!hInst) {
            LOG_MSG(logERR, ERR_CREATE_INSTANCE,
                    (long long unsigned)mPlane,
                    pCSLMdl->GetId().c_str());
            DestroyInstances();             // remove other instances we might have created already
            return false;
        }

        // Save the instance
        listInst.push_back(hInst);
    }
    
    // Success!
    LOG_MSG(logDEBUG, DEBUG_INSTANCE_CREATED, (long long unsigned)mPlane);
    return true;
}

// Destroy all instances
void Aircraft::DestroyInstances ()
{
    while (!listInst.empty()) {
        XPLMDestroyInstance(listInst.back());
        listInst.pop_back();
    }
    LOG_MSG(logDEBUG, DEBUG_INSTANCE_DESTRYD,
            (long long unsigned)mPlane);
}


// Converts world coordinates to local coordinates, writes to `drawInfo`
void Aircraft::SetLocation(double lat, double lon, double alt_f)
{
    // Weirdly, XPLMWorldToLocal expects points to double, while XPLMDrawInfo_t later on provides floats,
    // so we need intermediate variables
    double x, y, z;
    XPLMWorldToLocal(lat, lon, alt_f * M_per_FT,
                     &x, &y, &z);
    
    // Copy to drawInfo
    drawInfo.x = float(x);
    drawInfo.y = float(y) + GetVertOfs();
    drawInfo.z = float(z);
}



// Converts aircraft's local coordinates to lat/lon values
void Aircraft::GetLocation (double& lat, double& lon, double& alt_ft) const
{
    XPLMLocalToWorld(drawInfo.x, drawInfo.y, drawInfo.z,
                     &lat, &lon, &alt_ft);
    alt_ft /= M_per_FT;
}

// Make the plane (in)visible
void Aircraft::SetVisible (bool _bVisible)
{
    // no change?
    if (bVisible == _bVisible)
        return;
    
    // Set the flag
    bVisible = _bVisible;
    
    // In case of _now_ being invisible remove the instances and any AI slot
    if (!bVisible) {
        DestroyInstances();
        AISlotClear();
    }
}

//
// MARK: LegacyAircraft
//

// Constructor accepts the very same parameters as XPMPCreatePlane() and XPMPCreatePlaneWithModelName()
LegacyAircraft::LegacyAircraft(const char*      _ICAOCode,
                               const char*      _Airline,
                               const char*      _Livery,
                               XPMPPlaneData_f  _DataFunc,
                               void *           _Refcon,
                               const char *     _ModelName) :
XPCAircraft (_ICAOCode, _Airline, _Livery, _ModelName),
dataFunc (_DataFunc),
refcon (_Refcon)
{}

// Just calls `dataFunc`
XPMPPlaneCallbackResult LegacyAircraft::GetPlanePosition(XPMPPlanePosition_t* outPosition)
{
    if (!dataFunc) return xpmpData_Unavailable;
    return dataFunc(mPlane, xpmpDataType_Position, outPosition, refcon);
}

// Just calls `dataFunc`
XPMPPlaneCallbackResult LegacyAircraft::GetPlaneSurfaces(XPMPPlaneSurfaces_t* outSurfaces)
{
    if (!dataFunc) return xpmpData_Unavailable;
    return dataFunc(mPlane, xpmpDataType_Surfaces, outSurfaces, refcon);
}

// Just calls `dataFunc`
XPMPPlaneCallbackResult LegacyAircraft::GetPlaneRadar(XPMPPlaneRadar_t* outRadar)
{
    if (!dataFunc) return xpmpData_Unavailable;
    return dataFunc(mPlane, xpmpDataType_Radar, outRadar, refcon);
}

// Just calls `dataFunc`
XPMPPlaneCallbackResult LegacyAircraft::GetInfoTexts(XPMPInfoTexts_t* outInfoTexts)
{
    if (!dataFunc) return xpmpData_Unavailable;
    return dataFunc(mPlane, xpmpDataType_InfoTexts, outInfoTexts, refcon);
}

} // Namespace XPMP2

//
// MARK: XPCAircraft legacy
//

// Legacy constructor creates a plane and puts it under control of XPlaneMP
XPCAircraft::XPCAircraft(const char* inICAOCode,
                         const char* inAirline,
                         const char* inLivery,
                         const char* inModelName) :
Aircraft(inICAOCode, inAirline, inLivery, inModelName ? inModelName : "")
{}

// Just calls all 4 previous `Get...` functions and copies the provided values into `drawInfo` and `v`
void XPCAircraft::UpdatePosition()
{
    // Call the "callback" virtual functions and then update the core variables
    acPos.multiIdx = 			GetMultiIdx();             // provide the multiplayer index back to the plugin
    if (GetPlanePosition(&acPos) == xpmpData_NewData) {
        // Set the position and orientation
        SetLocation(acPos.lat, acPos.lon, acPos.elevation);
        drawInfo.pitch      = acPos.pitch;
        drawInfo.roll       = acPos.roll;
        drawInfo.heading    = acPos.heading;
        // Update the other values from acNextPos
        label               = acPos.label;
        memmove(colLabel, acPos.label_color, sizeof(colLabel));
        vertOfsRatio        = acPos.clampToGround ? acPos.offsetScale : 0.0f;
        aiPrio              = acPos.aiPrio;
    }
    
    if (GetPlaneSurfaces(&acSurfaces) == xpmpData_NewData) {
        v[V_CONTROLS_GEAR_RATIO                  ] = acSurfaces.gearPosition;
        v[V_CONTROLS_FLAP_RATIO                  ] = acSurfaces.flapRatio;
        v[V_CONTROLS_SPOILER_RATIO               ] = acSurfaces.spoilerRatio;
        v[V_CONTROLS_SPEED_BRAKE_RATIO           ] = acSurfaces.speedBrakeRatio;
        v[V_CONTROLS_SLAT_RATIO                  ] = acSurfaces.slatRatio;
        v[V_CONTROLS_WING_SWEEP_RATIO            ] = acSurfaces.wingSweep;
        v[V_CONTROLS_THRUST_RATIO                ] = acSurfaces.thrust;
        v[V_CONTROLS_YOKE_PITCH_RATIO            ] = acSurfaces.yokePitch;
        v[V_CONTROLS_YOKE_HEADING_RATIO          ] = acSurfaces.yokeHeading;
        v[V_CONTROLS_YOKE_ROLL_RATIO             ] = acSurfaces.yokeRoll;
        v[V_CONTROLS_THRUST_REVERS               ] = acSurfaces.thrust < 0.0f ? 1.0f : 0.0f;
        
        v[V_CONTROLS_TAXI_LITES_ON               ] = (float)acSurfaces.lights.taxiLights;
        v[V_CONTROLS_LANDING_LITES_ON            ] = (float)acSurfaces.lights.landLights;
        v[V_CONTROLS_BEACON_LITES_ON             ] = (float)acSurfaces.lights.bcnLights;
        v[V_CONTROLS_STROBE_LITES_ON             ] = (float)acSurfaces.lights.strbLights;
        v[V_CONTROLS_NAV_LITES_ON                ] = (float)acSurfaces.lights.navLights;
        v[V_CONTROLS_TAXI_LITES_ON               ] = (float)acSurfaces.lights.taxiLights;
        
        v[V_GEAR_TIRE_VERTICAL_DEFLECTION_MTR    ] = acSurfaces.tireDeflect;
        v[V_GEAR_TIRE_ROTATION_ANGLE_DEG         ] = acSurfaces.tireRotDegree;
        v[V_GEAR_TIRE_ROTATION_SPEED_RPM         ] = acSurfaces.tireRotRpm;
        v[V_GEAR_TIRE_ROTATION_SPEED_RAD_SEC     ] = acSurfaces.tireRotRpm * RPM_to_RADs;
        
        v[V_ENGINES_ENGINE_ROTATION_ANGLE_DEG    ] = acSurfaces.engRotDegree;
        v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM    ] = acSurfaces.engRotRpm;
        v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC] = acSurfaces.engRotRpm * RPM_to_RADs;
        v[V_ENGINES_PROP_ROTATION_ANGLE_DEG      ] = acSurfaces.propRotDegree;
        v[V_ENGINES_PROP_ROTATION_SPEED_RPM      ] = acSurfaces.propRotRpm;
        v[V_ENGINES_PROP_ROTATION_SPEED_RAD_SEC  ] = acSurfaces.propRotRpm * RPM_to_RADs;
        v[V_ENGINES_THRUST_REVERSER_DEPLOY_RATIO ] = acSurfaces.reversRatio;
        
        v[V_MISC_TOUCH_DOWN                      ] = acSurfaces.touchDown ? 1.0f : 0.0f;
    }
    
    // The following 2 calls provide directly the member variable structures:
    GetPlaneRadar(&acRadar);
    GetInfoTexts(&acInfoTexts);
}

//
// MARK: Global Functions
//

namespace XPMP2 {

/// We need to provide these functions for purely formal reasons.
/// They are not actually _ever_ called as we provide the current dataRef values via XPLMInstanceSetPosition.
/// So we don't bother provided any implementation
static float obj_get_float(void * /*refcon*/)
{
    return 0.0f;
}

/// See obj_get_float()
static int obj_get_float_array(
        void *               /*refcon*/,
        float *              ,
        int                  /*inOffset*/,
        int                  inCount)
{
    return inCount;
}



// Initialize the module
void AcInit ()
{
    // Register all our dataRefs, if not done already
    if (!ahDataRefs[0]) {
        int i = 0;
        for (const char* drName: DR_NAMES) {
            if (!drName) break;
            ahDataRefs[i++] = XPLMRegisterDataAccessor(drName,
                                                       xplmType_Float|xplmType_FloatArray, 0,
                                                       NULL, NULL,
                                                       obj_get_float, NULL,
                                                       NULL, NULL,
                                                       NULL, NULL,
                                                       obj_get_float_array, NULL,
                                                       NULL, NULL, (void*)drName, NULL);
        }
    }
}

// Grace cleanup
void AcCleanup ()
{
    // give all planes a chance to properly clean up
    glob.mapAc.clear();
    
    // Destroy flight loop
    if (gFlightLoopID) {
        XPLMDestroyFlightLoop(gFlightLoopID);
        gFlightLoopID = nullptr;
    }
    
    // Unregister dataRefs
    for (XPLMDataRef dr: ahDataRefs)
        if (dr)
            XPLMUnregisterDataAccessor(dr);
    memset(ahDataRefs, 0, sizeof(ahDataRefs));
}

// Find aircraft by its plane ID, can return nullptr
Aircraft* AcFindByID (XPMPPlaneID _id)
{
    try {
        // try finding the plane by its id
        return glob.mapAc.at(_id);
    }
    catch (const std::out_of_range&) {
        // not found
        return nullptr;
    }
}

}   // namespace XPMP2
