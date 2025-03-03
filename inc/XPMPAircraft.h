/// @file       XPMPAircraft.h
/// @brief      XPMP2::Aircraft represent an aircraft as managed by XPMP2
/// @details    New implementations should derive directly from XPMP2::Aircraft.
///
/// @details    This is one of two main header files for using XPMP2.
///             (The other is `XPMPMultiplayer.h`).
///             XPMP2 is a library allowing an X-Plane plugin to have
///             planes rendered in X-Plane's 3D world based on OBJ8
///             CSL models, which need to be installed separately.
///             The plugin shall subclass XPMP2::Aircraft and override
///             the abstract virtual function XPMP2::Aircraft::UpdatePosition()
///             to provide updated position and attitude information.
///             XPMP2 takes care of reading and initializaing CSL models,
///             instanciating and updating the aircraft objects in X-Plane,
///             display in a map layer, provisioning information via X-Plane's
///             TCAS targets and AI/multiplayer (and more) dataRefs.
///
/// @see        For more developer's information see
///             https://twinfan.github.io/XPMP2/
///
/// @see        Sample and "How to" available, see
///             https://twinfan.github.io/XPMP2/HowTo.html
///
/// @see        For TCAS Override approach see
///             https://developer.x-plane.com/article/overriding-tcas-and-providing-traffic-information/
///
/// @see        For a definition of ICAO aircraft type designators see
///             https://www.icao.int/publications/DOC8643/Pages/Search.aspx
///
/// @see        For a list of ICAO airline/operator codes see
///             https://en.wikipedia.org/wiki/List_of_airline_codes
///
/// @author     Birger Hoppe
/// @copyright  (c) 2020-2022 Birger Hoppe
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

#ifndef _XPMPAircraft_h_
#define _XPMPAircraft_h_

#include "XPMPMultiplayer.h"
#include "XPLMInstance.h"
#include "XPLMCamera.h"
#include "XPLMMap.h"

#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>
#include <list>

//
// MARK: XPMP2 New Definitions
//

namespace XPMP2 {

class CSLModel;                                 ///< Defined by XPMP2 internally

/// Convert revolutions-per-minute (RPM) to radians per second (rad/s) by multiplying with PI/30
constexpr float RPM_to_RADs = 0.10471975511966f;
/// Convert feet to meters, e.g. for altitude calculations
constexpr double M_per_FT   = 0.3048;   // meter per 1 foot
/// Convert nautical miles to meters
constexpr int M_per_NM      = 1852;     // meter per one nautical mile
/// Convert m/s to knots
constexpr double KT_per_M_per_S = 1.94384;  // 1m/s = 1.94384kt
/// @brief standard gravitational acceleration [m/s²]
/// @see https://en.wikipedia.org/wiki/Gravity_of_Earth
constexpr float G_EARTH     = 9.80665f;

/// The dataRefs provided by XPMP2 to the CSL models
enum DR_VALS : std::uint8_t {
    V_CONTROLS_GEAR_RATIO = 0,                  ///< `libxplanemp/controls/gear_ratio` and \n`sim/cockpit2/tcas/targets/position/gear_deploy`
    V_CONTROLS_NWS_RATIO,                       ///< `libxplanemp/controls/nws_ratio`, the nose wheel angle, actually in degrees
    V_CONTROLS_FLAP_RATIO,                      ///< `libxplanemp/controls/flap_ratio` and \n`sim/cockpit2/tcas/targets/position/flap_ratio` and `...flap_ratio2`
    V_CONTROLS_SPOILER_RATIO,                   ///< `libxplanemp/controls/spoiler_ratio`
    V_CONTROLS_SPEED_BRAKE_RATIO,               ///< `libxplanemp/controls/speed_brake_ratio` and \n`sim/cockpit2/tcas/targets/position/speedbrake_ratio`
    V_CONTROLS_SLAT_RATIO,                      ///< `libxplanemp/controls/slat_ratio` and \n`sim/cockpit2/tcas/targets/position/slat_ratio`
    V_CONTROLS_WING_SWEEP_RATIO,                ///< `libxplanemp/controls/wing_sweep_ratio` and \n`sim/cockpit2/tcas/targets/position/wing_sweep`
    V_CONTROLS_THRUST_RATIO,                    ///< `libxplanemp/controls/thrust_ratio` and \n`sim/cockpit2/tcas/targets/position/throttle`
    V_CONTROLS_YOKE_PITCH_RATIO,                ///< `libxplanemp/controls/yoke_pitch_ratio` and \n`sim/cockpit2/tcas/targets/position/yolk_pitch`
    V_CONTROLS_YOKE_HEADING_RATIO,              ///< `libxplanemp/controls/yoke_heading_ratio` and \n`sim/cockpit2/tcas/targets/position/yolk_yaw`
    V_CONTROLS_YOKE_ROLL_RATIO,                 ///< `libxplanemp/controls/yoke_roll_ratio` and \n`sim/cockpit2/tcas/targets/position/yolk_roll`
    V_CONTROLS_THRUST_REVERS,                   ///< `libxplanemp/controls/thrust_revers`
    
    V_CONTROLS_TAXI_LITES_ON,                   ///< `libxplanemp/controls/taxi_lites_on` and \n`sim/cockpit2/tcas/targets/position/lights`
    V_CONTROLS_LANDING_LITES_ON,                ///< `libxplanemp/controls/landing_lites_on` and \n`sim/cockpit2/tcas/targets/position/lights`
    V_CONTROLS_BEACON_LITES_ON,                 ///< `libxplanemp/controls/beacon_lites_on` and \n`sim/cockpit2/tcas/targets/position/lights`
    V_CONTROLS_STROBE_LITES_ON,                 ///< `libxplanemp/controls/strobe_lites_on` and \n`sim/cockpit2/tcas/targets/position/lights`
    V_CONTROLS_NAV_LITES_ON,                    ///< `libxplanemp/controls/nav_lites_on` and \n`sim/cockpit2/tcas/targets/position/lights`
    
    V_GEAR_NOSE_GEAR_DEFLECTION_MTR,            ///< `libxplanemp/gear/nose_gear_deflection_mtr`
    V_GEAR_TIRE_VERTICAL_DEFLECTION_MTR,        ///< `libxplanemp/gear/tire_vertical_deflection_mtr`
    V_GEAR_TIRE_ROTATION_ANGLE_DEG,             ///< `libxplanemp/gear/tire_rotation_angle_deg`
    V_GEAR_TIRE_ROTATION_SPEED_RPM,             ///< `libxplanemp/gear/tire_rotation_speed_rpm`
    V_GEAR_TIRE_ROTATION_SPEED_RAD_SEC,         ///< `libxplanemp/gear/tire_rotation_speed_rad_sec`
    
