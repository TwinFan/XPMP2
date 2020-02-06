/// @file       XPMP2-Sample.cpp
/// @brief      Example plugin demonstrating XPMP2 techniques
/// @details    This plugin creates 3 planes, one with each of the available ways of using the XPMP2 library.
///             All planes move in a circle around a position 200m in front of the user's plane
///             (even if the user plane moves).
///
///             1. Subclassing XPMP2::Aircraft, which is the recommended way. This plane flies
///             highest, 100m above the user plane.
///             2. Subclassing the legacy class XPCAircraft. This plane flies in the middle, 50m above the user plane.
///             3. Using direct C functions. This plane always rolls on the ground, no matter how
///             high the user plane flies.
///
///             Three menu items are provided:
///
///             1. "Toggle Planes" creates/removes the planes.
///             2. "Toggle Visibility" shows/temporary hides the planes without destroying them.
///             3. "Cycle Models" changes the CSL model used per plane.
///
///             For the plugin to work properly some CSL models are necessary in some folders
///             under `Resources` (all folders under `Resources` are scanned for
///             `xsb_aircraft.txt` file, so the actual structure underneath `Resources` does not matter).
///
///             The directory structure would look as follows:\n
///             `XPMP2-Sample/`\n
///             `  lin_x64/`\n
///             `    XPMP2-Sample.xpl`\n
///             `  mac_x64/`\n
///             `    XPMP2-Sample.xpl`\n
///             `  win_x64/`\n
///             `    XPMP2-Sample.xpl`\n
///             `  Resources/`\n
///             `    Doc8643.txt`\n
///             `    related.txt`\n
///             `    `CSL-Models, e.g. in a folder `CSL/`\n
///
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

// Standard C headers
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cmath>

// X-Plane SDK
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "XPLMPlugin.h"
#include "XPLMGraphics.h"
#include "XPLMMenus.h"

// Include XPMP2 headers
#include "XPCAircraft.h"
#include "XPMPAircraft.h"
#include "XPMPMultiplayer.h"

#if !XPLM300
	#error This plugin requires version 300 of the SDK
#endif

/// Initial type / airline / livery to be used to create our 3 planes
/// @see https://www.icao.int/publications/DOC8643/Pages/Search.aspx for ICAO aircraft types

/// @see https://forums.x-plane.org/index.php?/files/file/37041-bluebell-obj8-csl-packages/ for the Bluebell package, which includes the models named here
std::string PLANE_MODEL[3][3] = {
//    { "B06",  "TXB", "" },
    { "E135", "HAF", "" },
    { "AT76", "NOK", "" },
    { "A321", "DLH", "" },
};

//
// MARK: Utility Functions
//

/// Log a message to X-Plane's Log.txt with sprintf-style parameters
void LogMsg (const char* szMsg, ... )
{
    char buf[512];
    va_list args;
    // Write all the variable parameters
    va_start (args, szMsg);
    std::vsnprintf(buf, sizeof(buf)-2, szMsg, args);
    va_end (args);
    std::strcat(buf, "\n");
    // write to log (flushed immediately -> expensive!)
    XPLMDebugString(buf);
}

/// This is a callback the XPMP2 calls regularly to learn about configuration settings.
/// Only 3 are left, all of them integers.
int CBIntPrefsFunc (const char * section, const char * item, int defaultVal)
{
    if (!strcmp(section, "planes")) {
        // We always want clamping to ground
        if (!strcmp(item, "clamp_all_to_ground")) return 1;
    }
#if DEBUG
    // in debug version of the plugin we provide most complete log output
    else if (!strcmp(section, "debug")) {
        if (!strcmp(item, "model_matching")) return 1;
        if (!strcmp(item, "log_level")) return 0;       // DEBUG logging level
    }
#endif
    // For any other settings we accept defaults
    return defaultVal;
}

/// This is the callback for the plane notifier function, which just logs some info to Log.txt
/// @note Plane notifier functions are completely optional and actually rarely used,
///       because you should know already by other means when your plugin creates/modifies/deletes a plane.
///       So this is for pure demonstration (and testing) purposes.
void CBPlaneNotifier(XPMPPlaneID            inPlaneID,
                     XPMPPlaneNotification  inNotification,
                     void *                 /*inRefcon*/)
{
    XPMP2::Aircraft* pAc = XPMP2::AcFindByID(inPlaneID);
    if (pAc) {
        LogMsg("XPMP2-Sample: Plane of type %s, airline %s, model %s, label '%s' %s",
               pAc->acIcaoType.c_str(),
               pAc->acIcaoAirline.c_str(),
               pAc->GetModelName().c_str(),
               pAc->label.c_str(),
               inNotification == xpmp_PlaneNotification_Created ? "created" :
               inNotification == xpmp_PlaneNotification_ModelChanged ? "changed" : "destroyed");
    }
}

//
// MARK: Helper functions for position calculations
//

