# XPMP2 - Only set up to compile on Linux.
#         mingw is not (yet) suitable to compile C++ 17 with full C++ library support
#         MacOS might work...just didn't get around configuring Docker to compile a framework

cmake_minimum_required(VERSION 3.0)

project(XPMP2 LANGUAGES C CXX)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules" ${CMAKE_MODULE_PATH})

# Requires X-Plane 11 SDK
# add_definitions(-DXPLM200=1 -DXPLM210=1 -DXPLM300=1 -DXPLM301=1)
set(XPMP2_DEFINES ${XPMP2_DEFINES} XPLM200=1 XPLM210=1 XPLM300=1 XPLM301=1)

# We don't need deprecation warnings for the code we declared
# deprecated ourself
add_definitions(-Wall -Wextra -pedantic -Wno-deprecated-declarations)

# Debug build?
if(CMAKE_BUILD_TYPE MATCHES "Debug")
	set(XPMP2_DEFINES ${XPMP2_DEFINES} DEBUG=1)
	set(XPMP2_DEBUG ON)
else()
	set(XPMP2_DEBUG OFF)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    if(NOT CMAKE_BUILD_TYPE MATCHES "Debug")
        # highest optimization level in Release builds
        add_compile_options(-O3 -fPIC)
	endif()
    set(XPMP2_DEFINES ${XPMP2_DEFINES} LIN=1)

elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    if(NOT CMAKE_BUILD_TYPE MATCHES "Debug")
        # Use highest optimization level in Release builds
        add_compile_options(/GL)
    endif()
	set(XPMP2_DEFINES ${XPMP2_DEFINES} IBM=1 _CRT_SECURE_NO_WARNINGS)
	set(XPMP_PLATFORM_LIBRARIES opengl32 glu32)

elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	set(XPMP2_DEFINES ${XPMP2_DEFINES} APL=1)
	find_library(OPENGL OpenGL)
	set(XPMP_PLATFORM_LIBRARIES ${OPENGL})
endif()

# Source list
add_library(XPMP2
    inc/XPCAircraft.h
    inc/XPMPAircraft.h
    inc/XPMPMultiplayer.h
    inc/XPMPPlaneRenderer.h
    src/xplanemp2.h
    src/2D.h
    src/AIMultiplayer.h
    src/Aircraft.h
    src/CSLModels.h
    src/RelatedDoc8643.h
    src/Utilities.h
    src/2D.cpp
    src/AIMultiplayer.cpp
    src/Aircraft.cpp
    src/CSLModels.cpp
    src/RelatedDoc8643.cpp
    src/Utilities.cpp
    src/XPMPMultiplayer.cpp
)

# Header include directories
target_include_directories(XPMP2
	PUBLIC
		${ADDITIONAL_INCLUDES}
		${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/SDK/CHeaders/XPLM
		${CMAKE_CURRENT_SOURCE_DIR}/inc
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(XPMP2 PRIVATE ${XPMP2_DEFINES})
set_property(TARGET XPMP2 PROPERTY CXX_STANDARD_REQUIRED 17)
set_property(TARGET XPMP2 PROPERTY CXX_STANDARD 17)
