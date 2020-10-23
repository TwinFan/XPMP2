# XPMP2 - Set up to be used in the provided docker environment to build lin and mac
#         Set up to be used in a Visual Studio environment to build win (File > Open > Folder, then VS recognized the CMAKE configuration)

cmake_minimum_required(VERSION 3.16)

project(XPMP2 LANGUAGES C CXX)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules" ${CMAKE_MODULE_PATH})

# Enable all X-Plane SDK APIs up to the newest version.
add_definitions(-DXPLM200=1 -DXPLM210=1 -DXPLM300=1 -DXPLM301=1 -DXPLM303=1)

# Define platform macros.
add_definitions(-DAPL=$<BOOL:${APPLE}> -DIBM=$<BOOL:${WIN32}> -DLIN=$<AND:$<BOOL:${UNIX}>,$<NOT:$<BOOL:${APPLE}>>>)

# Enable stricter warnings and then disable some we are not interested in.
# For XPMP2 compile, we don't need to be warned about our self-defined depreciations
if (WIN32)
    add_compile_options(/wd4996 /wd4068)
else()
    add_compile_options(-Wall -Wshadow -Wfloat-equal -Wextra -Wno-deprecated-declarations -Wno-unknown-pragmas)

    # Force-enable exception support. This is most likely redundant, although for C
    # code the default is the opposite. Since we are mixing C++ and C libraries,
    # safer to set it on?
    add_compile_options(-fexceptions -fpermissive)

    # On UNIX systems this makes symbols non-exported by default. On Windows this
    # option is simply ignored, since symbol visibility works differently there.
    add_compile_options(-fvisibility=hidden)
endif()


# Debug build?
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    add_compile_options(-DDEBUG=1)
	set(XPMP2_DEBUG ON)
else()
	set(XPMP2_DEBUG OFF)

    if (UNIX)
        # Use position-independent code and highest optimization level (FPS!).
        add_compile_options(-O3 -fPIC)
    endif()
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
    src/Utilities.h
    src/Utilities.cpp
    src/XPMP2.h
    src/XPMPMultiplayer.cpp
)

# Define pre-compiled header
target_precompile_headers(XPMP2 PUBLIC src/XPMP2.h)