/// Distance of our simulated planes to the user's plane's position? [m]
constexpr float PLANE_DIST_M = 200.0f;
/// Radius of the circle the planes do [m]
constexpr float PLANE_RADIUS_M = 100.0f;
/// Altitude difference to stack the 3 planes one above the other [m]
constexpr float PLANE_STACK_ALT_M = 50.0f;
/// Time it shall take to fly/roll a full circle [seconds]
constexpr float PLANE_CIRCLE_TIME_S = 20.0f;
/// Time it shall take to fly/roll a full circle [minutes]
constexpr float PLANE_CIRCLE_TIME_MIN = PLANE_CIRCLE_TIME_S / 60.0f;
/// Assumed circumfence of one plane's tire (rough guess for commercial jet planes)
constexpr float PLANE_TIRE_CIRCUMFENCE_M = 3.2f;
/// Engine / prop rotation assumptions: rotations per minute
constexpr float PLANE_PROP_RPM = 300.0f;

/// PI
constexpr double PI         = 3.1415926535897932384626433832795028841971693993751;

/// Summarizes the 3 values of a position in the local coordinate system
struct positionTy {
    double x = 0.0f;
    double y = 0.0f;
    double z = 0.0f;
};

/// Position of user's plane
static XPLMDataRef dr_x = XPLMFindDataRef("sim/flightmodel/position/local_x");      // double
static XPLMDataRef dr_y = XPLMFindDataRef("sim/flightmodel/position/local_y");      // double
static XPLMDataRef dr_z = XPLMFindDataRef("sim/flightmodel/position/local_z");      // double
static XPLMDataRef dr_heading = XPLMFindDataRef("sim/flightmodel/position/psi");    // float
static XPLMDataRef dr_time = XPLMFindDataRef("sim/time/total_running_time_sec");    // float

/// Returns a number between 0.0 and 1.0, increasing over the course of 10 seconds, then restarting
inline float GetTimeFragment ()
{
    const float t = XPLMGetDataf(dr_time);
    return std::fmod(t, PLANE_CIRCLE_TIME_S) / PLANE_CIRCLE_TIME_S;
}

/// Returns a number between 0.0 and 1.0, going up and down over the course of 10 seconds
inline float GetTimeUpDown ()
{
    return std::abs(std::fmod(XPLMGetDataf(dr_time), PLANE_CIRCLE_TIME_S) / (PLANE_CIRCLE_TIME_S/2.0f) - 1.0f);
}

/// Convert from degree to radians
inline double deg2rad (const double deg) { return (deg * PI / 180.0); }

/// Save string copy
inline char* strScpy (char* dest, const char* src, size_t size)
{
    strncpy(dest, src, size);
    dest[size-1] = 0;               // this ensures zero-termination!
    return dest;
}

/// Finds a position 200m in front of the user's plane serving as the center for further operations
positionTy FindCenterPos (float dist)
{
    // Current user's plane's position and heading (relative to Z)
    positionTy pos = {
        XPLMGetDatad(dr_x),
        XPLMGetDatad(dr_y),
        XPLMGetDatad(dr_z)
    };
    float heading = XPLMGetDataf(dr_heading);
    
    // Move point 200m away from aircraft, direction of its heading
    const double head_rad = deg2rad(heading);
    pos.x += sin(head_rad) * dist;              // east axis
    pos.z -= cos(head_rad) * dist;              // south axis
    
    return pos;
}