    V_ENGINES_ENGINE_ROTATION_ANGLE_DEG,        ///< `libxplanemp/engines/engine_rotation_angle_deg`
    V_ENGINES_ENGINE_ROTATION_SPEED_RPM,        ///< `libxplanemp/engines/engine_rotation_speed_rpm`
    V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC,    ///< `libxplanemp/engines/engine_rotation_speed_rad_sec`
    V_ENGINES_PROP_ROTATION_ANGLE_DEG,          ///< `libxplanemp/engines/prop_rotation_angle_deg`
    V_ENGINES_PROP_ROTATION_SPEED_RPM,          ///< `libxplanemp/engines/prop_rotation_speed_rpm`
    V_ENGINES_PROP_ROTATION_SPEED_RAD_SEC,      ///< `libxplanemp/engines/prop_rotation_speed_rad_sec`
    V_ENGINES_THRUST_REVERSER_DEPLOY_RATIO,     ///< `libxplanemp/engines/thrust_reverser_deploy_ratio`
    
    V_ENGINES_ENGINE_ROTATION_ANGLE_DEG1,       ///< `libxplanemp/engines/engine_rotation_angle_deg1`
    V_ENGINES_ENGINE_ROTATION_ANGLE_DEG2,       ///< `libxplanemp/engines/engine_rotation_angle_deg2`
    V_ENGINES_ENGINE_ROTATION_ANGLE_DEG3,       ///< `libxplanemp/engines/engine_rotation_angle_deg3`
    V_ENGINES_ENGINE_ROTATION_ANGLE_DEG4,       ///< `libxplanemp/engines/engine_rotation_angle_deg4`
    V_ENGINES_ENGINE_ROTATION_SPEED_RPM1,       ///< `libxplanemp/engines/engine_rotation_speed_rpm1`
    V_ENGINES_ENGINE_ROTATION_SPEED_RPM2,       ///< `libxplanemp/engines/engine_rotation_speed_rpm2`
    V_ENGINES_ENGINE_ROTATION_SPEED_RPM3,       ///< `libxplanemp/engines/engine_rotation_speed_rpm3`
    V_ENGINES_ENGINE_ROTATION_SPEED_RPM4,       ///< `libxplanemp/engines/engine_rotation_speed_rpm4`
    V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC1,   ///< `libxplanemp/engines/engine_rotation_speed_rad_sec1`
    V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC2,   ///< `libxplanemp/engines/engine_rotation_speed_rad_sec2`
    V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC3,   ///< `libxplanemp/engines/engine_rotation_speed_rad_sec3`
    V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC4,   ///< `libxplanemp/engines/engine_rotation_speed_rad_sec4`

    
    V_MISC_TOUCH_DOWN,                          ///< `libxplanemp/misc/touch_down`
    
    V_COUNT                                     ///< always last, number of dataRefs XPMP2 pre-defines
};

/// @brief Collates some information on the CSL model
/// @details The XPMP2::CSLModel class definition is private to the XPMP2 library
///          as it contains many technical implementation details.
///          This structure contains some of the CSLModel information in a public
///          definition, returned by XPMP2::Aircraft::GetModelInfo().
struct XPMP2_EXPORT CSLModelInfo_t {
    /// id, just an arbitrary label read from `xsb_aircraft.txt::OBJ8_AIRCRAFT`
    std::string         cslId;
    /// name, formed by last part of path plus id
    std::string         modelName;
    /// Path to the xsb_aircraft.txt file from where this model is loaded
    std::string         xsbAircraftPath;
    /// Line number in the xsb_aircraft.txt file where the model definition starts
    int                 xsbAircraftLn = 0;
    /// ICAO aircraft type this model represents: `xsb_aircraft.txt::ICAO`
    std::string         icaoType;
    /// Doc8643 information: Classification, like L1P, L4J, H1T
    std::string         doc8643Classification;
    /// Doc8643 information: wake turbulence class, like M, L/M, L, H
    std::string         doc8643WTC;

    /// Any number of airline codes and/or liveries can be assigned to a model for matching purpose
    struct MatchCrit_t {
        std::string     icaoAirline;    ///< ICAO airine/operator code
        std::string     livery;         ///< special livery (not often used)
    };
    typedef std::vector<MatchCrit_t> vecMatchCrit_t;
    // List of match criteria defined for the model, can be empty
    vecMatchCrit_t      vecMatchCrit;

    /// Default constructor does nothing
    CSLModelInfo_t() {}
    /// Constructor copies from XPMP2::CSLModel
    CSLModelInfo_t(const XPMP2::CSLModel& csl);
};

/// @brief Actual representation of all aircraft in XPMP2.
/// @note In modern implementations, this class shall be subclassed by your plugin's code.
class XPMP2_EXPORT Aircraft {
    
public:
    
protected:
    /// @brief A plane is uniquely identified by a 24bit number [0x01..0xFFFFFF]
    /// @details This number is directly used as `modeS_id` n the new
    ///          [TCAS override](https://developer.x-plane.com/article/overriding-tcas-and-providing-traffic-information/)
    ///          approach.
    XPMPPlaneID modeS_id = 0;
    
public:
    /// @brief ICAO aircraft type designator of this plane
    /// @see https://www.icao.int/publications/DOC8643/Pages/Search.aspx
    std::string acIcaoType;
    std::string acIcaoAirline;          ///< ICAO Airline code of this plane
    std::string acLivery;               ///< Livery code of this plane
    
    /// @brief Holds position (in local coordinates!) and orientation (pitch, heading roll) of the aircraft.
    /// @details This is where the plane will be placed in this drawing cycle.\n
    /// @note    When filling `y` directly (instead of using SetLocation()) remember to add
    ///          GetVertOfs() for accurate placement on the ground
    XPLMDrawInfo_t drawInfo;
    
    /// Cartesian velocity in m/s per axis, updated at least once per second
    float               v_x = 0.0f, v_y = 0.0f, v_z = 0.0f;

    /// Is the aircraft on the ground?
    bool        bOnGrnd = false;

    /// @brief actual dataRef values to be provided to the CSL model
    /// @details XPMP2 provides a minimum set of dataRefs and also getter/setter
    ///          member functions, see below. This is the one place where
    ///          current dataRef values are stored. This array is passed on
    ///          _directly_ to the XP instance.\n
    ///          The size of the vector can increase if adding user-defined
    ///          dataRefs through XPMPAddModelDataRef().
    std::vector<float> v;
    
