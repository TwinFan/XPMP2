# XPMP2 - Set up to be used in the provided docker environment to build lin and mac
#         Set up to be used in a Visual Studio environment to build win (File > Open > Folder, then VS recognized the CMAKE configuration)

cmake_minimum_required(VERSION 3.9)
project(XPMP2-Sample DESCRIPTION "XPMP2-Sample X-Plane plugin" LANGUAGES C CXX)

# By default, use optimized release configuration.
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif ()

# Enable all X-Plane SDK APIs up to the newest version.
add_definitions(-DXPLM200=1 -DXPLM210=1 -DXPLM300=1 -DXPLM301=1)

# Define platform macros.
add_definitions(-DAPL=$<BOOL:${APPLE}> -DIBM=$<BOOL:${WIN32}> -DLIN=$<AND:$<BOOL:${UNIX}>,$<NOT:$<BOOL:${APPLE}>>>)

if (UNIX OR APPLE)
    # Force-enable exception support. This is most likely redundant, although for C
    # code the default is the opposite. Since we are mixing C++ and C libraries,
    # safer to set it on?
    add_compile_options(-fexceptions -fpermissive)

    # On UNIX systems this makes symbols non-exported by default. On Windows this
    # option is simply ignored, since symbol visibility works differently there.
    add_compile_options(-fvisibility=hidden)

    # Enable stricter warnings and then disable some we are not interested in.
    add_compile_options(-Wall -Wshadow -Wfloat-equal -Wextra)
    add_compile_options(-Wno-unused)

    # Always use position-independent code and highest optimization level (FPS!).
    add_compile_options(-O3 -fPIC)
elseif(WIN32)
    # Deprecation warning: once is enough
    add_compile_options(/wo4996)
endif()

################################################################################
# Source Files
################################################################################
add_library(XPMP2-Sample MODULE
    lib/XPMP2.framework/Headers/XPCAircraft.h
    lib/XPMP2.framework/Headers/XPMPAircraft.h
    lib/XPMP2.framework/Headers/XPMPMultiplayer.h
    SDK/CHeaders/XPLM/XPLMDataAccess.h
    SDK/CHeaders/XPLM/XPLMUtilities.h
    SDK/CHeaders/XPLM/XPLMPlugin.h
    SDK/CHeaders/XPLM/XPLMGraphics.h
    SDK/CHeaders/XPLM/XPLMMenus.h
    XPMP2-Sample.cpp
)

################################################################################
# Link Libraries
################################################################################

# Specify library search locations, especially for X-Plane SDK
if (APPLE)
    list(APPEND CMAKE_FRAMEWORK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib")
    list(APPEND CMAKE_FRAMEWORK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/SDK/Libraries/Mac")
else ()
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/SDK/Libraries/Win")
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib")
endif ()

# Find the XPMP2 library
if (APPLE)
    find_library(XPMP2_LIBRARY NAMES XPMP2)
elseif (UNIX)
    find_library(XPMP2_LIBRARY NAMES libXPMP2.a)
elseif (WIN32)
    find_library(XPMP2_LIBRARY NAMES XPMP2.lib)
endif ()
target_link_libraries(XPMP2-Sample ${XPMP2_LIBRARY})

# Link OpenGL
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)  # apt install freeglut3-dev
include_directories( ${OpenGL_INCLUDE_DIRS} )
target_link_libraries( XPMP2-Sample OpenGL::GL )

# Link X-Plane plugin system libraries. They are only provided for OS X and Windows.
if (WIN32 OR APPLE)
    find_library(XPLM_LIBRARY NAMES XPLM XPLM_64.lib)
    target_link_libraries(XPMP2-Sample ${XPLM_LIBRARY})
    # target_link_libraries(XPMP2-Sample ${XPWIDGETS_LIBRARY})
endif ()


# Link library for dynamic loading of shared objects on UNIX systems.
if (UNIX)
    find_library(DL_LIBRARY dl)
    target_link_libraries(XPMP2-Sample ${DL_LIBRARY})
endif ()

# Link OS X core system libraries.
if (APPLE)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(CORE_FOUNDATION_LIBRARY CoreFoundation)
    find_library(OpenGL_LIBRARY OpenGL)

    target_link_libraries(XPMP2-Sample ${IOKIT_LIBRARY} ${CORE_FOUNDATION_LIBRARY} ${OpenGL_LIBRARY})
endif ()

if (WIN32)
    # Unlike OS X and Linux we build standard libraries statically since X-Plane does not provide them.
    # target_link_libraries(XPMP2-Sample -static-libgcc -static-libstdc++ opengl32)
    # MSYS2 for some reason always links against winpthread, so we have to work around it by statically linking the
    # entire plugin. This allows building the plugin nativaly on Windows.
    # target_link_libraries(XPMP2-Sample -static)
elseif (APPLE)
    # X-Plane supports OS X 10.10+, so this should ensure FlyWithLua can run on
    # all supported versions.
    target_compile_options(XPMP2-Sample PUBLIC -mmacosx-version-min=10.10)
    target_link_libraries(XPMP2-Sample -mmacosx-version-min=10.10)
    # Restrict set of symbols exported from the plugin. This reduces changes of
    # conflict with other plugins, in particular ones with Lua interpreter
    # embedded.
    target_link_libraries(XPMP2-Sample "-exported_symbols_list ${CMAKE_SOURCE_DIR}/XPMP2-Sample.sym_mac")
elseif (UNIX)
    # Specify additional runtime search paths for dynamically-linked libraries.
    # Restrict set of symbols exported from the plugin. This reduces chances of
    # conflict with other plugins, in particular ones with Lua interpreter
    # embedded.
    target_link_libraries(XPMP2-Sample -Wl,--version-script -Wl,${CMAKE_SOURCE_DIR}/XPMP2-Sample.sym)
endif ()

# Header include directories
target_include_directories(XPMP2-Sample
    PUBLIC
        ${ADDITIONAL_INCLUDES}
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/CHeaders/XPLM
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/XPMP2.framework/Headers
)

# We need C++ 17
set(CMAKE_CXX_STANDARD 17)
target_compile_features(XPMP2-Sample PUBLIC cxx_std_17)
set_property(TARGET XPMP2-Sample PROPERTY CXX_STANDARD_REQUIRED 17)
set_property(TARGET XPMP2-Sample PROPERTY CXX_STANDARD 17)

# Target directory and file name
if (WIN32)
   set_target_properties(XPMP2-Sample PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/win_x64")
elseif (APPLE)
   set_target_properties(XPMP2-Sample PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mac_x64")
elseif (UNIX)
   set_target_properties(XPMP2-Sample PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lin_x64")
endif ()

set_target_properties(XPMP2-Sample
    PROPERTIES
    PREFIX ""
    OUTPUT_NAME "XPMP2-Sample"
    SUFFIX ".xpl"
)