/// Find ground altitude, i.e. change the position to be at ground altitude
void PlaceOnGround (positionTy& pos)
{
    // first call, don't have handle?
    static XPLMProbeRef hProbe = nullptr;
    if (!hProbe)
        hProbe = XPLMCreateProbe(xplm_ProbeY);
    
    // let the probe drop...
    XPLMProbeInfo_t probeInfo { sizeof(probeInfo), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    if (XPLMProbeTerrainXYZ(hProbe,
                            (float)pos.x,
                            (float)pos.y,
                            (float)pos.z,
                            &probeInfo) != xplm_ProbeError) {
        // transfer the result back
        pos.x = probeInfo.locationX;
        pos.y = probeInfo.locationY;
        pos.z = probeInfo.locationZ;
    }
}

/// Put the position on a circle around itself
void CirclePos (positionTy& pos,
                float heading,
                float radius)
{
    const double head_rad = deg2rad(heading);
    pos.x += radius * sin(head_rad);        // east axis
    pos.z -= radius * cos(head_rad);        // south axis
}

/// Convert local position to world coordinates
void ConvLocalToWorld (const positionTy& pos,
                       double& lat, double& lon, double& alt)
{
    XPLMLocalToWorld(pos.x, pos.y, pos.z,
                     &lat, &lon, &alt);
}

//
// MARK: Using XPMP2 - New XPMP2::Aircraft class
//       This is the new and recommended way of using the library:
//       Deriving a class from XPMP2::Aircraft and providing
//       a custom implementation for UpdatePosition(),
//       which provides all current values in one go directly into
//       the member variables, which are later on used for
//       controlling the plane objects. This avoids any unnecessary copying
//       within the library

using namespace XPMP2;

/// Subclassing XPMP2::Aircraft to create our own class
class SampleAircraft : public Aircraft
{
public:
    /// Constructor just passes on all parameters to library
    SampleAircraft(const std::string& _icaoType,
                   const std::string& _icaoAirline,
                   const std::string& _livery,
                   const std::string& _modelName = "") :
    Aircraft(_icaoType, _icaoAirline, _livery, _modelName)
    {
        // in our sample implementation, label, radar and info texts
        // are not dynamic. In others, they might be, then update them
        // in UpdatePosition()
        
        // Label
        label = "XPMP2::Aircraft";
        
        // Radar
        acRadar.code = 7654;
        acRadar.mode = xpmpTransponderMode_ModeC;
        
        // informational texts
        strScpy(acInfoTexts.icaoAcType, _icaoType.c_str(), sizeof(acInfoTexts.icaoAcType));
        strScpy(acInfoTexts.icaoAirline, _icaoAirline.c_str(), sizeof(acInfoTexts.icaoAirline));
    }
    
    /// Custom implementation for the virtual function providing updates values
    virtual void UpdatePosition ()
    {
        // Calculate the plane's position
        const float angle = std::fmod(360.0f * GetTimeFragment(), 360.0f);
        positionTy pos = FindCenterPos(PLANE_DIST_M);               // relative to user's plane
        CirclePos(pos, angle, PLANE_RADIUS_M);                      // turning around a circle
        pos.y += PLANE_STACK_ALT_M * 2;                             // 100m above user's aircraft

        // Strictly speaking...this is not necessary, we could just write
        // directly to drawInfo.x/y/z with above values (for y: + GetVertOfs()),
        // but typically in a real-world application you would actually
        // have lat/lon/elev...and then the call to SetLocation() is
        // what you shall do:
        double lat, lon, elev;
        // location in lat/lon/feet
        XPLMLocalToWorld(pos.x, pos.y, pos.z, &lat, &lon, &elev);
        elev /= M_per_FT;                   // we need elevation in feet
        
        // So, here we tell the plane its position, which takes care of vertical offset, too
        SetLocation(lat, lon, elev);
        
        // further attitude information
        drawInfo.pitch      =  0.0f;
        drawInfo.heading    = std::fmod(90.0f + angle, 360.0f);
        drawInfo.roll       = 20.0f;
        
        // Plane configuration info
        // This fills a large array of float values:
        v[V_CONTROLS_GEAR_RATIO]                    =
        v[V_CONTROLS_FLAP_RATIO]                    =
        v[V_CONTROLS_SPOILER_RATIO]                 =
        v[V_CONTROLS_SPEED_BRAKE_RATIO]             =
        v[V_CONTROLS_SLAT_RATIO]                    = GetTimeUpDown();
        v[V_CONTROLS_WING_SWEEP_RATIO]              = 0.0f;
        v[V_CONTROLS_THRUST_RATIO]                  = 0.5f;
        v[V_CONTROLS_YOKE_PITCH_RATIO]              =
        v[V_CONTROLS_YOKE_HEADING_RATIO]            =
        v[V_CONTROLS_YOKE_ROLL_RATIO]               = 0.0f;
        v[V_CONTROLS_THRUST_REVERS]                 = 0.0f;

        // lights
        v[V_CONTROLS_TAXI_LITES_ON]                 = 0.0f;
        v[V_CONTROLS_LANDING_LITES_ON]              = 0.0f;
        v[V_CONTROLS_BEACON_LITES_ON]               = 1.0f;
        v[V_CONTROLS_STROBE_LITES_ON]               = 1.0f;
        v[V_CONTROLS_NAV_LITES_ON]                  = 1.0f;

        // tires don't roll in the air
        v[V_GEAR_TIRE_VERTICAL_DEFLECTION_MTR]      =
        v[V_GEAR_TIRE_ROTATION_ANGLE_DEG]           =
        v[V_GEAR_TIRE_ROTATION_SPEED_RPM]           =
        v[V_GEAR_TIRE_ROTATION_SPEED_RAD_SEC]       = 0.0f;

        // For simplicity, we keep engine and prop rotation identical...probably unrealistic
        constexpr float PROP_REVOLUTIONS = PLANE_PROP_RPM * PLANE_CIRCLE_TIME_MIN;
        v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM]      =
        v[V_ENGINES_PROP_ROTATION_SPEED_RPM]        = PLANE_PROP_RPM;
        // We need to set engine/prop speed as both RPM and RAD values...that's a bit inconvenient
        v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC]  =
        v[V_ENGINES_PROP_ROTATION_SPEED_RAD_SEC]    = PLANE_PROP_RPM * RPM_to_RADs;
        // Current position of engine / prop: keeps turning as per engine/prop speed:
        v[V_ENGINES_ENGINE_ROTATION_ANGLE_DEG]      =
        v[V_ENGINES_PROP_ROTATION_ANGLE_DEG]        = std::fmod(PROP_REVOLUTIONS * GetTimeFragment() * 360.0f,
                                                                360.0f);
        // no reversers and no moment of touch-down in flight
        v[V_ENGINES_THRUST_REVERSER_DEPLOY_RATIO]   = 0.0f;
        v[V_MISC_TOUCH_DOWN]                        = 0.0f;
    }

};

