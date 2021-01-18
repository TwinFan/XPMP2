# XPMP2 - Set up to be used in the provided docker environment to build lin and mac
#         Set up to be used in a Visual Studio environment to build win (File > Open > Folder, then VS recognized the CMAKE configuration)

cmake_minimum_required(VERSION 3.16)

project(XPMP2-Sample
        VERSION 2.1
        DESCRIPTION "XPMP2-Sample X-Plane plugin")

################################################################################
# Target Systems
################################################################################

# Windows: Target Windows 7.0 and later
if (WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601)
elseif(APPLE)
    add_compile_options(-mmacosx-version-min=10.11)
    add_link_options(-mmacosx-version-min=10.11)
endif()

################################################################################
# C++ Standard required
################################################################################

set(CMAKE_CXX_STANDARD 17)
set_property(GLOBAL PROPERTY CXX_STANDARD_REQUIRED 17)
set_property(GLOBAL PROPERTY CXX_STANDARD 17)

################################################################################
# Compile Options
################################################################################

# Enable all X-Plane SDK APIs up to the newest version.
add_compile_definitions(XPLM200=1 XPLM210=1 XPLM300=1 XPLM301=1 XPLM303=1)

# Define platform macros.
add_compile_definitions(APL=$<BOOL:${APPLE}> IBM=$<BOOL:${WIN32}> LIN=$<AND:$<BOOL:${UNIX}>,$<NOT:$<BOOL:${APPLE}>>>)

# Enable stricter warnings and then disable some we are not interested in.
# For XPMP2 compile, we don't need to be warned about our self-defined depreciations
if (MSVC)
    add_compile_options(/wd4996 /wd4068)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wshadow -Wno-float-equal -Wextra -Wno-deprecated-declarations -Wno-unknown-pragmas)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 9.0 AND NOT APPLE)
        add_compile_options(-Wno-stringop-truncation)
    endif()

    # Force-enable exception support. This is most likely redundant, although for C
    # code the default is the opposite. Since we are mixing C++ and C libraries,
    # safer to set it on?
    add_compile_options(-fexceptions -fpermissive)

    # Makes symbols non-exported by default.
    add_compile_options(-fvisibility=hidden)
endif()


# Debug vs Release build
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    add_compile_definitions(DEBUG=1)
    if (MSVC)
        add_compile_options(/Zi)
    else()
        add_compile_options(-O0 -g)
    endif()
else()
    add_compile_definitions(NDEBUG=1)
    if(MSVC)
        # Use highest optimization level in Release builds
        add_compile_options(/GL)
    elseif(APPLE)
        add_compile_options(-O3 -fPIC)
    elseif (UNIX OR MINGW)
        # Use position-independent code and highest optimization level (FPS!).
        add_compile_options(-O3 -fPIC)
        # Strip symbols during linking
        add_link_options(-s)
    endif()
endif()

################################################################################
# Source Files
################################################################################
add_library(XPMP2-Sample MODULE
    lib/XPMP2.framework/Versions/${PROJECT_VERSION}/Headers/XPCAircraft.h
    lib/XPMP2.framework/Versions/${PROJECT_VERSION}/Headers/XPMPAircraft.h
    lib/XPMP2.framework/Versions/${PROJECT_VERSION}/Headers/XPMPMultiplayer.h
    SDK/CHeaders/XPLM/XPLMDataAccess.h
    SDK/CHeaders/XPLM/XPLMUtilities.h
    SDK/CHeaders/XPLM/XPLMPlugin.h
    SDK/CHeaders/XPLM/XPLMGraphics.h
    SDK/CHeaders/XPLM/XPLMMenus.h
    XPMP2-Sample.cpp
)

# Header include directories
target_include_directories(XPMP2-Sample PRIVATE
    ${ADDITIONAL_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR}/SDK/CHeaders/XPLM
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/XPMP2.framework/Versions/${PROJECT_VERSION}/Headers
)


################################################################################
# Link Libraries
################################################################################

# Specify library search locations, especially for X-Plane SDK
# XPMP2 path is hard-coded. Couldn't convince find_library to distinguish libXPMP2.a from XPMP2.lib in Mingw vs MSVC build.
if (APPLE)
    list(APPEND CMAKE_FRAMEWORK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib" "${CMAKE_CURRENT_SOURCE_DIR}/SDK/Libraries/Mac")
else ()
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/SDK/Libraries/Win" "${CMAKE_CURRENT_SOURCE_DIR}/lib/$ENV{platform}")
endif ()

# Link the XPMP2 library
find_library(XPMP2_LIBRARY XPMP2 REQUIRED)
message ("XPMP2_LIBRARY = ${XPMP2_LIBRARY}")
target_link_libraries(XPMP2-Sample ${XPMP2_LIBRARY})


# Link X-Plane plugin system libraries. They are only provided for OS X and Windows.
if (WIN32 OR APPLE)
    find_library(XPLM_LIBRARY XPLM REQUIRED NAMES XPLM_64.lib)
    message ("XPLM_LIBRARY  = ${XPLM_LIBRARY}")
    target_link_libraries(XPMP2-Sample ${XPLM_LIBRARY})
endif ()


if (WIN32)
    # Link with winsock for network and iphlpapi for GetAdaptersAddresses
    target_link_libraries(XPMP2-Sample wsock32 ws2_32 iphlpapi)
    if (MINGW)
        # When cross-compiling we link the standard libraries statically
        target_link_options(XPMP2-Sample PRIVATE -static-libgcc -static-libstdc++)
    endif()
elseif (APPLE)
    # Link OS X core system libraries.
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(CORE_FOUNDATION_LIBRARY CoreFoundation REQUIRED)
    target_link_libraries(XPMP2-Sample ${IOKIT_LIBRARY} ${CORE_FOUNDATION_LIBRARY})
    # Restrict set of symbols exported from the plugin to the ones required by XPLM:
    target_link_options(XPMP2-Sample PRIVATE "-exported_symbols_list ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample.sym_mac")
elseif (UNIX)
    # Threads and dynamic loading
    find_library(DL_LIBRARY dl REQUIRED)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)
    target_link_libraries(XPMP2-Sample ${DL_LIBRARY} Threads::Threads)
    # Restrict set of symbols exported from the plugin to the ones required by XPLM:
    target_link_options(XPMP2-Sample PRIVATE -Wl,--version-script -Wl,${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample.sym)
endif ()


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
