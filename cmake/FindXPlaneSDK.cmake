# FindXPlaneSDK.cmake

# Allow the user to set the path manually
set(XPLANE_SDK_DIR "" CACHE PATH "Path to the X-Plane SDK")

# Define common search paths for SDK
if(NOT XPLANE_SDK_DIR)
    if(WIN32)
        set(_xplane_sdk_possible_paths
            "C:/XPlaneSDK"
            "C:/Program Files/XPlaneSDK"
        )
    elseif(APPLE)
        set(_xplane_sdk_possible_paths
            "/Library/Developer/XPlaneSDK"
            "$ENV{HOME}/XPlaneSDK"
        )
    elseif(UNIX)
        set(_xplane_sdk_possible_paths
            "/usr/local/XPlaneSDK"
            "/opt/XPlaneSDK"
        )
    endif()

    # Search for the SDK root directory
    find_path(XPLANE_SDK_DIR
        NAMES XPLM/XPLM.h
        PATHS ${_xplane_sdk_possible_paths}
        DOC "Directory containing the X-Plane SDK"
    )
endif()

# If not found, display an error message
if(NOT XPLANE_SDK_DIR)
    message(FATAL_ERROR "X-Plane SDK not found. Please set XPLANE_SDK_DIR.")
endif()

# Include directories for the X-Plane SDK
set(XPLANE_SDK_INCLUDE_DIR ${XPLANE_SDK_DIR}/CHeaders)

# Define common search paths for libraries
set(_xplane_lib_xplm_name "XPLM")
set(_xplane_lib_xpwidgets_name "XPWidgets")
if(WIN32)
    set(_xplane_lib_possible_paths
        "${XPLANE_SDK_DIR}/Libraries/Win"
    )
	set(_xplane_lib_xplm_name "XPLM_64")
	set(_xplane_lib_xpwidgets_name "XPWidgets_64")
elseif(APPLE)
    set(_xplane_lib_possible_paths
        "${XPLANE_SDK_DIR}/Libraries/Mac"
    )
elseif(UNIX)
    set(_xplane_lib_possible_paths
        "${XPLANE_SDK_DIR}/Libraries/Linux"
    )
endif()

# Search for the XPLM and XPWidgets libraries
find_library(XPLM_LIBRARY NAMES ${_xplane_lib_xplm_name} PATHS ${_xplane_lib_possible_paths})
find_library(XPWIDGETS_LIBRARY NAMES ${_xplane_lib_xpwidgets_name} PATHS ${_xplane_lib_possible_paths})

# If the libraries are not found, display an error
if(NOT XPLM_LIBRARY)
    message(FATAL_ERROR "Failed to find XPLM library.")
endif()

if(NOT XPWIDGETS_LIBRARY)
    message(FATAL_ERROR "Failed to find XPWidgets library.")
endif()

# Provide the results to the parent scope
set(XPLANE_SDK_INCLUDE_DIR ${XPLANE_SDK_INCLUDE_DIR})
set(XPLANE_SDK_LIBRARIES ${XPLM_LIBRARY} ${XPWIDGETS_LIBRARY})

# Optionally, create an imported target for modern CMake usage
if(NOT TARGET XPlaneSDK::XPLM)
    add_library(XPlaneSDK::XPLM UNKNOWN IMPORTED)
    set_target_properties(XPlaneSDK::XPLM PROPERTIES
        IMPORTED_LOCATION ${XPLM_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${XPLANE_SDK_INCLUDE_DIR}
    )
endif()

if(NOT TARGET XPlaneSDK::XPWidgets)
    add_library(XPlaneSDK::XPWidgets UNKNOWN IMPORTED)
    set_target_properties(XPlaneSDK::XPWidgets PROPERTIES
        IMPORTED_LOCATION ${XPWIDGETS_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${XPLANE_SDK_INCLUDE_DIR}
    )
endif()