/// The one aircraft of this type that we manage
SampleAircraft* pSamplePlane = nullptr;


//
// MARK: Using XPMP2 - Legacy XPCAircraft class
//       XPCAircraft was a wrapper class offered in the original library
//       already. It now is derived from XPMP2's main class,
//       XPMP2::Aircraft, to provide the same interface as before.
//       Still, it is deprecated and should not be used in new applications.
//       Derive directly from XPMP2::Aircraft instead.
//
//       This plane will be 50m higher than user's plane,
//       circling 200m in front of user's plane.
//

/// Subclassing XPCAircraft to create our own class
class LegacySampleAircraft : public XPCAircraft
{
public:
    /// Constructor just passes on all parameters to library
    LegacySampleAircraft(const char* inICAOCode,
                         const char* inAirline,
                         const char* inLivery,
                         const char* inModelName = nullptr) :
    XPCAircraft(inICAOCode, inAirline, inLivery, inModelName) {}
    
    // My own class overwrites the individual data provision functions
    
    /// Called before rendering to query plane's current position, overwritten to provide your implementation
    virtual XPMPPlaneCallbackResult GetPlanePosition(XPMPPlanePosition_t* outPosition)
    {
        // Calculate the plane's position
        const float angle = std::fmod(120.0f + 360.0f * GetTimeFragment(), 360.0f);
        positionTy pos = FindCenterPos(PLANE_DIST_M);               // relative to user's plane
        CirclePos(pos, angle, PLANE_RADIUS_M);                      // turning around a circle
        pos.y += PLANE_STACK_ALT_M;                                 // 50m above user's aircraft

        // fill the XPMP2 data structure
        
        // The size is pre-filled and shall support version differences.
        // We make the check simple here and only proceed if the library
        // has at least as much storage as we expected for everything:
        if (outPosition->size < (long)sizeof(XPMPPlanePosition_t))
            return xpmpData_Unavailable;
        
        // location in lat/lon/feet
        XPLMLocalToWorld(pos.x, pos.y, pos.z,
                         &outPosition->lat,
                         &outPosition->lon,
                         &outPosition->elevation);      // elevation is now given in meter
        outPosition->elevation /= M_per_FT;             // put it is expected in feet!
        
        outPosition->pitch          = 0.0f;
        outPosition->roll           =20.0f;             // rolled 20° right (tight curve!)
        outPosition->heading        = std::fmod(90.0f + angle, 360.0f);
        strcpy ( outPosition->label, "XPCAircraft subclassed");
        outPosition->offsetScale    = 1.0f;             // so that full vertical offset is applied and plane sits on its wheels (should probably always be 1.0)
        outPosition->clampToGround  = true;
        outPosition->aiPrio         = 1;
        outPosition->label_color[0] = 1.0f;             // yellow
        outPosition->label_color[1] = 1.0f;
        outPosition->label_color[2] = 0.0f;
        outPosition->label_color[3] = 1.0f;
        return xpmpData_NewData;
    }
    
    /// Called before rendering to query plane's current configuration, overwritten to provide your implementation
    virtual XPMPPlaneCallbackResult GetPlaneSurfaces(XPMPPlaneSurfaces_t* outSurfaces)
    {
        // The size is pre-filled and shall support version differences.
        // We make the check simple here and only proceed if the library
        // has at least as much storage as we expected for everything:
        if (outSurfaces->size < (long)sizeof(XPMPPlaneSurfaces_t))
            return xpmpData_Unavailable;

        // gear & flight surfaces keep moving for show
        outSurfaces->gearPosition       =
        outSurfaces->flapRatio          =
        outSurfaces->spoilerRatio       =
        outSurfaces->speedBrakeRatio    =
        outSurfaces->slatRatio          = GetTimeUpDown();
        outSurfaces->thrust             = 0.5f;
        outSurfaces->yokePitch          = 0.0f;
        outSurfaces->yokeHeading        = 0.0f;
        outSurfaces->yokeRoll           = 0.0f;
        
        // lights: taxi, beacon, and nav lights
        outSurfaces->lights.timeOffset  = 0;            // unused in XPMP2
        outSurfaces->lights.taxiLights  = 1;
        outSurfaces->lights.landLights  = 1;
        outSurfaces->lights.bcnLights   = 1;
        outSurfaces->lights.strbLights  = 1;
        outSurfaces->lights.navLights   = 1;
        outSurfaces->lights.flashPattern = xpmp_Lights_Pattern_Default; // unused in XPMP2
        
        // tires don't roll in the air
        outSurfaces->tireDeflect        = 0;
        outSurfaces->tireRotDegree      = 0;
        outSurfaces->tireRotRpm         = 0;
        
        // For simplicity, we keep engine and prop rotation identical...probably unrealistic
        constexpr float PROP_REVOLUTIONS = PLANE_PROP_RPM * PLANE_CIRCLE_TIME_MIN;
        outSurfaces->engRotRpm          =
        outSurfaces->propRotRpm         = PLANE_PROP_RPM;
        outSurfaces->engRotDegree       =
        outSurfaces->propRotDegree      = std::fmod(PROP_REVOLUTIONS * GetTimeFragment() * 360.0f,
                                                    360.0f);
        // no reversers in flight
        outSurfaces->reversRatio        = 0.0f;
        
        outSurfaces->touchDown          = false;
        
        return xpmpData_NewData;
    }
    