    /// aircraft label shown in the 3D world next to the plane
    std::string label;
    float       colLabel[4]  = {1.0f,1.0f,0.0f,1.0f};   ///< label base color (RGB)
    bool        bDrawLabel   = true;                    ///< Shall this aircraft's label be drawn?
    
    /// How much of the vertical offset shall be applied? (This allows phasing out the vertical offset in higher altitudes.) [0..1]
    float       vertOfsRatio = 1.0f;
    
    /// @brief By how much of the gear deflection shall the plane's altitude be reduced?
    /// @details This is to keep the wheels on the ground when gear deflection is applied.
    ///          Unfortunately, the exact factor is model-dependend. `0.5` seems an OK compromise.\n
    ///          If you know better, then overwrite.\n
    ///          If you don't want XPMP2 to correct for gear deflection, set `0.0`.
    float       gearDeflectRatio = 0.5f;
    
    /// @brief Shall this plane be clamped to ground (ie. never sink below ground)?
    /// @note This involves Y-Testing, which is a bit expensive, see [SDK](https://developer.x-plane.com/sdk/XPLMScenery).
    ///       If you know your plane is not close to the ground,
    ///       you may want to avoid clamping by setting this to `false`.
    /// @see configuration item `XPMP2_CFG_ITM_CLAMPALL`
    bool        bClampToGround = false;
    
    /// @brief Priority for display in one of the limited number of TCAS target slots
    /// @details The lower the earlier will a plane be considered for TCAS.
    ///          Increase this value if you want to make a plane less likely
    ///          to occupy one of the limited TCAS slots.
    int         aiPrio      = 1;
    
    /// @brief Current radar status
    /// @note Only the condition `mode != Standby` is of interest to XPMP2 for considering the aircraft for TCAS display
    XPMPPlaneRadar_t acRadar;
    
    /// Contrail: list of objects for contrail generation
    std::list<XPLMInstanceRef> listContrail;
    unsigned contrailNum = 0;               ///< number of contrails requested
    unsigned contrailDist_m = 5;            ///< distance between several contrails and to the aircraft's centerline, in meter
    unsigned contrailLifeTime = 25;         ///< this aircraft's contrail's life time
    
    /// @brief Wake dataRef support
    /// @see https://developer.x-plane.com/article/plugin-traffic-wake-turbulence/
    /// @details If values aren't set during aircraft creation, ie. remain at `NAN`, then defaults will be applied
    ///         based on the aircraft's wake turbulence category,
    ///         looked up from the Doc8643 list via ICAO aircraft type designator.
    struct wakeTy {
        float wingSpan_m = NAN;             ///< wing span of the aircraft creating wake turbulence
        float wingArea_m2 = NAN;            ///< wing area (total area of both wings combined) of the aircraft creating wake turbulence
        float mass_kg = NAN;                ///< actual mass of the aircraft creating the wake
        
        void clear() { *this = wakeTy(); }  ///< clear all values to defaults
        bool needsDefaults() const;         ///< any value left at `NAN`, ie. requires setting from Doc8643 WTC defaults?
        /// based on Doc8643 WTC fill with defaults
        void applyDefaults(const std::string& _wtc, bool _bOverwriteAllFields = true);
        /// Copies values only for non-NAN fields
        void fillUpFrom(const wakeTy& o);
    } wake;                                 ///< wake-support data
    
    /// Informational texts passed on via multiplayer shared dataRefs
    XPMPInfoTexts_t acInfoTexts;
    
protected:
    bool bValid                 = true;     ///< is this object valid? (Will be reset in case of exceptions)
    bool bVisible               = true;     ///< Shall this plane be drawn at the moment and be visible to TCAS/interfaces?
    bool bRender                = true;     ///< Shall the CSL model be drawn in 3D world? (if !bRender && bVivile then still visible on TCAS/interfaces, Remote Client uses this for local senders' planes to take over TCAS but not drawing)
    
    XPMP2::CSLModel*    pCSLMdl = nullptr;  ///< the CSL model in use
    int                 matchQuality = -1;  ///< quality of the match with the CSL model
    int                 acRelGrp = 0;       ///< related group, ie. line in `related.txt` in which this a/c appears, if any
    
    // this is data from about a second ago to calculate cartesian velocities
    float               prev_x = 0.0f, prev_y = 0.0f, prev_z = 0.0f;
    float               prev_ts = 0.0f;     ///< last update of `prev_x/y/z` in XP's network time
    float               gs_kn = 0.0f;       /// ground speed in [kn] based on above v_x/z
    
    /// Reverse-engineered lat/lon/alt, updated only once per second
    double lat1s = NAN, lon1s = NAN, alt1s_ft = NAN;
    
    /// Set by SetOnGrnd() with the timestamp when to reset SetTouchDown()
    float               tsResetTouchDown = NAN;
    
    /// X-Plane instance handles for all objects making up the model
    std::list<XPLMInstanceRef> listInst;
    /// Which `sim/cockpit2/tcas/targets`-index does this plane occupy? [1..63], `-1` if none
    int                 tcasTargetIdx = -1;
    
    /// Timestamp of last update of camera dist/bearing
    float               camTimLstUpd = 0.0f;
    /// Distance to camera in meters (updated internally regularly)
    float               camDist = 0.0f;
    /// Bearing from camera in degrees (updated internally regularly)
    float               camBearing = 0.0f;
    
    /// Y Probe for terrain testing, needed in ground clamping
    XPLMProbeRef        hProbe = nullptr;
    
    // Data used for drawing icons in X-Plane's map
    int                 mapIconRow = 0;     ///< map icon coordinates, row
    int                 mapIconCol = 0;     ///< map icon coordinates, column
    float               mapX = 0.0f;        ///< temporary: map coordinates (NAN = not to be drawn)
    float               mapY = 0.0f;        ///< temporary: map coordinates (NAN = not to be drawn)
    std::string         mapLabel;           ///< label for map drawing
    
    // *** Sound support ***
public:
    /// Types of sound supported directly by XPMP2
    enum SoundEventsTy {
        SND_ENG = 0,                        ///< Engine sound (continuously while engine running), bases on GetThrustRatio()
        SND_REVERSE_THRUST,                 ///< Engine sound while reverse thrust (continuously while reversers deployed), bases on GetThrustReversRatio()
        SND_TIRE,                           ///< Tires rolling on the ground (continuously while rolling on ground), bases on GetTireRotRpm()
        SND_GEAR,                           ///< Gear extending/retracting (once per event), bases on GetGearRatio()
        SND_FLAPS,                          ///< Flaps extending/retracting (once per event), bases on GetFlapRatio()
        SND_NUM_EVENTS                      ///< Number of events (always last in `enum`)
    };
    /// List of sound channel ids, also beyond sounds created for SoundEventsTy
    typedef std::list<uint64_t> ChnListTy;
    
