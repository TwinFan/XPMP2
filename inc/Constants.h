/// Common constants for XPMP2 library
#pragma once

namespace XPMP2 {

    // MARK: Bit masks for lighting requests
    constexpr unsigned int REQUEST_ACTUATORS_NONE       = 0;
    constexpr unsigned int REQUEST_GEAR_DOWN            = 1 <<  0;
    constexpr unsigned int REQUEST_GEAR_UP              = 1 <<  1;
    constexpr unsigned int REQUEST_LITES_TAXI_ON        = 1 <<  2;
    constexpr unsigned int REQUEST_LITES_TAXI_OFF       = 1 <<  3;
    constexpr unsigned int REQUEST_LITES_LANDING_ON     = 1 <<  4;
    constexpr unsigned int REQUEST_LITES_LANDING_OFF    = 1 <<  5;
    constexpr unsigned int REQUEST_LITES_BEACON_ON      = 1 <<  6;
    constexpr unsigned int REQUEST_LITES_BEACON_OFF     = 1 <<  7;
    constexpr unsigned int REQUEST_LITES_STROBE_ON      = 1 <<  8;
    constexpr unsigned int REQUEST_LITES_STROBE_OFF     = 1 <<  9;
    constexpr unsigned int REQUEST_LITES_NAV_ON         = 1 << 10;
    constexpr unsigned int REQUEST_LITES_NAV_OFF        = 1 << 11;

    // TODO It would be better if it would be binded to a specific model.

    // Time [seconds] for full lighting switching on or off
    constexpr float TIME_FOR_LITES_DEFAULT      = 1.0;
    // Separately for landing lights
    constexpr float TIME_FOR_LITES_LANDING      = 10.0;
    // Time for full landing gear racks motion down or up
    constexpr float TIME_FOR_GEAR_MOTION        = 10.0;

};