    /// Called before rendering to query plane's current radar visibility, overwritten to provide your implementation
    virtual XPMPPlaneCallbackResult GetPlaneRadar(XPMPPlaneRadar_t* outRadar)
    {
        // The size is pre-filled and shall support version differences.
        // We make the check simple here and only proceed if the library
        // has at least as much storage as we expected for everything:
        if (outRadar->size < (long)sizeof(XPMPPlaneRadar_t))
            return xpmpData_Unavailable;

        if (outRadar->code != 4711) {
            outRadar->code               = 4711;
            outRadar->mode               = xpmpTransponderMode_ModeC;
            return xpmpData_NewData;
        }
        else
            return xpmpData_Unchanged;
    }

    /// @brief Called before rendering to query plane's textual information, overwritten to provide your implementation (optional)
    /// @details Handling this requests is completely optional. The texts are
    ///          provided on shared dataRefs and used only by few other plugins,
    ///          one of it is FSTramp.\n
    ///          Here in the example we keep it simple and just return some known data.
    virtual XPMPPlaneCallbackResult GetInfoTexts(XPMPInfoTexts_t * outInfoTexts)
    {
        // The size is pre-filled and shall support version differences.
        // We make the check simple here and only proceed if the library
        // has at least as much storage as we expected for everything:
        if (outInfoTexts->size < (long)sizeof(XPMPInfoTexts_t))
            return xpmpData_Unavailable;
        
        if (acIcaoType      != outInfoTexts->icaoAcType ||
            acIcaoAirline   != outInfoTexts->icaoAirline) {
            strScpy(outInfoTexts->icaoAcType,   acIcaoType.c_str(),     sizeof(outInfoTexts->icaoAcType));
            strScpy(outInfoTexts->icaoAirline,  acIcaoAirline.c_str(),  sizeof(outInfoTexts->icaoAirline));
            return xpmpData_NewData;
        }
        else
            return xpmpData_Unchanged;
    }

};

/// The one aircraft of this type that we manage
LegacySampleAircraft* pLegacyPlane = nullptr;

//
// MARK: Using XPMP2 - Standard C Functions
//       This plane will always be on the ground, ie. its altitude is
//       calculated to be on ground level, gear is down.
//       The plane's label for display is "Standard C" in yellow.
//

/// We handle just one aircraft with standard functions, this one:
XPMPPlaneID hStdPlane = NULL;

/// @brief Handles requests for plane's position data
/// @see LegacySampleAircraft::GetPlanePosition(), which basically is the very same thing.
XPMPPlaneCallbackResult SetPositionData (XPMPPlanePosition_t& data)
{
    // Calculate the plane's position
    const float angle = std::fmod(240.0f + 360.0f * GetTimeFragment(), 360.0f);
    positionTy pos = FindCenterPos(PLANE_DIST_M);               // relative to user's plane
    CirclePos(pos, angle, PLANE_RADIUS_M);                      // turning around a circle
    PlaceOnGround(pos);                                         // this one is always on the ground

    // fill the XPMP2 data structure
    
    // The size is pre-filled and shall support version differences.
    // We make the check simple here and only proceed if the library
    // has at least as much storage as we expected for everything:
    if (data.size < (long)sizeof(XPMPPlanePosition_t))
        return xpmpData_Unavailable;
    
    // location in lat/lon/feet
    XPLMLocalToWorld(pos.x, pos.y, pos.z,
                     &data.lat,
                     &data.lon,
                     &data.elevation);      // elevation is now given in meter
    data.elevation /= M_per_FT;             // put it is expected in feet!
    
    data.pitch          = 0.0f;             // plane stays level
    data.roll           = 0.0f;
    data.heading        = std::fmod(90.0f + angle, 360.0f);
    strcpy ( data.label, "Standard C");
    data.offsetScale    = 1.0f;             // so that full vertical offset is applied and plane sits on its wheels (should probably always be 1.0)
    data.clampToGround  = true;
    data.aiPrio         = 1;
    data.label_color[0] = 1.0f;             // yellow
    data.label_color[1] = 1.0f;
    data.label_color[2] = 0.0f;
    data.label_color[3] = 1.0f;
    return xpmpData_NewData;
}

