# XPMP2 - Set up to be used in the provided docker environment to build lin and mac
#         Set up to be used in a Visual Studio environment to build win (File > Open > Folder, then VS recognized the CMAKE configuration)

cmake_minimum_required(VERSION 3.16)

project(XPMP2 LANGUAGES C CXX)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules" ${CMAKE_MODULE_PATH})

# Enable all X-Plane SDK APIs up to the newest version.
add_definitions(-DXPLM200=1 -DXPLM210=1 -DXPLM300=1 -DXPLM301=1)

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

# Source list
add_library(XPMP2 STATIC
    inc/XPCAircraft.h
    inc/XPMPAircraft.h
    inc/XPMPMultiplayer.h
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
    src/RelatedDoc8643.h
    src/RelatedDoc8643.cpp
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
