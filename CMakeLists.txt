# XPMP2 - Set up to be used in the provided docker environment to build lin and mac
#         Set up to be used in a Visual Studio environment to build win (File > Open > Folder, then VS recognized the CMAKE configuration)

cmake_minimum_required(VERSION 3.16)

# Mac: Need to tell early on that we want a cross platform build
if(DEFINED ENV{platform})
    message ("-- Platform is $ENV{platform}")
    if($ENV{platform} STREQUAL "mac-x86")
        message ("   Building cross-platform for mac/x86_64")
        set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Archs to build")
    elseif($ENV{platform} STREQUAL "mac-arm")
        message ("   Building cross-platform for mac/arm64")
        set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Archs to build")
    endif()
else()
    # No 'platform' defined could mean running from command line, assume we build universal image in one go via XCode
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Archs to build")
endif()

project(XPMP2
        VERSION 2.60
        DESCRIPTION "Multiplayer library for X-Plane 11 and 12")

# Provide compile macros from the above project version definition
add_compile_definitions(
    XPMP2_VERSION="${PROJECT_VERSION}"
    XPMP2_VER_MAJOR=${PROJECT_VERSION_MAJOR}
    XPMP2_VER_MINOR=${PROJECT_VERSION_MINOR}
)

message ("Compiler Info:")
message ("CMAKE_CXX_COMPILER_ID         = ${CMAKE_CXX_COMPILER_ID}")
message ("CMAKE_CXX_COMPILER_VERSION    = ${CMAKE_CXX_COMPILER_VERSION}")
message ("CMAKE_CXX_COMPILER            = ${CMAKE_CXX_COMPILER}")
message ("WIN32 / MSVC / MINGW          = ${WIN32} / ${MSVC} / ${MINGW}")
message ("UNIX / APPLE                  = ${UNIX} / ${APPLE}")
if (APPLE)
    message ("OSX_SDK_PATH                  = $ENV{OSX_SDK_PATH}")
    message ("CMAKE_OSX_ARCHITECTURES       = ${CMAKE_OSX_ARCHITECTURES}")
endif()

################################################################################
# Target Systems
################################################################################

# Windows: Target Windows 7.0 and later
if (WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601)
    if (NOT DEFINED ENV{platform})
        set(ENV{platform} "win")
    endif()
elseif(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
    add_compile_options(-mmacosx-version-min=11.0)
    add_link_options(-mmacosx-version-min=11.0)
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
    add_compile_options(-Wall -Wshadow -Wfloat-equal -Wextra -Wno-deprecated-declarations)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 9.0 AND NOT APPLE)
        add_compile_options(-Wno-stringop-truncation)
    endif()

    # Force-enable exception support. This is most likely redundant, although for C
    # code the default is the opposite. Since we are mixing C++ and C libraries,
    # safer to set it on?
    add_compile_options(-fexceptions)

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

# Mingw Threads
if (MINGW)
    option(MINGW_STDTHREADS_GENERATE_STDHEADERS "" ON)
    add_subdirectory(lib/mingw-std-threads)
endif()

################################################################################
# XPMP2 Library
################################################################################

# Source list
add_library(XPMP2 STATIC
    inc/XPCAircraft.h
    inc/XPMPAircraft.h
    inc/XPMPMultiplayer.h
    inc/XPMPRemote.h
    inc/XPMPPlaneRenderer.h
    src/2D.h
    src/2D.cpp
    src/AIMultiplayer.h
    src/AIMultiplayer.cpp
    src/Aircraft.h
    src/Aircraft.cpp
    src/CSLCopy.cpp
    src/CSLModels.h
    src/CSLModels.cpp
    src/Map.h
    src/Map.cpp
    src/Network.h
    src/Network.cpp
    src/RelatedDoc8643.h
    src/RelatedDoc8643.cpp
    src/Remote.h
    src/Remote.cpp
    src/Sound.h
    src/Sound.cpp
    src/Utilities.h
    src/Utilities.cpp
    src/XPMP2.h
    src/XPMPMultiplayer.cpp
)

# Define pre-compiled header
target_precompile_headers(XPMP2 PRIVATE src/XPMP2.h)