/// @brief Handles requests for plane's surface data
/// @see LegacySampleAircraft::GetPlaneSurfaces(), which basically is the very same thing.
XPMPPlaneCallbackResult SetSurfaceData (XPMPPlaneSurfaces_t& data)
{
    // The size is pre-filled and shall support version differences.
    // We make the check simple here and only proceed if the library
    // has at least as much storage as we expected for everything:
    if (data.size < (long)sizeof(XPMPPlaneSurfaces_t))
        return xpmpData_Unavailable;

    // gear & flight surfaces
    data.gearPosition       = 1.0;          // gear is always down
    data.flapRatio          =               // flight surfaces cycle up and down
    data.spoilerRatio       =
    data.speedBrakeRatio    =
    data.slatRatio          = GetTimeUpDown();
    data.thrust             = 0.2f;
    data.yokePitch          = 0.0f;
    data.yokeHeading        = 0.0f;
    data.yokeRoll           = 0.0f;
    
    // lights: taxi, beacon, and nav lights
    data.lights.timeOffset  = 0;            // unused in XPMP2
    data.lights.taxiLights  = 1;
    data.lights.landLights  = 0;
    data.lights.bcnLights   = 1;
    data.lights.strbLights  = 0;
    data.lights.navLights   = 1;
    data.lights.flashPattern = xpmp_Lights_Pattern_Default; // unused in XPMP2
    
    // tires (assuming a tire circumfence of 3.2m and a circumfence of 628m of the
    // circle the plane moves around a center position we try to simulate
    // more or less accurate tire rolling, so the tire turns 196 times for
    // a full plane's circle, which in turn shall take 10s.
    constexpr float ROLL_CIRCUMFENCE = float(2.0 * PI * PLANE_RADIUS_M);
    constexpr float TIRE_REVOLUTIONS = ROLL_CIRCUMFENCE / PLANE_TIRE_CIRCUMFENCE_M;
    data.tireDeflect        = 0;
    data.tireRotDegree      = std::fmod(TIRE_REVOLUTIONS * GetTimeFragment() * 360.0f,
                                        360.0f);
    data.tireRotRpm         = TIRE_REVOLUTIONS / PLANE_CIRCLE_TIME_MIN;
    
    // For simplicity, we keep engine and prop rotation identical...probably unrealistic
    constexpr float PROP_REVOLUTIONS = PLANE_PROP_RPM * PLANE_CIRCLE_TIME_MIN;
    data.engRotRpm          =
    data.propRotRpm         = PLANE_PROP_RPM;
    data.engRotDegree       =
    data.propRotDegree      = std::fmod(PROP_REVOLUTIONS * GetTimeFragment() * 360.0f,
                                        360.0f);
    // for the show of it we open/close reversers
    data.reversRatio        = GetTimeUpDown();
    
    // Some models produces tire smoke at the moment of touch down,
    // so at 0° we tell the model we would touch down right now
    data.touchDown          = GetTimeFragment() <= 0.05f;
    
    return xpmpData_NewData;
}

/// @brief Handles requests for plane's radar data (doesn't actually change over time)
/// @see LegacySampleAircraft::GetPlaneRadar(), which basically is the very same thing.
XPMPPlaneCallbackResult SetRadarData (XPMPPlaneRadar_t& data)
{
    // The size is pre-filled and shall support version differences.
    // We make the check simple here and only proceed if the library
    // has at least as much storage as we expected for everything:
    if (data.size < (long)sizeof(XPMPPlaneRadar_t))
        return xpmpData_Unavailable;

    if (data.code != 1234) {
        data.code               = 1234;
        data.mode               = xpmpTransponderMode_ModeC;
        return xpmpData_NewData;
    } else
        return xpmpData_Unchanged;
}

/// @brief Handles requests for plane's informational texts
/// @details Handling this requests is completely optional. The texts are
///          provided on shared dataRefs and used only by few other plugins,
///          one of it is FSTramp.\n
///          Here in the example we keep it simple and just return the ICAO plane type.
/// @see LegacySampleAircraft::GetInfoTexts(), which basically is the very same thing.
XPMPPlaneCallbackResult SetInfoData (XPMPInfoTexts_t& data)
{
    // The size is pre-filled and shall support version differences.
    // We make the check simple here and only proceed if the library
    // has at least as much storage as we expected for everything:
    if (data.size < (long)sizeof(XPMPInfoTexts_t))
        return xpmpData_Unavailable;
    
    XPMPGetPlaneICAOAndLivery(hStdPlane,        // get ICAO type from XPMP2
                              data.icaoAcType,
                              NULL);
    return xpmpData_NewData;
}