    /// Operational values per sound channel, that is triggered by a standard sound event
    struct SndChTy {
        bool bAuto = true;                  ///< Shall this sound event be handled automatically? (Set to false in your constructor or in Aircraft::SoundSetup() if you want to control that event type yourself)
        uint64_t chnId = 0;                 ///< id of channel playing the sound currently
        float lastDRVal = NAN;              ///< last observed dataRef value to see if sound is to be triggered
        float volAdj = 1.0f;                ///< Volume adjustment, fed from Aircraft::SoundGetName()
    };

    /// @brief Minimum distance in [m] to play sound in full volume, the larger the 'louder' the aircraft
    /// @details Initialized based on engine type and numbers, overwrite in your constructor if you want to control "size" of aircraft in terms of its sound volume
    int sndMinDist = 50;
    
protected:
    /// Operational values per sound channel, that is triggered by a standard sound event
    SndChTy aSndCh[SND_NUM_EVENTS];
    /// Is sound for this aircraft currently muted?
    bool                bChnMuted = false;
    /// List of channels produced via calls to SoundPlay()
    ChnListTy           chnList;
    /// Counts how often we skipped expensive computations
    int                 skipCounter = 0;
    
private:
    bool bDestroyInst           = false;    ///< Instance to be destroyed in next flight loop callback?
public:
    
    /// @name Construction
    /// @{
    
    /// @brief Constructor creates a new aircraft object, which will be managed and displayed
    /// @exception XPMP2::XPMP2Error Mode S id invalid or duplicate, no model found during model matching
    /// @param _icaoType ICAO aircraft type designator, like 'A320', 'B738', 'C172'
    /// @param _icaoAirline ICAO airline code, like 'BAW', 'DLH', can be an empty string
    /// @param _livery Special livery designator, can be an empty string
    /// @param _modeS_id (optional) **Unique** identification of the plane [0x01..0xFFFFFF], e.g. the 24bit mode S transponder code. XPMP2 assigns an arbitrary unique number of not given
    /// @param _cslId (optional) specific unique model id to be used (package name/short id, as defined in the `OBJ8_AIRCRAFT` line)
    Aircraft (const std::string& _icaoType,
              const std::string& _icaoAirline,
              const std::string& _livery,
              XPMPPlaneID _modeS_id = 0,
              const std::string& _cslId = "");
    /// Default constructor creates an empty, invalid(!) and invisible shell; call XPMP2::Aircraft::Create() to actually create a plane
    Aircraft ();
    /// Destructor cleans up all resources acquired
    virtual ~Aircraft();
    
    /// Aircraft must not be copied as they reference non-copyable resources like XP instances
    Aircraft (const Aircraft&) = delete;
    /// Aircraft must not be copied as they reference non-copyable resources like XP instances
    Aircraft& operator=(const Aircraft&) = delete;
    
    /// @brief Creates a plane, only a valid operation if object was created using the default constructor
    /// @exception Tried on already defined object; XPMP2::XPMP2Error Mode S id invalid or duplicate, no model found during model matching
    /// @param _icaoType ICAO aircraft type designator, like 'A320', 'B738', 'C172'
    /// @param _icaoAirline ICAO airline code, like 'BAW', 'DLH', can be an empty string
    /// @param _livery Special livery designator, can be an empty string
    /// @param _modeS_id (optional) **Unique** identification of the plane [0x01..0xFFFFFF], e.g. the 24bit mode S transponder code. XPMP2 assigns an arbitrary unique number of not given
    /// @param _cslId (optional) specific unique model id to be used (package name/short id, as defined in the `OBJ8_AIRCRAFT` line)
    /// @param _pCSLModel (optional) The actual model to use (no matching or search by `_cslId` if model is given this way)
    void Create (const std::string& _icaoType,
                 const std::string& _icaoAirline,
                 const std::string& _livery,
                 XPMPPlaneID _modeS_id = 0,
                 const std::string& _cslId = "",
                 CSLModel* _pCSLModel = nullptr);
    /// @}
    /// @name Information
    /// @{

    /// return the XPMP2 plane id
    XPMPPlaneID GetModeS_ID () const { return modeS_id; }
    /// @brief Is this object "related" to the given ICAO code? (named in the same line in related.txt)
    /// @param _icaoType ICAO aircraft type designator, to which `*this` is compared
    /// @details For example, `IsRelatedTo("GLID")` returns if `*this` is a glider
    bool        IsRelatedTo (const std::string& _icaoType) const;
    /// Is this object a ground vehicle? (related to `glob.carIcaoType`)
    bool        IsGroundVehicle() const;
    /// Is this object a glider?
    bool        IsGlider() const { return IsRelatedTo("GLID"); }
    /// @brief return the current TCAS target index (into `sim/cockpit2/tcas/targets`), 1-based, `-1` if not used
    int         GetTcasTargetIdx () const { return tcasTargetIdx; }
    /// Is this plane currently also being tracked as a TCAS target, ie. will appear on TCAS?
    bool        IsCurrentlyShownAsTcasTarget () const { return tcasTargetIdx >= 1; }
    /// Is this plane currently also being tracked by X-Plane's classic AI/multiplayer?
    bool        IsCurrentlyShownAsAI () const;
    /// Is this plane to be drawn on TCAS? (It will if transponder is not switched off)
    bool        ShowAsAIPlane () const { return IsVisible() && acRadar.mode != xpmpTransponderMode_Standby; }
    /// Reset TCAS target slot index to `-1`
    void        ResetTcasTargetIdx () { tcasTargetIdx = -1; }
    
    /// @brief Return a value for dataRef .../tcas/target/flight_id
    /// @returns The first non-empty string out of: flight number, registration, departure/arrival airports
    virtual std::string GetFlightId() const;
    
    /// @}
    /// @name Model Matching
    /// @see  https://twinfan.github.io/XPMP2/Matching.html
    /// @{

    /// @brief (Potentially) changes the plane's model after doing a new match attempt
    /// @param _icaoType ICAO aircraft type designator, like 'A320', 'B738', 'C172'
    /// @param _icaoAirline ICAO airline code, like 'BAW', 'DLH', can be an empty string
    /// @param _livery Special livery designator, can be an empty string
    /// @return match quality, the lower the better
    int ChangeModel (const std::string& _icaoType,
                     const std::string& _icaoAirline,
                     const std::string& _livery);
    