# Header include directories
target_include_directories(XPMP2
	PUBLIC
		${ADDITIONAL_INCLUDES}
		${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/SDK/CHeaders/XPLM
		${CMAKE_CURRENT_SOURCE_DIR}/inc
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(APPLE)
    target_compile_options(XPMP2 PUBLIC -mmacosx-version-min=10.10)
    target_link_libraries(XPMP2 -mmacosx-version-min=10.10)

    # make it a framework
    list(APPEND XPMP2_PUB_HEADERS
        ../inc/XPCAircraft.h
        ../inc/XPMPAircraft.h
        ../inc/XPMPMultiplayer.h
        ../inc/XPMPPlaneRenderer.h
    )

    set_target_properties(XPMP2 PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION 1.0
      MACOSX_FRAMEWORK_IDENTIFIER com.twinfan.XPMP2
      MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
      VERSION 0.1.0
      SOVERSION 0.1.0
      PUBLIC_HEADER "${XPMP2_PUB_HEADERS}"
    )

elseif(WIN32)
    if(NOT CMAKE_BUILD_TYPE MATCHES "Debug")
        # Use highest optimization level in Release builds
        add_compile_options(/GL)
    endif()
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

endif()

set_property(TARGET XPMP2 PROPERTY CXX_STANDARD_REQUIRED 17)
set_property(TARGET XPMP2 PROPERTY CXX_STANDARD 17)

# Copy the resulting framework/library also into the 'lib' directory of the sample plugin
if(APPLE)
    add_custom_command(TARGET XPMP2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/lib/XPMP2.framework
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/lib/XPMP2.framework
        # While the following does copy the framework it does not retain the symbolic links, but produces copies. Still...it works for the purpose
        COMMAND ${CMAKE_COMMAND} -E copy_directory $<TARGET_FILE_DIR:XPMP2>/../.. ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/lib/XPMP2.framework
    )
elseif(UNIX OR WIN32)
    add_custom_command(TARGET XPMP2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:XPMP2> ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/lib
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
    # X-Plane SDK
    XPMP2-Sample/SDK/CHeaders/XPLM/XPLMDataAccess.h
    XPMP2-Sample/SDK/CHeaders/XPLM/XPLMUtilities.h
    XPMP2-Sample/SDK/CHeaders/XPLM/XPLMPlugin.h
    XPMP2-Sample/SDK/CHeaders/XPLM/XPLMGraphics.h
    XPMP2-Sample/SDK/CHeaders/XPLM/XPLMMenus.h
    # actual XPMP2-Remote client code
    XPMP2-Remote/Client.h
    XPMP2-Remote/Client.cpp
    XPMP2-Remote/Utilities.h
    XPMP2-Remote/Utilities.cpp
    XPMP2-Remote/XPMP2-Remote.h
    XPMP2-Remote/XPMP2-Remote.cpp
)

# Header include directories
target_include_directories(XPMP2-Remote
    PUBLIC
        ${ADDITIONAL_INCLUDES}
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Remote
        ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/SDK/CHeaders/XPLM
)

# Which depends on XPMP2
add_dependencies(XPMP2-Remote XPMP2)

# Link to the just built XPMP2 library
target_link_libraries(XPMP2-Remote $<TARGET_FILE:XPMP2>)

# Link X-Plane plugin system libraries. They are only provided for OS X and Windows.
if (WIN32 OR APPLE)
    # Specify library search location for X-Plane SDK
    if (APPLE)
        list(APPEND CMAKE_FRAMEWORK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/SDK/Libraries/Mac")
    else ()
        list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/SDK/Libraries/Win")
    endif ()
    find_library(XPLM_LIBRARY NAMES XPLM XPLM_64.lib)
    target_link_libraries(XPMP2-Remote ${XPLM_LIBRARY})
endif ()

if (WIN32)
    # Link with winsock for network
    target_link_libraries(XPMP2-Remote wsock32 ws2_32)
elseif (APPLE)
    # Link OS X core system libraries.
    find_library(IOKIT_LIBRARY IOKit)
    find_library(CORE_FOUNDATION_LIBRARY CoreFoundation)
    target_link_libraries(XPMP2-Remote ${IOKIT_LIBRARY} ${CORE_FOUNDATION_LIBRARY})
    # X-Plane supports OS X 10.10+
    target_compile_options(XPMP2-Remote PUBLIC -mmacosx-version-min=10.11)
    target_link_libraries(XPMP2-Remote -mmacosx-version-min=10.11)

    # Restrict set of symbols exported from the plugin. This reduces changes of
    # conflict with other plugins, in particular ones with Lua interpreter
    # embedded.
    target_link_libraries(XPMP2-Remote "-exported_symbols_list ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/XPMP2-Sample.sym_mac")
elseif (UNIX)
    # Link library for dynamic loading of shared objects on UNIX systems.
    find_library(DL_LIBRARY dl)
    target_link_libraries(XPMP2-Remote ${DL_LIBRARY})
    # Threads
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)

    # Specify additional runtime search paths for dynamically-linked libraries.
    # Restrict set of symbols exported from the plugin. This reduces chances of
    # conflict with other plugins, in particular ones with Lua interpreter
    # embedded.
    target_link_libraries(XPMP2-Remote Threads::Threads -Wl,--version-script -Wl,${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/XPMP2-Sample.sym)
endif ()

# We need C++ 17
set(CMAKE_CXX_STANDARD 17)
target_compile_features(XPMP2-Remote PUBLIC cxx_std_17)
set_property(TARGET XPMP2-Remote PROPERTY CXX_STANDARD_REQUIRED 17)
set_property(TARGET XPMP2-Remote PROPERTY CXX_STANDARD 17)

# Target directory and file name
if (WIN32)
   set_target_properties(XPMP2-Remote PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/win_x64")
elseif (APPLE)
   set_target_properties(XPMP2-Remote PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mac_x64")
elseif (UNIX)
   set_target_properties(XPMP2-Remote PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lin_x64")
endif ()

set_target_properties(XPMP2-Remote
    PROPERTIES
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
    # X-Plane SDK
    XPMP2-Sample/SDK/CHeaders/XPLM/XPLMDataAccess.h
    XPMP2-Sample/SDK/CHeaders/XPLM/XPLMUtilities.h
    XPMP2-Sample/SDK/CHeaders/XPLM/XPLMPlugin.h
    XPMP2-Sample/SDK/CHeaders/XPLM/XPLMGraphics.h
    XPMP2-Sample/SDK/CHeaders/XPLM/XPLMMenus.h
    # actual XPMP2-Sample code
    XPMP2-Sample/XPMP2-Sample.cpp
)

# Header include directories
target_include_directories(XPMP2-Sample
    PUBLIC
        ${ADDITIONAL_INCLUDES}
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Remote
        ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/SDK/CHeaders/XPLM
)

# Which depends on XPMP2
add_dependencies(XPMP2-Sample XPMP2)

# Link to the just built XPMP2 library
target_link_libraries(XPMP2-Sample $<TARGET_FILE:XPMP2>)

# Link X-Plane plugin system libraries. They are only provided for OS X and Windows.
if (WIN32 OR APPLE)
    target_link_libraries(XPMP2-Sample ${XPLM_LIBRARY})
endif ()

if (WIN32)
    # Link with winsock for network
    target_link_libraries(XPMP2-Sample wsock32 ws2_32)
elseif (APPLE)
    target_link_libraries(XPMP2-Sample ${IOKIT_LIBRARY} ${CORE_FOUNDATION_LIBRARY})
    # X-Plane supports OS X 10.10+
    target_compile_options(XPMP2-Sample PUBLIC -mmacosx-version-min=10.11)
    target_link_libraries(XPMP2-Sample -mmacosx-version-min=10.11)

    # Restrict set of symbols exported from the plugin. This reduces changes of
    # conflict with other plugins, in particular ones with Lua interpreter
    # embedded.
    target_link_libraries(XPMP2-Sample "-exported_symbols_list ${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/XPMP2-Sample.sym_mac")
elseif (UNIX)
    target_link_libraries(XPMP2-Sample ${DL_LIBRARY})
    # Specify additional runtime search paths for dynamically-linked libraries.
    # Restrict set of symbols exported from the plugin. This reduces chances of
    # conflict with other plugins, in particular ones with Lua interpreter
    # embedded.
    target_link_libraries(XPMP2-Sample Threads::Threads -Wl,--version-script -Wl,${CMAKE_CURRENT_SOURCE_DIR}/XPMP2-Sample/XPMP2-Sample.sym)
endif ()

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