/// Callback function handed to XPMP2, will be called in every drawing frame to deliver plane position and configuration
XPMPPlaneCallbackResult CBPlaneData (XPMPPlaneID         inPlane,
                                     XPMPPlaneDataType   inDataType,
                                     void *              ioData,
                                     void *              /* inRefcon */)
{
    // sanity check: our plane?
    if (inPlane != hStdPlane) return xpmpData_Unavailable;
    
    // There is 4 requests to deal with
    switch (inDataType) {
        case xpmpDataType_Position:
            return SetPositionData (*(XPMPPlanePosition_t*)ioData);
        case xpmpDataType_Surfaces:
            return SetSurfaceData(*(XPMPPlaneSurfaces_t*)ioData);
        case xpmpDataType_Radar:
            return SetRadarData(*(XPMPPlaneRadar_t*)ioData);
        case xpmpDataType_InfoTexts:
            return SetInfoData(*(XPMPInfoTexts_t*)ioData);
        default:
            return xpmpData_Unavailable;
    }
}

//
// MARK: Menu functionality
//

/// menu id of our plugin's menu
XPLMMenuID hMenu = nullptr;

/// Planes currently visible?
bool gbVisible = true;

/// for cycling CSL models: what is the index used for the first plane?
int gModelIdxBase = 0;

/// Is any plane object created?
inline bool ArePlanesCreated () { return pSamplePlane || pLegacyPlane || hStdPlane; }

/// Create our 3 planes (if they don't exist already)
void PlanesCreate ()
{
 /*   // 1. New interface of XPMP2::Aircraft class
    if (!pSamplePlane) try {
        pSamplePlane = new SampleAircraft(PLANE_MODEL[gModelIdxBase][0],  // type
                                          PLANE_MODEL[gModelIdxBase][1],  // airline
                                          PLANE_MODEL[gModelIdxBase][2]); // livery
    }
    catch (const XPMP2::XPMP2Error& e) {
        LogMsg("Could not create object of type SampleAircraft: %s", e.what());
        pSamplePlane = nullptr;
    }
*/
    // 2. Subclassing the Legacy XPCAircraft class
    
    // Creating the plane can now (this is new in XPMP2) throw an exception
    if (!pLegacyPlane) try {
        pLegacyPlane = new LegacySampleAircraft(PLANE_MODEL[(gModelIdxBase+1)%3][0].c_str(),  // type
                                                PLANE_MODEL[(gModelIdxBase+1)%3][1].c_str(),  // airline
                                                PLANE_MODEL[(gModelIdxBase+1)%3][2].c_str()); // livery
    }
    catch (const XPMP2::XPMP2Error& e) {
        LogMsg("Could not create object of type LegacySampleAircraft: %s", e.what());
        pLegacyPlane = nullptr;
    }
    
    // 3. Using Standard Legacy C interface
    if (!hStdPlane)
        hStdPlane = XPMPCreatePlane(PLANE_MODEL[(gModelIdxBase+2)%3][0].c_str(),  // type
                                    PLANE_MODEL[(gModelIdxBase+2)%3][1].c_str(),  // airline
                                    PLANE_MODEL[(gModelIdxBase+2)%3][2].c_str(),  // livery
                                    CBPlaneData, NULL);
    
    // Put a checkmark in front of menu item if planes are visible
    XPLMCheckMenuItem(hMenu, 0, ArePlanesCreated()  ? xplm_Menu_Checked : xplm_Menu_Unchecked);
    XPLMCheckMenuItem(hMenu, 1, gbVisible           ? xplm_Menu_Checked : xplm_Menu_Unchecked);
}

/// Remove all planes
void PlanesRemove ()
{
    if (pSamplePlane) {
        delete pSamplePlane;
        pSamplePlane = nullptr;
    }
    
    if (pLegacyPlane) {
        delete pLegacyPlane;
        pLegacyPlane = nullptr;
    }
    
    if (hStdPlane) {
        XPMPDestroyPlane(hStdPlane);
        hStdPlane = nullptr;
    }

    // Remove the checkmark in front of menu item
    XPLMCheckMenuItem(hMenu, 0, xplm_Menu_Unchecked);
    XPLMCheckMenuItem(hMenu, 1, xplm_Menu_Unchecked);
}

/// Show/hide the planes (temporarily, without destroying the plane objects)
void PlanesShowHide ()
{
    gbVisible = !gbVisible;             // toggle setting
    if (pSamplePlane)
        pSamplePlane->SetVisible(gbVisible);
    if (pLegacyPlane)
        pLegacyPlane->SetVisible(gbVisible);
    if (hStdPlane)
        XPMPSetPlaneVisibility(hStdPlane, gbVisible);

    // Put a checkmark in front of menu item if planes are visible
    XPLMCheckMenuItem(hMenu, 1, gbVisible           ? xplm_Menu_Checked : xplm_Menu_Unchecked);
}