    /// @brief Finds a match again, using the existing parameters, eg. after more models have been loaded
    /// @return match quality, the lower the better
    int ReMatchModel () { return ChangeModel(acIcaoType,acIcaoAirline,acLivery); }
    
    /// @brief Assigns the given model
    /// @param _cslId Search for this id (package/short)
    /// @param _pCSLModel (optional) If given use this model and don't search
    /// @return Successfuly found and assigned a model?
    bool AssignModel (const std::string& _cslId,
                      CSLModel* _pCSLModel = nullptr);
    
    /// return a pointer to the CSL model in use (Note: The CSLModel structure is not public.)
    XPMP2::CSLModel* GetModel () const { return pCSLMdl; }
    /// return the name of the CSL model in use
    const std::string& GetModelName () const;
    /// return an information structure for the CSL model associated with the aircraft
    CSLModelInfo_t GetModelInfo() const { return pCSLMdl ? CSLModelInfo_t(*pCSLMdl) : CSLModelInfo_t();  }
    /// quality of the match with the CSL model
    int         GetMatchQuality () const { return matchQuality; }

    /// @}
    /// @name Visualization
    /// @{

    /// Vertical offset, ie. the value that needs to be added to `drawInfo.y` to make the aircraft appear on the ground
    float       GetVertOfs () const;
    
    /// Is the a/c object valid?
    bool IsValid() const { return bValid; }
    /// Mark the plane invalid, e.g. after exceptions occured on the data
    virtual void SetInvalid();
    
    /// Make the plane (in)visible
    virtual void SetVisible (bool _bVisible);
    /// Is the plane visible?
    bool IsVisible () const { return bVisible && bValid; }
    
    /// Switch rendering of the CSL model on or off
    virtual void SetRender (bool _bRender);
    /// Is this plane to be rendered?
    bool IsRendered () const { return bRender && IsVisible(); }
    
    /// Are instances created for this aircraft?
    bool IsInstanciated () const { return !listInst.empty(); }
    
    /// Define if this aircraft's label is to be drawn (provided label drawing is enabled globally)
    void SetDrawLabel (bool _b) { bDrawLabel = _b; }
    /// Shall this aircraft's label be drawn?
    bool ShallDrawLabel () const { return bDrawLabel; }
    
    /// Distance to camera [m]
    float GetCameraDist () const { return camDist; }
    /// Bearing from camera [°]
    float GetCameraBearing () const { return camBearing; }

    /// @brief Called right before updating the aircraft's placement in the world
    /// @details Abstract virtual function. Override in derived classes and fill
    ///          `drawInfo`, the `v` array of dataRefs by calling the `Set`ters,
    ///          `label`, and `infoTexts` with current values.
    /// @see See [XPLMFlightLoop_f](https://developer.x-plane.com/sdk/XPLMProcessing/#XPLMFlightLoop_f)
    ///      for background on the two passed-on parameters:
    /// @param _elapsedSinceLastCall The wall time since last call
    /// @param _flCounter A monotonically increasing counter, bumped once per flight loop dispatch from the sim.
    virtual void UpdatePosition (float _elapsedSinceLastCall, int _flCounter) = 0;
    
    /// Is the aircraft on the ground?
    bool IsOnGrnd () const { return bOnGrnd; }
    /// @brief Set if the aircraft is on the ground
    /// @param on_grnd Is the aircraft on the ground?
    /// @param setTouchDownTime [opt] If not `NAN` activates SetTouchDown(), and automatically resets it after the given time in seconds
    void SetOnGrnd (bool on_grnd, float setTouchDownTime = NAN);
    
    /// @}
    /// @name Getters and Setters for the values in Aircraft::drawInfo
    /// @{

    /// @brief Converts world coordinates to local coordinates, writes to Aircraft::drawInfo
    /// @note Alternatively, the calling plugin can set local coordinates in Aircraft::drawInfo directly
    /// @param lat Latitude in degress -90..90
    /// @param lon Longitude in degrees -180..180
    /// @param alt_ft Altitude in feet above MSL
    /// @param on_grnd Is the aircraft on the ground?
    /// @param setTouchDownTime [opt] If not `NAN` activates SetTouchDown(), and automatically resets it after the given time in seconds
    void SetLocation (double lat, double lon, double alt_ft, bool on_grnd, float setTouchDownTime = NAN) {
        SetLocation(lat, lon, alt_ft);
        SetOnGrnd(on_grnd, setTouchDownTime);
    }

    /// Legacy version of above version of SetLocatiion(), here without setting ground flag
    void SetLocation (double lat, double lon, double alt_ft);
    
    /// @brief Converts aircraft's local coordinates to lat/lon values
    /// @warning This isn't exactly precice. If you need precise location keep it in your derived class yourself.
    void GetLocation (double& lat, double& lon, double& alt_ft) const;
    
    /// Sets location in local world coordinates
    void SetLocalLoc (float _x, float _y, float _z) { drawInfo.x = _x; drawInfo.y = _y; drawInfo.z = _z; }
    /// Gets all location info (including local coordinates)
    const XPLMDrawInfo_t& GetLocation () const { return drawInfo; }

    float GetPitch () const              { return drawInfo.pitch; }                     ///< pitch [degree]
    void  SetPitch (float _deg)          { drawInfo.pitch = _deg; }                     ///< pitch [degree]
    float GetHeading () const            { return drawInfo.heading; }                   ///< heading [degree]
    void  SetHeading (float _deg)        { drawInfo.heading = _deg; }                   ///< heading [degree]
    float GetRoll () const               { return drawInfo.roll; }                      ///< roll [degree]
    void  SetRoll (float _deg)           { drawInfo.roll = _deg; }                      ///< roll [degree]

    float GetGS_kn() const               { return gs_kn; }                              ///< Rough estimate of a ground speed based on `v_x/z`
    
    /// @}
    /// @name Getters and Setters for the values in the Aircraft::v array
    /// @see  https://twinfan.github.io/XPMP2/CSLdataRefs.html
    /// @{

