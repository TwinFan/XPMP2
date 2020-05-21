dataRefs available to CSL Models
==

XPMP2 provides the following dataRef values for each CSL model rendered.
That means, a CSL model can use it for its animations.

Actual values are taken from the `XPMP2::Aircraft::v` array. Compare the indexes
listed in the second comlumn as defined in the `enum DR_VALS`
([`XPMPAircraft.h`](html/XPMPAircraft_8h.html)).

Controls
--

dataRef                                     | index into `v`                 | Meaning
------------------------------------------- | ------------------------------ | ---------------------------------
`libxplanemp/controls/gear_ratio`           | `V_CONTROLS_GEAR_RATIO`        | Gear deployment ratio, `0..1`
`libxplanemp/controls/flap_ratio`           | `V_CONTROLS_FLAP_RATIO`        | Flaps deployment ratio, `0..1`
`libxplanemp/controls/spoiler_ratio`        | `V_CONTROLS_SPOILER_RATIO`     | Spoiler deployment ratio, `0..1`
`libxplanemp/controls/speed_brake_ratio`    | `V_CONTROLS_SPEED_BRAKE_RATIO` | Speed brakes deployment ratio, `0..1`
`libxplanemp/controls/slat_ratio`           | `V_CONTROLS_SLAT_RATIO`        | Slats deployment ratio, `0..1`
`libxplanemp/controls/wing_sweep_ratio`     | `V_CONTROLS_WING_SWEEP_RATIO`  | Wing sweep ratio, `0..1`
`libxplanemp/controls/thrust_ratio`         | `V_CONTROLS_THRUST_RATIO`      | Thrust ratio, `0..1`
`libxplanemp/controls/yoke_pitch_ratio`     | `V_CONTROLS_YOKE_PITCH_RATIO`  | Yoke pitch ratio, `0..1`
`libxplanemp/controls/yoke_heading_ratio`   | `V_CONTROLS_YOKE_HEADING_RATIO`| Yoke heading ratio, `0..1`
`libxplanemp/controls/yoke_roll_ratio`      | `V_CONTROLS_YOKE_ROLL_RATIO`   | Yoke roll ratio, `0..1`
`libxplanemp/controls/thrust_revers`        | `V_CONTROLS_THRUST_REVERS`     | Thrust reversers ratio, `0..1`
`libxplanemp/misc/touch_down`               | `V_MISC_TOUCH_DOWN`            | Moment of touch down, `0` or `1`

Lights
--

dataRef                                  | index into `v`               | Meaning
---------------------------------------- | ---------------------------- | ---------------------------------
`libxplanemp/controls/taxi_lites_on`     | `V_CONTROLS_TAXI_LITES_ON`   | Taxi lights on? `0` or `1`
`libxplanemp/controls/landing_lites_on`  | `V_CONTROLS_LANDING_LITES_ON`| Landing lights on? `0` or `1`
`libxplanemp/controls/beacon_lites_on`   | `V_CONTROLS_BEACON_LITES_ON` | Beacon lights on? `0` or `1`
`libxplanemp/controls/strobe_lites_on`   | `V_CONTROLS_STROBE_LITES_ON` | Strobe lights on? `0` or `1`
`libxplanemp/controls/nav_lites_on`      | `V_CONTROLS_NAV_LITES_ON`    | Navigation lights on? `0` or `1`

Gear
--

dataRef                                         | index into `v`                        | Meaning
----------------------------------------------- | ------------------------------------- | ---------------------------------
`libxplanemp/controls/gear_ratio`               | `V_CONTROLS_GEAR_RATIO`               | Gear deployment ratio, `0..1`
`libxplanemp/gear/tire_vertical_deflection_mtr` | `V_GEAR_TIRE_VERTICAL_DEFLECTION_MTR` | Vertical gear deflection in meters
`libxplanemp/gear/tire_rotation_angle_deg`      | `V_GEAR_TIRE_ROTATION_ANGLE_DEG`      | Tire rotation angle, `0..359` degrees
`libxplanemp/gear/tire_rotation_speed_rpm`      | `V_GEAR_TIRE_ROTATION_SPEED_RPM`      | Tire rotation speed in revolutions per minute
`libxplanemp/gear/tire_rotation_speed_rad_sec`  | `V_GEAR_TIRE_ROTATION_SPEED_RAD_SEC`  | Tire rotation speed in radians per second (`= _RPM * PI/30`)

Engines and Props
--

dataRef                                             | index into `v`                            | Meaning
--------------------------------------------------- | ----------------------------------------- | ---------------------------------
`libxplanemp/engines/engine_rotation_angle_deg`     | `V_ENGINES_ENGINE_ROTATION_ANGLE_DEG`     | Engine rotation angle, `0..359` degrees
`libxplanemp/engines/engine_rotation_speed_rpm`     | `V_ENGINES_ENGINE_ROTATION_SPEED_RPM`     | Engine rotation speed in revolutions per minute
`libxplanemp/engines/engine_rotation_speed_rad_sec` | `V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC` | Engine rotation speed in radians per second (`= _RPM * PI/30`)
`libxplanemp/engines/prop_rotation_angle_deg`       | `V_ENGINES_PROP_ROTATION_ANGLE_DEG`       | Propellor or rotor rotation angle, `0..359` degrees
`libxplanemp/engines/prop_rotation_speed_rpm`       | `V_ENGINES_PROP_ROTATION_SPEED_RPM`       | Propellor or rotor rotation speed in revolutions per minute
`libxplanemp/engines/prop_rotation_speed_rad_sec`   | `V_ENGINES_PROP_ROTATION_SPEED_RAD_SEC`   | Propellor or rotor rotation speed in radians per second (`= _RPM * PI/30`)
`libxplanemp/engines/thrust_reverser_deploy_ratio`  | `V_ENGINES_THRUST_REVERSER_DEPLOY_RATIO`  | Thrust reversers deployment ration, `0..1`