/// Cycle the CSL models of the 3 planes
void PlanesCycleModels ()
{
    // increase the index into our list of CSL models
    // (this is the index used by the first plane, the XPMP2::Aircraft one)
    gModelIdxBase = (gModelIdxBase + 1) % 3;
    
    // Now apply the new model to the 3 planes
    if (pSamplePlane)
        pSamplePlane->ChangeModel(PLANE_MODEL[gModelIdxBase][0],  // type
                                  PLANE_MODEL[gModelIdxBase][1],  // airline
                                  PLANE_MODEL[gModelIdxBase][2]); // livery
    if (pLegacyPlane)
        pLegacyPlane->ChangeModel(PLANE_MODEL[(gModelIdxBase+1)%3][0],  // type
                                  PLANE_MODEL[(gModelIdxBase+1)%3][1],  // airline
                                  PLANE_MODEL[(gModelIdxBase+1)%3][2]); // livery
    if (hStdPlane)
        XPMPChangePlaneModel(hStdPlane,
                             PLANE_MODEL[(gModelIdxBase+2)%3][0].c_str(),  // type
                             PLANE_MODEL[(gModelIdxBase+2)%3][1].c_str(),  // airline
                             PLANE_MODEL[(gModelIdxBase+2)%3][2].c_str()); // livery
}

/// Callback function for menu
void CBMenu (void* /*inMenuRef*/, void* inItemRef)
{
    // Toggle plane visibility?
    if (inItemRef == (void*)1)
    {
        if (ArePlanesCreated())
            PlanesRemove();
        else
            PlanesCreate();
    }
    // Show/Hide Planes?
    else if (inItemRef == (void*)2)
    {
        PlanesShowHide();
    }
    // Cycle Models?
    else if (inItemRef == (void*)3)
    {
        PlanesCycleModels();
    }
    
}

//
// MARK: Standard Plugin Callbacks
//

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
	std::strcpy(outName, "XPMP2-Sample");
	std::strcpy(outSig, "TwinFan.plugin.XPMP2-Sample");
	std::strcpy(outDesc, "Sample plugin demonstrating using XPMP2 library");
    
    // Create the menu for the plugin
    int my_slot = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "XPMP2 Sample", NULL, 0);
    hMenu = XPLMCreateMenu("XPMP2 Sample", XPLMFindPluginsMenu(), my_slot, CBMenu, NULL);
    XPLMAppendMenuItem(hMenu, "Toggle Planes",      (void*)1, 0);
    XPLMAppendMenuItem(hMenu, "Toggle Visibility",  (void*)2, 0);
    XPLMAppendMenuItem(hMenu, "Cycle Models",       (void*)3, 0);
    
	return 1;
}

PLUGIN_API void	XPluginStop(void)
{
}

PLUGIN_API int XPluginEnable(void)
{
    // The path separation character, one out of /\:
    char pathSep = XPLMGetDirectorySeparator()[0];
    // The plugin's path, results in something like ".../Resources/plugins/XPMP2-Sample/64/XPMP2-Sample.xpl"
    char szPath[256];
    XPLMGetPluginInfo(XPLMGetMyID(), nullptr, szPath, nullptr, nullptr);
    *(std::strrchr(szPath, pathSep)) = 0;   // Cut off the plugin's file name
    *(std::strrchr(szPath, pathSep)+1) = 0; // Cut off the "64" directory name, but leave the dir separation character
    // We search in a subdirectory named "Resources" for all we need
    std::string resourcePath = szPath;
    resourcePath += "Resources";            // should now be something like ".../Resources/plugins/XPMP2-Sample/Resources"

    // Try initializing XPMP2. Although the function is "...Legacy..."
    // it actually includes all infos in just one go...
    const char *res = XPMPMultiplayerInitLegacyData (resourcePath.c_str(),
                                                     (resourcePath + pathSep + "related.txt").c_str(),
                                                     nullptr,       // TexturePath is no longer used
                                                     (resourcePath + pathSep + "Doc8643.txt").c_str(),
                                                     "C172",        // default model ICAO
                                                     CBIntPrefsFunc);
    if (res[0]) {
        LogMsg("XPMP2-Sample: Initialization of XPMP2 failed: %s", res);
        return 0;
    }
    
    // Now we also try to get control of AI planes. That's optional, though,
    // other plugins (like LiveTraffic, XSquawkBox, X-IvAp...)
    // could have control already
    res = XPMPMultiplayerEnable();
    if (res[0]) {
        LogMsg("XPMP2-Sample: Could not enable AI planes: %s", res);
    }
    
    // Register the plane notifer function
    // (this is rarely used in actual applications, but used here for
    //  demonstration and testing purposes)
    XPMPRegisterPlaneNotifierFunc(CBPlaneNotifier, NULL);
    
    // *** Create the planes ***
    PlanesCreate();

    // Success
    LogMsg("XPMP2-Sample: Enabled");
	return 1;
}

PLUGIN_API void XPluginDisable(void)
{
    // Remove the planes
    PlanesRemove();
    
    // Give up AI plane control
    XPMPMultiplayerDisable();
        
    // Unregister plane notifier (must match function _and_ refcon)
    XPMPUnregisterPlaneNotifierFunc(CBPlaneNotifier, NULL);
    
    // Properly cleanup the XPMP2 library
    XPMPMultiplayerCleanup();
    
    LogMsg("XPMP2-Sample: Disabled");
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID, long, void*)
{
}