# Header include directories
target_include_directories(XPMP2
	PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/lib/SDK/CHeaders/XPLM
		${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/lib/fmod/inc
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Include MingW threads
if (MINGW)
    target_link_libraries(XPMP2 mingw_stdthreads)
endif()

# FMOD Library
list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/lib/fmod")
if(WIN32)
    find_library(FMOD_LIBRARY NAMES fmod_vc.lib REQUIRED)
elseif(APPLE)
    find_library(FMOD_LIBRARY NAMES libfmod.dylib REQUIRED)
elseif(UNIX AND NOT APPLE)
    find_library(FMOD_LIBRARY NAMES libfmod.so.13.9 REQUIRED)
endif()
message ("FMOD_LIBRARY                  = ${FMOD_LIBRARY}")
target_link_libraries(XPMP2 ${FMOD_LIBRARY})

# make it a framework
if(APPLE)
    list(APPEND XPMP2_PUB_HEADERS
        ../inc/XPCAircraft.h
        ../inc/XPMPAircraft.h
        ../inc/XPMPMultiplayer.h
        ../inc/XPMPPlaneRenderer.h
        ../inc/XPMPRemote.h
    )

    set_target_properties(XPMP2 PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION ${PROJECT_VERSION}
        MACOSX_FRAMEWORK_IDENTIFIER com.twinfan.XPMP2
        MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        VERSION ${PROJECT_VERSION}.0
        SOVERSION ${PROJECT_VERSION}.0
        PUBLIC_HEADER "${XPMP2_PUB_HEADERS}"
    )
endif()

# Copy the resulting framework/library also into the 'lib/XPMP2' directory of the sample plugin
if(APPLE)
    add_custom_command(TARGET XPMP2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/lib/XPMP2/XPMP2.framework
        COMMAND ${CMAKE_COMMAND} -E make_directory   ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/lib/XPMP2/XPMP2.framework
        COMMAND cp -a $<TARGET_FILE_DIR:XPMP2>/../.. ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/lib/XPMP2/XPMP2.framework
    )
elseif(UNIX OR WIN32)
    add_custom_command(TARGET XPMP2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory            ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/lib/XPMP2/$ENV{platform}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:XPMP2> ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/lib/XPMP2/$ENV{platform}
    )
endif()

################################################################################
# XPMP2-Remote plugin
################################################################################

# To be able to build here we use header files from the original location.
# And we use the SDK from XPMP2-Sample so that we don't need to keep 2 copies.
add_library(XPMP2-Remote MODULE
    # Public XPMP2 header files, usually taken from the XPMP2 framework
    inc/XPCAircraft.h
    inc/XPMPAircraft.h
    inc/XPMPMultiplayer.h
    inc/XPMPRemote.h
    inc/XPMPPlaneRenderer.h
    # actual XPMP2-Remote client code
    XPMP2-Remote/Client.h
    XPMP2-Remote/Client.cpp
    XPMP2-Remote/Utilities.h
    XPMP2-Remote/Utilities.cpp
    XPMP2-Remote/XPMP2-Remote.h
    XPMP2-Remote/XPMP2-Remote.cpp
)

# Header include directories
target_include_directories(XPMP2-Remote PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/lib/SDK/CHeaders/XPLM
    ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Remote
)

# Which depends on XPMP2
add_dependencies(XPMP2-Remote XPMP2)

# Link to the just built XPMP2 library
target_link_libraries(XPMP2-Remote XPMP2 ${FMOD_LIBRARY})

# Link X-Plane plugin system libraries. They are only provided for OS X and Windows.
if (WIN32 OR APPLE)
    # Specify library search location for X-Plane SDK
    if (APPLE)
        list(APPEND CMAKE_FRAMEWORK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/lib/SDK/Libraries/Mac")
    else ()
        list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/lib/SDK/Libraries/Win")
    endif ()
    find_library(XPLM_LIBRARY XPLM REQUIRED NAMES XPLM XPLM_64.lib)
    target_link_libraries(XPMP2-Remote ${XPLM_LIBRARY})
endif ()

if (WIN32)
    # Link with winsock for network and iphlpap for GetAdaptersAddresses
    target_link_libraries(XPMP2-Remote wsock32 ws2_32 iphlpapi)
    if (MINGW)
        # Include MingW threads
        target_link_libraries(XPMP2-Remote mingw_stdthreads)
        # When cross-compiling we link the standard libraries statically
        target_link_options(XPMP2-Remote PRIVATE -static-libgcc -static-libstdc++)
    endif()
elseif (APPLE)
    # Link OS X core system libraries.
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(CORE_FOUNDATION_LIBRARY CoreFoundation REQUIRED)
    target_link_libraries(XPMP2-Remote ${IOKIT_LIBRARY} ${CORE_FOUNDATION_LIBRARY})
    # Restrict set of symbols exported from the plugin to the ones required by XPLM:
    target_link_options(XPMP2-Remote PRIVATE "-exported_symbols_list ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/XPMP2-Sample.sym_mac")
elseif (UNIX)
    # Link library for dynamic loading of shared objects on UNIX systems.
    find_library(DL_LIBRARY dl REQUIRED)
    # Threads
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)
    target_link_libraries(XPMP2-Remote ${DL_LIBRARY} Threads::Threads)
    # Specify additional runtime search paths for dynamically-linked libraries.
    # Restrict set of symbols exported from the plugin to the ones required by XPLM:
    target_link_options(XPMP2-Remote PRIVATE -Wl,--version-script -Wl,${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/XPMP2-Sample.sym)
endif ()


# Target directory and file name
if (WIN32)
   set_target_properties(XPMP2-Remote PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/win_x64")
elseif (APPLE)
   set_target_properties(XPMP2-Remote PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mac_x64")
elseif (UNIX)
   set_target_properties(XPMP2-Remote PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lin_x64")
endif ()

set_target_properties(XPMP2-Remote PROPERTIES
    PREFIX ""
    OUTPUT_NAME "XPMP2-Remote"
    SUFFIX ".xpl"
)


################################################################################
# XPMP2-Sample plugin
################################################################################

# To be able to build here we use header files from the original location.
add_library(XPMP2-Sample MODULE
    # Public XPMP2 header files, usually taken from the XPMP2 framework
    inc/XPCAircraft.h
    inc/XPMPAircraft.h
    inc/XPMPMultiplayer.h
    inc/XPMPRemote.h
    inc/XPMPPlaneRenderer.h
    # actual XPMP2-Sample code
    XPMP2-Sample/XPMP2-Sample.cpp
)

# Header include directories
target_include_directories(XPMP2-Sample PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/lib/SDK/CHeaders/XPLM
    ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Remote
)

# Which depends on XPMP2
add_dependencies(XPMP2-Sample XPMP2)

# Link to the just built XPMP2 library
target_link_libraries(XPMP2-Sample XPMP2 ${FMOD_LIBRARY})

# Link X-Plane plugin system libraries. They are only provided for OS X and Windows.
if (WIN32 OR APPLE)
    target_link_libraries(XPMP2-Sample ${XPLM_LIBRARY})
endif ()

if (WIN32)
    # Link with winsock for network and iphlpapi for GetAdaptersAddresses
    target_link_libraries(XPMP2-Sample wsock32 ws2_32 iphlpapi)
    if (MINGW)
        # Include MingW threads
        target_link_libraries(XPMP2-Sample mingw_stdthreads)
        # When cross-compiling we link the standard libraries statically
        target_link_options(XPMP2-Sample PRIVATE -static-libgcc -static-libstdc++)
    endif()
elseif (APPLE)
    target_link_libraries(XPMP2-Sample ${IOKIT_LIBRARY} ${CORE_FOUNDATION_LIBRARY})
    # Restrict set of symbols exported from the plugin to the ones required by XPLM:
    target_link_options(XPMP2-Sample PRIVATE "-exported_symbols_list ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/XPMP2-Sample.sym_mac")
elseif (UNIX)
    target_link_libraries(XPMP2-Sample ${DL_LIBRARY} Threads::Threads)
    # Restrict set of symbols exported from the plugin to the ones required by XPLM:
    target_link_options(XPMP2-Sample PRIVATE -Wl,--version-script -Wl,${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/XPMP2-Sample.sym)
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