    float GetGearRatio () const          { return v[V_CONTROLS_GEAR_RATIO]; }           ///< Gear deploy ratio
    void  SetGearRatio (float _f)        { v[V_CONTROLS_GEAR_RATIO] = _f;   }           ///< Gear deploy ratio
    float GetNoseWheelAngle () const     { return v[V_CONTROLS_NWS_RATIO]; }            ///< Nose Wheel angle in degrees
    void  SetNoseWheelAngle (float _f)   { v[V_CONTROLS_NWS_RATIO] = _f;   }            ///< Nose Wheel angle in degrees
    float GetFlapRatio () const          { return v[V_CONTROLS_FLAP_RATIO]; }           ///< Flaps deploy ratio
    void  SetFlapRatio (float _f)        { v[V_CONTROLS_FLAP_RATIO] = _f;   }           ///< Flaps deploy ratio
    float GetSpoilerRatio () const       { return v[V_CONTROLS_SPOILER_RATIO]; }        ///< Spoilers deploy ratio
    void  SetSpoilerRatio (float _f)     { v[V_CONTROLS_SPOILER_RATIO] = _f;   }        ///< Spoilers deploy ratio
    float GetSpeedbrakeRatio () const    { return v[V_CONTROLS_SPEED_BRAKE_RATIO]; }    ///< Speedbrakes deploy ratio
    void  SetSpeedbrakeRatio (float _f)  { v[V_CONTROLS_SPEED_BRAKE_RATIO] = _f;   }    ///< Speedbrakes deploy ratio
    float GetSlatRatio () const          { return v[V_CONTROLS_SLAT_RATIO]; }           ///< Slats deploy ratio
    void  SetSlatRatio (float _f)        { v[V_CONTROLS_SLAT_RATIO] = _f;   }           ///< Slats deploy ratio
    float GetWingSweepRatio () const     { return v[V_CONTROLS_WING_SWEEP_RATIO]; }     ///< Wing sweep ratio
    void  SetWingSweepRatio (float _f)   { v[V_CONTROLS_WING_SWEEP_RATIO] = _f;   }     ///< Wing sweep ratio
    float GetThrustRatio () const        { return v[V_CONTROLS_THRUST_RATIO]; }         ///< Thrust ratio
    void  SetThrustRatio (float _f)      { v[V_CONTROLS_THRUST_RATIO] = _f;   }         ///< Thrust ratio
    float GetYokePitchRatio () const     { return v[V_CONTROLS_YOKE_PITCH_RATIO]; }     ///< Yoke pitch ratio
    void  SetYokePitchRatio (float _f)   { v[V_CONTROLS_YOKE_PITCH_RATIO] = _f;   }     ///< Yoke pitch ratio
    float GetYokeHeadingRatio () const   { return v[V_CONTROLS_YOKE_HEADING_RATIO]; }   ///< Yoke heading ratio
    void  SetYokeHeadingRatio (float _f) { v[V_CONTROLS_YOKE_HEADING_RATIO] = _f;   }   ///< Yoke heading ratio
    float GetYokeRollRatio () const      { return v[V_CONTROLS_YOKE_ROLL_RATIO]; }      ///< Yoke roll ratio
    void  SetYokeRollRatio (float _f)    { v[V_CONTROLS_YOKE_ROLL_RATIO] = _f;   }      ///< Yoke roll ratio
    float GetThrustReversRatio () const  { return v[V_CONTROLS_THRUST_REVERS]; }        ///< Thrust reversers ratio
    void  SetThrustReversRatio (float _f){ v[V_CONTROLS_THRUST_REVERS] = _f;   }        ///< Thrust reversers ratio

    bool  GetLightsTaxi () const         { return v[V_CONTROLS_TAXI_LITES_ON] > 0.5f; }     ///< Taxi lights
    void  SetLightsTaxi (bool _b)        { v[V_CONTROLS_TAXI_LITES_ON] = float(_b);   }     ///< Taxi lights
    bool  GetLightsLanding () const      { return v[V_CONTROLS_LANDING_LITES_ON] > 0.5f; }  ///< Landing lights
    void  SetLightsLanding (bool _b)     { v[V_CONTROLS_LANDING_LITES_ON] = float(_b);   }  ///< Landing lights
    bool  GetLightsBeacon () const       { return v[V_CONTROLS_BEACON_LITES_ON] > 0.5f; }   ///< Beacon lights
    void  SetLightsBeacon (bool _b)      { v[V_CONTROLS_BEACON_LITES_ON] = float(_b);   }   ///< Beacon lights
    bool  GetLightsStrobe () const       { return v[V_CONTROLS_STROBE_LITES_ON] > 0.5f; }   ///< Strobe lights
    void  SetLightsStrobe (bool _b)      { v[V_CONTROLS_STROBE_LITES_ON] = float(_b);   }   ///< Strobe lights
    bool  GetLightsNav () const          { return v[V_CONTROLS_NAV_LITES_ON] > 0.5f; }      ///< Navigation lights
    void  SetLightsNav (bool _b)         { v[V_CONTROLS_NAV_LITES_ON] = float(_b);   }      ///< Navigation lights

    float GetNoseGearDeflection () const { return v[V_GEAR_NOSE_GEAR_DEFLECTION_MTR]; }     ///< Vertical nose gear deflection [meter]
    void  SetNoseGearDeflection (float _mtr) { v[V_GEAR_NOSE_GEAR_DEFLECTION_MTR] = _mtr; } ///< Vertical nose gear deflection [meter]
    float GetTireDeflection () const     { return v[V_GEAR_TIRE_VERTICAL_DEFLECTION_MTR]; } ///< Vertical (main) gear deflection [meter]
    void  SetTireDeflection (float _mtr) { v[V_GEAR_TIRE_VERTICAL_DEFLECTION_MTR] = _mtr; } ///< Vertical (main) gear deflection [meter]
    float GetTireRotAngle () const       { return v[V_GEAR_TIRE_ROTATION_ANGLE_DEG]; }      ///< Tire rotation angle [degree]
    void  SetTireRotAngle (float _deg)   { v[V_GEAR_TIRE_ROTATION_ANGLE_DEG] = _deg; }      ///< Tire rotation angle [degree]
    float GetTireRotRpm () const         { return v[V_GEAR_TIRE_ROTATION_SPEED_RPM]; }      ///< Tire rotation speed [rpm]
    void  SetTireRotRpm (float _rpm)     { v[V_GEAR_TIRE_ROTATION_SPEED_RPM] = _rpm;        ///< Tire rotation speed [rpm], also sets [rad/s]
                                           v[V_GEAR_TIRE_ROTATION_SPEED_RAD_SEC] = _rpm * RPM_to_RADs; }
    float GetTireRotRad () const         { return v[V_GEAR_TIRE_ROTATION_SPEED_RAD_SEC]; }  ///< Tire rotation speed [rad/s]
    void  SetTireRotRad (float _rad)     { v[V_GEAR_TIRE_ROTATION_SPEED_RAD_SEC] = _rad;    ///< Tire rotation speed [rad/s], also sets [rpm]
                                           v[V_GEAR_TIRE_ROTATION_SPEED_RPM] = _rad / RPM_to_RADs; }
    
