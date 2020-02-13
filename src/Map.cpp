/// @file       Map.cpp
/// @brief      Implementation for creating the map layer
/// @details    Draws aircraft icons on X-Plane's maps in positions where our aircraft are.
///             Aircraft icons are provided in a PNG icon sheet.
/// @see        https://developer.x-plane.com/sdk/XPLMMap/
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

#define INFO_MAP_ENABLED        "Map drawing has been %s"

#define ERR_MAP_LAYER_FAILED    "Creating a map layer failed for map '%s'"
#define ERR_UNABLE_NO_ICONS     "Cannot enable map drawing due to missing map icons file"

#define DEBUG_MAP_LAYER_CREATED "Created a map layer for map '%s'"

namespace XPMP2 {

/// Dimenson of MapIcon.png
constexpr int MAP_ICON_WIDTH = 3;
/// Dimenson of MapIcon.png
constexpr int MAP_ICON_HEIGHT = 3;

/// Minimum icon size in map in UI units
constexpr int   MAP_MIN_ICON_SIZE = 40;
/// Assumed size of an aircraft in m
constexpr float MAP_AC_SIZE = 30.0f;

/// The list of known maps, which are hard-coded by X-Plane's SDK
std::array<const char*, 2> ALL_MAPS = {
    XPLM_MAP_USER_INTERFACE,
    XPLM_MAP_IOS,
};

//
// MARK: Map Drawing
//

/// Tests if x/y lies within the rect, given as a float[4] array
bool IsInRect (float x, float y, const float bounds_ltrb[4])
{
    return ((bounds_ltrb[0] <= x) && (x < bounds_ltrb[2]) &&
            (bounds_ltrb[3] <= y) && (y < bounds_ltrb[1]));
}

/// @brief Determine which map icon to use for this aircraft
/// @details    MapIcon.png has the following models:\n
///             `y \ x | 0       | 1   | 2    `\n
///             `----- | ------- | --- | ---- `\n
///             `2     | H1T     | H2T | GLID `\n
///             `1     | L1P     | L2P | L4P  `\n
///             `0     | Default | L2J | L4J  `\n
void Aircraft::MapFindIcon ()
{
    // special cases based on icao type directly
    if (acIcaoType == "GLID" ||
        acIcaoType == "PARA" ||
        acIcaoType == "ULAC")
    { mapIconRow=2; mapIconCol=2; }
    else if (acIcaoType == "GYRO" ||
             acIcaoType == "UHEL")
    { mapIconRow=2; mapIconCol=0; }
    // now determine based on type and number of engines
    else {
        const Doc8643& doc8643 = Doc8643Get(acIcaoType);
        switch (doc8643.GetClassType())
        {
                // planes with wings
            case 'A':
            case 'L':
            case 'S':
            case 'T':
                // Pick row based on Jet or no Jet
                mapIconCol = doc8643.GetClassEngType() == 'J' ? 0 : 1;
                // Pick column based on number of engines
                mapIconRow = doc8643.GetClassNumEng() >= '4' ? 2 :
                       doc8643.GetClassNumEng() >= '2' ? 1 : 0;
                break;
                
                // planes with rotos
            case 'H':
            case 'G':
                mapIconCol = 2;
                // Pick column based on number of engines
                mapIconRow = doc8643.GetClassNumEng() >= '2' ? 1 : 0;
                break;
                
                // default
            default:
                mapIconRow = mapIconCol = 0;
        }
    }
}

// Prepare map coordinates
void Aircraft::MapPreparePos (XPLMMapProjectionID  projection,
                              const float boundsLTRB[4])
{
    if (IsVisible()) {
        // Convert longitude/latitude to map coordinates
        double lat = 0.0, lon = 0.0, alt_ft = 0.0;
        GetLocation(lat, lon, alt_ft);
        XPLMMapProject(projection, lat, lon, &mapX, &mapY);

        // visible in current map? - Good!
        if (IsInRect(mapX, mapY, boundsLTRB))
            return;
    }

    // not visible (either hidden or out of visibility bounds)
    mapX = mapY = NAN;
}

// Actually draw the map icon
void Aircraft::MapDrawIcon (XPLMMapLayerID inLayer, const float acSize)
{
    // draw only if said to be visible on this map
    if (!std::isnan(mapX) && !std::isnan(mapY)) {
        XPLMDrawMapIconFromSheet(inLayer,
                                 glob.mapIconsFileName.c_str(),
                                 mapIconRow, mapIconCol,
                                 MAP_ICON_WIDTH, MAP_ICON_HEIGHT,
                                 mapX, mapY,
                                 xplm_MapOrientation_Map,
                                 drawInfo.heading,
                                 acSize);
    }
}

// Actually draw the map icon
void Aircraft::MapDrawLabel (XPLMMapLayerID inLayer, float yOfs)
{
    // draw only if said to be visible on this map
    if (!std::isnan(mapX) && !std::isnan(mapY)) {
        XPLMDrawMapLabel(inLayer,
                         label.c_str(),
                         mapX, mapY + yOfs,
                         xplm_MapOrientation_UI,
                         0.0f);
    }
}


/// Actually draw the icons into the map
/// @details This call computes the labels location on the map
///          and puts this info into the aircraft object.
///          MapLabelDrawingCB() reuses the cached location info.
void MapIconDrawingCB (XPLMMapLayerID       inLayer,
                       const float *        inMapBoundsLeftTopRightBottom,
                       float                , // zoomRatio,
                       float                mapUnitsPerUserInterfaceUnit,
                       XPLMMapStyle         , // mapStyle,
                       XPLMMapProjectionID  projection,
                       void *               )       // refcon
{
    // How many map units does one a/c have "in reality"
    const float m_per_mu = XPLMMapScaleMeter(projection,
                                             inMapBoundsLeftTopRightBottom[2] - inMapBoundsLeftTopRightBottom[0],
                                             inMapBoundsLeftTopRightBottom[1] - inMapBoundsLeftTopRightBottom[3]);
    const float acSize = std::max(MAP_AC_SIZE * m_per_mu,
                                  // But to be able to identify an icon it needs a minimum size
                                  MAP_MIN_ICON_SIZE * mapUnitsPerUserInterfaceUnit);

    // Draw icons for all (visible) aircraft
    for (const auto& p: glob.mapAc) {
        p.second->MapPreparePos(projection, inMapBoundsLeftTopRightBottom);
        p.second->MapDrawIcon(inLayer, acSize);
    }
}

/// Actually draw the labels into th emap
void MapLabelDrawingCB (XPLMMapLayerID       inLayer,
                        const float *        inMapBoundsLeftTopRightBottom,
                        float,               // zoomRatio,
                        float                mapUnitsPerUserInterfaceUnit,
                        XPLMMapStyle,        // mapStyle,
                        XPLMMapProjectionID projection,
                        void *               )      // refcon
{
    // Return at once if label drawing is off
    if (!glob.bMapLabels) return;
    
    // How many map units does one a/c have "in reality"
    const float m_per_mu = XPLMMapScaleMeter(projection,
                                             inMapBoundsLeftTopRightBottom[2] - inMapBoundsLeftTopRightBottom[0],
                                             inMapBoundsLeftTopRightBottom[1] - inMapBoundsLeftTopRightBottom[3]);
    // Based on icon size determine how much the label needs to be put below the icon
    const float yOfs = std::max(MAP_AC_SIZE * m_per_mu,
                                // But to be able to identify an icon it needs a minimum size
                                MAP_MIN_ICON_SIZE * mapUnitsPerUserInterfaceUnit) / -1.75f;

    // Draw labels for all (visible) aircraft
    for (const auto& p: glob.mapAc)
        p.second->MapDrawLabel(inLayer, yOfs);
}

//
// MARK: Create and Destroy Map Layers
//

/// Called when a map is about to be deleted
void MapDeleteCB (XPLMMapLayerID       inLayer,
                  void *               ) // inRefcon
{
    // find the entry in the map and remove it
    mapMapLayerIDTy::iterator iter =
    std::find_if(glob.mapMapLayers.begin(), glob.mapMapLayers.end(),
                 [inLayer](const mapMapLayerIDTy::value_type& p)
                 {return p.second == inLayer;});
    if (iter != glob.mapMapLayers.end())
        glob.mapMapLayers.erase(iter);
}

/// Create our map layer for the given map
void MapLayerCreate (const char* mapIdentifier)
{
    // Safety check: Do we already have a layer there?
    if (glob.mapMapLayers.count(mapIdentifier) > 0)
        return;
    
    // Create the map layer
    XPLMCreateMapLayer_t mapParams = {
        sizeof(XPLMCreateMapLayer_t),               // structSize
        mapIdentifier,                              // mapToCreateLayerIn
        xplm_MapLayer_Markings,                     // layerType
        MapDeleteCB,                                // willBeDeletedCallback
        nullptr,                                    // prepCacheCallback
        nullptr,                                    // drawCallback
        MapIconDrawingCB,                           // iconCallback
        MapLabelDrawingCB,                          // labelCallback
        1,                                          // showUiToggle
        glob.pluginName.c_str(),                    // layerName
        nullptr                                     // refcon
    };
    XPLMMapLayerID layerId = XPLMCreateMapLayer(&mapParams);
    if (layerId) {
        // save the layer id for future reference
        glob.mapMapLayers.emplace(mapIdentifier, layerId);
        LOG_MSG(logDEBUG, DEBUG_MAP_LAYER_CREATED, mapIdentifier);
    } else {
        LOG_MSG(logERR, ERR_MAP_LAYER_FAILED, mapIdentifier);
    }
        
    
}

/// Loop all existing maps and create our map layer for them
void MapCreateAll ()
{
    // Loop all possible maps and see if they exist
    for (const char* mapIdentifier: ALL_MAPS)
        if (XPLMMapExists(mapIdentifier))
            MapLayerCreate(mapIdentifier);
}

/// Remove all our map layers
void MapDestroyAll ()
{
    for (const auto& p: glob.mapMapLayers)
        XPLMDestroyMapLayer(p.second);
    glob.mapMapLayers.clear();
}

/// Callback called when a map is created. We then need to add our layer to it
void MapCreateCB (const char *  mapIdentifier,
                  void *        )       // refcon
{
    MapLayerCreate(mapIdentifier);
}


//
// MARK: Control Functions
//

// Initialize the module
void MapInit ()
{
    // Register the map create callback hook,
    // so get informed when a new map is opened
    XPLMRegisterMapCreationHook (MapCreateCB, nullptr);
    
    // Handle all already existing maps
    MapCreateAll();
}

/// Grace cleanup
void MapCleanup ()
{
    MapDestroyAll();
}


}  // namespace XPMP2

//
// MARK: General API functions outside XPMP2 namespace
//

using namespace XPMP2;

// Enable or disable the drawing of aircraft icons on X-Plane's map in a separate Layer names after the plugin
void XPMPEnableMap (bool _bEnable, bool _bLabels)
{
    // The label setting is just to be saved, no matter what
    glob.bMapLabels = _bLabels;
    
    // Short cut for no change
    if (glob.bMapEnabled == _bEnable) return;
    
    // Are we to enable but can't due to missing icon file?
    if (_bEnable && glob.mapIconsFileName.empty()) {
        LOG_MSG(logERR, ERR_UNABLE_NO_ICONS);
        return;
    }
    
    // Perform the change
    glob.bMapEnabled = _bEnable;
    if (_bEnable)
        MapCreateAll();
    else
        MapDestroyAll();
    LOG_MSG(logINFO, INFO_MAP_ENABLED,
            _bEnable ? "enabled" : "disabled");
}