    float GetEngineRotAngle () const     { return v[V_ENGINES_ENGINE_ROTATION_ANGLE_DEG]; }     ///< Engine rotation angle [degree]
    void  SetEngineRotAngle (float _deg);                                                       ///< Engine rotation angle [degree], also sets engines 1..4
    float GetEngineRotRpm () const       { return v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM]; }     ///< Engine rotation speed [rpm]
    void  SetEngineRotRpm (float _rpm);                                                         ///< Engine rotation speed [rpm], also sets [rad/s] and engines 1..4
    float GetEngineRotRad () const       { return v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC]; } ///< Engine rotation speed [rad/s]
    void  SetEngineRotRad (float _rad);                                                         ///< Engine rotation speed [rad/s], also sets [rpm] and engines 1..4
    
    float GetEngineRotAngle (size_t idx) const              ///< Engine rotation angle [degree] for engine `idx` (1..4)
    { return 1 <= idx && idx <= 4 ? v[V_ENGINES_ENGINE_ROTATION_ANGLE_DEG1+idx-1] : 0.0f; }
    void  SetEngineRotAngle (size_t idx, float _deg);       ///< Engine rotation angle [degree] for engine `idx` (1..4)
    float GetEngineRotRpm (size_t idx) const                ///< Engine rotation speed [rpm] for engine `idx` (1..4)
    { return 1 <= idx && idx <= 4 ? v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM1+idx-1] : 0.0f; }
    void  SetEngineRotRpm (size_t idx, float _rpm);         ///< Engine rotation speed [rpm] for engine `idx` (1..4), also sets [rad/s]
    float GetEngineRotRad (size_t idx) const                ///< Engine rotation speed [rad/s] for engine `idx` (1..4)
    { return 1 <= idx && idx <= 4 ? v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC+idx-1] : 0.0f; }
    void  SetEngineRotRad (size_t idx, float _rad);         ///< Engine rotation speed [rad/s] for engine `idx` (1..4), also sets [rpm]

    float GetPropRotAngle () const       { return v[V_ENGINES_PROP_ROTATION_ANGLE_DEG]; }      ///< Propellor rotation angle [degree]
    void  SetPropRotAngle (float _deg)   { v[V_ENGINES_PROP_ROTATION_ANGLE_DEG] = _deg; }      ///< Propellor rotation angle [degree]
    float GetPropRotRpm () const         { return v[V_ENGINES_PROP_ROTATION_SPEED_RPM]; }      ///< Propellor rotation speed [rpm]
    void  SetPropRotRpm (float _rpm)     { v[V_ENGINES_PROP_ROTATION_SPEED_RPM] = _rpm;        ///< Propellor rotation speed [rpm], also sets [rad/s]
                                           v[V_ENGINES_PROP_ROTATION_SPEED_RAD_SEC] = _rpm * RPM_to_RADs; }
    float GetPropRotRad () const         { return v[V_ENGINES_PROP_ROTATION_SPEED_RAD_SEC]; }  ///< Propellor rotation speed [rad/s]
    void  SetPropRotRad (float _rad)     { v[V_ENGINES_PROP_ROTATION_SPEED_RAD_SEC] = _rad;    ///< Propellor rotation speed [rad/s], also sets [rpm]
                                           v[V_ENGINES_PROP_ROTATION_SPEED_RPM] = _rad / RPM_to_RADs; }

    float GetReversDeployRatio () const  { return v[V_ENGINES_THRUST_REVERSER_DEPLOY_RATIO]; }  ///< Thrust reversers deploy ratio
    void  SetReversDeployRatio (float _f){ v[V_ENGINES_THRUST_REVERSER_DEPLOY_RATIO] = _f;   }  ///< Thrust reversers deploy ratio

    bool  GetTouchDown () const          { return v[V_MISC_TOUCH_DOWN] > 0.5f; }                ///< Moment of touch down
    void  SetTouchDown (bool _b)         { v[V_MISC_TOUCH_DOWN] = float(_b);   }                ///< Moment of touch down

    /// @}
    /// @name Wake support as per X-Plane 12
    /// @see  https://twinfan.github.io/XPMP2/Wake.html
    /// @see  https://developer.x-plane.com/2022/02/wake-turbulence/
    /// @see  https://developer.x-plane.com/article/plugin-traffic-wake-turbulence/
    /// @{
                                                                                                
    /// @brief Fill in default wake turbulence support data based on Doc8643 wake turbulence category
    /// @param _bOverwriteAllFields If `false` only overwrites `NAN` values in `wakeTy`
    void WakeApplyDefaults(bool _bOverwriteAllFields = true);

    float GetWingSpan () const          { return wake.wingSpan_m; }     ///< Wing span [m]
    void SetWingSpan (float _m)         { wake.wingSpan_m = _m; }       ///< Wing span [m]

    float GetWingArea() const           { return wake.wingArea_m2; }    ///< Wing area [m²]
    void SetWingArea (float _m2)        { wake.wingArea_m2 = _m2; }     ///< Wing area [m²]

    int GetWakeCat() const;                                             ///< Category between 0=light and 3=Super, derived from WTC

    float GetMass() const               { return wake.mass_kg; }        ///< Mass [kg]
    void SetMass(float _kg)             { wake.mass_kg = _kg; }         ///< Mass [kg]

    // Overridables:
    virtual float GetAoA() const        { return GetPitch(); }          ///< Angle of Attach, returns pitch (but you can override in your class)
    virtual float GetLift() const       { return GetMass() * G_EARTH; } ///< Lift produced. You _should_ override to blend in/out for take-off/landing, but XPMP2 has no dynamic info of your plane, not even an on-the-ground flag

    /// @}
    /// @name Contrails
    /// @note Implemented in Contrail.cpp
    /// @{

    /// @brief Trigger standard contrails as per plane type
    /// @details Bases on Doc8643 data. Contrails will be created for Jets only.
    ///          No contrails for non-jet aircraft.
    ///          Contrails will be created as per global configuration (multi? life time),
    ///          see `XPMP_CFG_ITM_CONTR_*` config items.
    ///          If you want more control use ContrailRequest().
    /// @returns Number of created contrails
    unsigned ContrailTrigger ();
    
    /// @brief Request Contrails
    /// @details They are not actually created here, that happens later in the flight loop
    /// @param num Number of contrails. Keep performance impact in mind, 1 might be sufficient.
    ///            Passing `0` will cause the contrails to be removed.
    /// @param dist_m If more than one contrail is created, this is the spacing between them and the aircraft's centerline in meters. `0` means no change: use current value of `contrailDist_m`, which in turn is initialized to `5`
    /// @param lifeTime is the time to live in seconds for each contrail puff, indirectly determining its length; `0` means no change: use current value of `contrailLifeTime`, which in turn is initialized from global configuration item `XPMP_CFG_ITM_CONTR_LIFE`
    void ContrailRequest (unsigned num, unsigned dist_m = 0, unsigned lifeTime = 0);

    /// Remove all contrail objects
    void ContrailRemove ();
    
    /// @}
    /// @name Map Support
    /// @note Implemented in Map.cpp
    /// @{

    /// Determine which map icon to use for this aircraft
    void MapFindIcon ();
    /// Prepare map coordinates
    void MapPreparePos (XPLMMapProjectionID  projection,
                        const float boundsLTRB[4]);
    /// Actually draw the map icon
    void MapDrawIcon (XPLMMapLayerID inLayer, float acSize);
    /// Actually draw the map label
    void MapDrawLabel (XPLMMapLayerID inLayer, float yOfs);
    
    /// @}
    /// @name Sound Support
    /// @note Implemented in Sound.cpp
    /// @{

    /// @brief Play a sound; a looping sound plays until explicitely stopped
    /// @param sndName One of the sounds available or registered with XPMP2, see XPMPSoundAdd() and XPMPSoundEnumerate()
    /// @param vol [opt] Volume level. 0 = silent, 1 = full. Negative level inverts the signal. Values larger than 1 amplify the signal.
    /// @returns an internal id for the sound channel, or `0` if unsuccessful
    uint64_t SoundPlay (const std::string& sndName, float vol = 1.0f);
    
    /// @brief Stop a continuously playing sound
    /// @param chnId The channel's id returned by SoundPlay()
    void SoundStop (uint64_t chnId);
    
    /// @brief Sets the sound's volume (after applying master volume and Sound File's adjustments)
    /// @param chnId The channel's id returned by SoundPlay()
    /// @param vol Volume level. 0 = silent, 1 = full. Negative level inverts the signal. Values larger than 1 amplify the signal.
    void SoundVolume (uint64_t chnId, float vol);
    
    /// @brief Mute/Unmute all sounds of the airplane temporarily
    void SoundMuteAll (bool bMute);
    /// @brief Are all sounds currently muted?
    bool SoundIsMuted() const { return bChnMuted; }
    
    /// @brief Returns the name of the sound to play per event
    /// @details This standard implementation determines the engine sound via
    ///          aircraft classification and returns constant
    ///          names for the other sound types.
    /// @note Override in derived class if you want to assign (some) sounds yourself
    /// @param sndEvent The type of sound event, like engine or gear
    /// @param[out] volAdj Volume adjustment factor, allows to make sound louder (>1.0) or quiter, defaults to 1.0
    /// @returns Name of the sound to play, empty string if no sound shall play
    virtual std::string SoundGetName (SoundEventsTy sndEvent, float& volAdj) const;

protected:

    /// @}

    /// Internal: Flight loop callback function controlling update and movement of all planes
    static float FlightLoopCB (float, float, int, void*);

    /// @name Internal Control Functions
    /// @{

    /// Internal: This puts the instance into XP's sky and makes it move
    void DoMove ();
    /// Internal: Processes once every second only stuff that doesn't require being computed every flight loop
    void DoEverySecondUpdates (float now);
    /// Internal: Create/move contrails, implemented in Contrail.cpp
    void ContrailMove ();
    /// Internal: (Re)Assess if contrails are to be created
    void ContrailAutoUpdate ();
    /// Internal: Update the plane's distance/bearing from the camera location
    void UpdateDistBearingCamera (const XPLMCameraPosition_t& posCam);
    /// Clamp to ground: Make sure the plane is not below ground, corrects Aircraft::drawInfo if needed.
    void ClampToGround ();
    /// Create the instances required to represent the plane, return if successful
    bool CreateInstances ();
    /// Destroy all instances
    void DestroyInstances ();
    
    /// @brief Put together the map label
    /// @details Called about once a second. Label depends on tcasTargetIdx
    virtual void ComputeMapLabel ();

    // The following functions are implemented in AIMultiplayer.cpp:
    /// Define the TCAS target index in use
    virtual void SetTcasTargetIdx (int _idx) { tcasTargetIdx = _idx; }

    /// @}

    // These functions perform the TCAS target / multiplayer data updates
    friend void AIMultiUpdate ();
    friend size_t AIUpdateTCASTargets ();
    friend size_t AIUpdateMultiplayerDataRefs ();
    
    /// @name Sound Support (internal)
    /// @note Implemented in Sound.cpp
    /// @{

    /// Sound-related initializations, called by Create() and ChangeModel()
    virtual void SoundSetup ();
    /// Update sound, like position and volume, called once per frame
    virtual void SoundUpdate ();
    /// Remove all sound, e.g. during destruction
    virtual void SoundRemoveAll ();
    
    /// @}
};

/// Find aircraft by its plane ID, can return nullptr
Aircraft* AcFindByID (XPMPPlaneID _id);

/// (Re)Define default wake turbulence values per WTC
bool AcSetDefaultWakeData(const std::string& _wtc, const Aircraft::wakeTy& _wake);

//
// MARK: XPMP2 Exception class
//

/// XPMP2 Exception class, e.g. thrown if there are no CSL models or duplicate modeS_ids when creating an Aircraft
class XPMP2Error : public std::logic_error {
protected:
    std::string fileName;           ///< filename of the line of code where exception occurred
    int ln;                         ///< line number of the line of code where exception occurred
    std::string funcName;           ///< function of the line of code where exception occurred
    std::string msg;                ///< additional text message
public:
    /// Constructor puts together a formatted exception text
    XPMP2Error (const char* szFile, int ln, const char* szFunc, const char* szMsg, ...);
public:
    /// returns msg.c_str()
    virtual const char* what() const noexcept;
    
public:
    // copy/move constructor/assignment as per default
    XPMP2Error (const XPMP2Error& o) = default;
    XPMP2Error (XPMP2Error&& o) = default;
    XPMP2Error& operator = (const XPMP2Error& o) = default;
    XPMP2Error& operator = (XPMP2Error&& o) = default;
};



}   // namespace XPMP2

#endif
