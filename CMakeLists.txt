cmake_minimum_required(VERSION 3.9)
project(QMPlay2)

if(WIN32)
    set(CUSTOM_INCLUDE_DIR "" CACHE STRING "Custom include directory")
    set(CUSTOM_LINK_DIR "" CACHE STRING "Custom link directory")

    if(CUSTOM_INCLUDE_DIR)
        set(CMAKE_INCLUDE_PATH ${CUSTOM_INCLUDE_DIR})
        include_directories(${CUSTOM_INCLUDE_DIR})
    endif()
    if(CUSTOM_LINK_DIR)
        set(CMAKE_LIBRARY_PATH ${CUSTOM_LINK_DIR})
        link_directories(${CUSTOM_LINK_DIR})
    endif()
endif()

# Set default build type as Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(ANDROID)
    if(ANDROID_ABI STREQUAL "armeabi-v7a")
        set(ANDROID_EXTRA_LIBS_DIR "${CMAKE_SYSROOT}/usr/lib/arm-linux-androideabi")
    endif()
    set(ANDROID_EXTRA_LIBS
        "${ANDROID_EXTRA_LIBS_DIR}/libavcodec.so,${ANDROID_EXTRA_LIBS_DIR}/libavformat.so,${ANDROID_EXTRA_LIBS_DIR}/libavutil.so,${ANDROID_EXTRA_LIBS_DIR}/libswresample.so,${ANDROID_EXTRA_LIBS_DIR}/libswscale.so,${ANDROID_EXTRA_LIBS_DIR}/libssl_1_1.so,${ANDROID_EXTRA_LIBS_DIR}/libcrypto_1_1.so"
    )
    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/gui/Android/manifest")
endif()

# Silence warning about unused variable in QtCreator
set(QT_QMAKE_EXECUTABLE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/src/cmake/Modules")

include(FeatureSummary)

if(NOT CMAKE_HOST_WIN32 AND NOT ANDROID)
    find_package(PkgConfig REQUIRED)
else()
    find_package(DummyPkgConfig REQUIRED)
endif()

find_package(Qt5Widgets 5.10 REQUIRED)

pkg_check_modules(LIBAVFORMAT REQUIRED libavformat>=58.12.100)
pkg_check_modules(LIBAVCODEC REQUIRED libavcodec>=58.18.100)
pkg_check_modules(LIBSWSCALE REQUIRED libswscale>=5.1.100)
pkg_check_modules(LIBAVUTIL REQUIRED libavutil>=56.14.100)
pkg_check_modules(LIBSWRESAMPLE REQUIRED libswresample>=3.1.100)

# Add FFmpeg link directories here, because it must be global.
# "target_link_directories" is supported since CMake 3.13, but QMPlay2 supports CMake 3.9.
link_directories(
    ${LIBAVFORMAT_LIBRARY_DIRS}
    ${LIBAVCODEC_LIBRARY_DIRS}
    ${LIBSWSCALE_LIBRARY_DIRS}
    ${LIBAVUTIL_LIBRARY_DIRS}
    ${LIBSWRESAMPLE_LIBRARY_DIRS}
)

set(CMAKE_LINK_DEPENDS_NO_SHARED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
    option(USE_PCH "Use precompiled headers" OFF)
    add_feature_info(PCH USE_PCH "Use precompiled headers")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(DEFAULT_ALSA ON)
    set(DEFAULT_PORTAUDIO OFF)
elseif(ANDROID)
    set(DEFAULT_ALSA OFF)
    set(DEFAULT_PORTAUDIO OFF)
else()
    set(DEFAULT_ALSA OFF)
    set(DEFAULT_PORTAUDIO ON)
endif()

if(APPLE OR ANDROID)
    set(DEFAULT_CUVID OFF)
else()
    set(DEFAULT_CUVID ON)
endif()

add_definitions(-D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DQT_USE_FAST_OPERATOR_PLUS)

option(USE_UPDATES "Build with software updates" ON)
add_feature_info(Updates USE_UPDATES "Build with software updates")

if(WIN32)
    option(USE_CMD "Show CMD when running QMPlay2" OFF)
    mark_as_advanced(USE_CMD)
    add_feature_info(CMD USE_CMD "Show CMD when running QMPlay2")
endif()

option(USE_OPENGL "Build with OpenGL support" ON)
add_feature_info(OpenGL USE_OPENGL "Build with OpenGL support")
if(USE_OPENGL)
    add_definitions(-DUSE_OPENGL)
endif()

if(NOT APPLE)
    get_target_property(Qt5GuiFeatures Qt5::Gui INTERFACE_QT_ENABLED_FEATURES)
    list(FIND Qt5GuiFeatures "vulkan" Qt5Vulkan)
    if(${Qt5Vulkan} GREATER -1)
        set(VULKAN_AVAILABLE ON)
    endif()
endif()
option(USE_VULKAN "Build with Vulkan support" ${VULKAN_AVAILABLE})
add_feature_info(Vulkan USE_VULKAN "Build with Vulkan support")
if(USE_VULKAN)
    add_definitions(-DUSE_VULKAN)
endif()

option(USE_GLSLC "Compile Vulkan shaders" OFF)
add_feature_info(GLSLC USE_GLSLC "Compile Vulkan shaders")

if(NOT WIN32 AND NOT APPLE AND NOT ANDROID)
    option(USE_FREEDESKTOP_NOTIFICATIONS "Use Freedesktop notifications" ON)
    add_feature_info("Freedesktop notifications" USE_FREEDESKTOP_NOTIFICATIONS "Use Freedesktop notifications")

    option(USE_DBUS_SUSPEND "Use D-Bus suspend" ON)
    add_feature_info("D-Bus suspend" USE_DBUS_SUSPEND "Use D-Bus suspend")
endif()

option(USE_LIBASS "Build with libass support" ON)
add_feature_info(libass USE_LIBASS "Build with libass support")

option(USE_INPUTS "Build with Inputs module" ON)
add_feature_info(Inputs USE_INPUTS "Build with Inputs module")

option(USE_MODPLUG "Build with Modplug module" ON)
add_feature_info(Modplug USE_MODPLUG "Build with Modplug module")

option(USE_EXTENSIONS "Build with Extensions module" ON)
add_feature_info(Extensions USE_EXTENSIONS "Build with Extensions module")

if(USE_EXTENSIONS)
    option(USE_MEDIABROWSER "Build with MediaBrowser support" ON)
    add_feature_info(MediaBrowser USE_MEDIABROWSER "Build with MediaBrowser support")

    option(USE_LASTFM "Build with LastFM support" ON)
    add_feature_info(LastFM USE_LASTFM "Build with LastFM support")

    option(USE_LYRICS "Build with lyrics support" ON)
    add_feature_info(Lyrics USE_LYRICS "Build with lyrics support")

    option(USE_RADIO "Build with Radio Browser support" ON)
    add_feature_info(Radio USE_RADIO "Build with Radio Browser support")

    option(USE_YOUTUBE "Build with YouTube support" ON)
    add_feature_info(YouTube USE_YOUTUBE "Build with YouTube support")
endif()

if(USE_YOUTUBE OR USE_MEDIABROWSER)
    set(USE_YOUTUBEDL ON)
    add_definitions(-DUSE_YOUTUBEDL)
else()
    set(USE_YOUTUBEDL OFF)
endif()

if(USE_EXTENSIONS AND NOT WIN32 AND NOT APPLE AND NOT ANDROID)
    option(USE_MPRIS2 "Build Extensions with MPRIS2 support" ON)
    add_feature_info(MPRIS2 USE_MPRIS2 "Build Extensions with MPRIS2 support")
endif()

option(USE_VISUALIZATIONS "Build with Visualizations module" ON)
add_feature_info(Visualizations USE_VISUALIZATIONS "Build with Visualizations module")

option(USE_AUDIOFILTERS "Build with AudioFilters module" ON)
add_feature_info(AudioFilters USE_AUDIOFILTERS "Build with AudioFilters module")

option(USE_VIDEOFILTERS "Build with VideoFilters module" ON)
add_feature_info(VideoFilters USE_VIDEOFILTERS "Build with VideoFilters module")

option(USE_PORTAUDIO "Build with PortAudio module" ${DEFAULT_PORTAUDIO})
add_feature_info(PortAudio USE_PORTAUDIO "Build with PortAudio module")

if(NOT DEFINED USE_TAGLIB)
    find_package(TagLib)
endif()
option(USE_TAGLIB "Build with tags editor" ${TAGLIB_FOUND})
add_feature_info(TagLib USE_TAGLIB "Build with tags editor")

if(NOT DEFINED USE_FFMPEG_VAAPI)
    pkg_check_modules(LIBS_VAAPI libva>=1.1.0 libva-drm)
endif()
option(USE_FFMPEG_VAAPI "Build VAAPI acceleration into FFmpeg" ${LIBS_VAAPI_FOUND})
add_feature_info(VAAPI USE_FFMPEG_VAAPI "Build VAAPI acceleration into FFmpeg")

if(NOT DEFINED USE_FFMPEG_VDPAU)
    pkg_check_modules(LIBS_VDPAU vdpau x11)
endif()
option(USE_FFMPEG_VDPAU "Build VDPAU acceleration into FFmpeg" ${LIBS_VDPAU_FOUND})
add_feature_info(VDPAU USE_FFMPEG_VDPAU "Build VDPAU acceleration into FFmpeg")

if(WIN32)
    option(USE_FFMPEG_DXVA2 "Build DXVA2 acceleration into FFmpeg" ON)
    add_feature_info(DXVA2 USE_FFMPEG_DXVA2 "Build DXVA2 acceleration into FFmpeg")

    if(USE_VULKAN)
        option(USE_FFMPEG_D3D11VA "Build D3D11VA acceleration into FFmpeg" ON)
        add_feature_info(DXVA2 USE_FFMPEG_D3D11VA "Build D3D11VA acceleration into FFmpeg")
    endif()
endif()

if(APPLE)
    option(USE_FFMPEG_VTB "Build VideoToolBox acceleration into FFmpeg" ON)
    add_feature_info(VTB USE_FFMPEG_VTB "Build VideoToolBox acceleration into FFmpeg")
endif()

if(NOT DEFINED USE_FFMPEG_AVDEVICE)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        pkg_check_modules(LIBAVDEVICE libavdevice)
        set(DEFAULT_AVDEVICE ${LIBAVDEVICE_FOUND})
    else()
        set(DEFAULT_AVDEVICE OFF)
    endif()
endif()
option(USE_FFMPEG_AVDEVICE "Build FFmpeg with libavdevice suport" ${DEFAULT_AVDEVICE})
add_feature_info(libavdevice USE_FFMPEG_AVDEVICE "Build FFmpeg with libavdevice suport")

if(NOT DEFINED USE_AUDIOCD)
    pkg_check_modules(LIBCD libcdio libcddb)
endif()
option(USE_AUDIOCD "Build with AudioCD module" ${LIBCD_FOUND})
add_feature_info(AudioCD USE_AUDIOCD "Build with AudioCD module")

option(USE_ALSA "Build with ALSA module" ${DEFAULT_ALSA})
add_feature_info(ALSA USE_ALSA "Build with ALSA module")

if(NOT DEFINED USE_CHIPTUNE_GME)
    find_package(GME)
endif()
option(USE_CHIPTUNE_GME "Build Chiptune with GME support" ${LIBGME_FOUND})
add_feature_info("Chiptune GME" USE_CHIPTUNE_GME "Build Chiptune with GME support")

if(NOT DEFINED USE_CHIPTUNE_SID)
    pkg_check_modules(LIBSIDPLAYFP libsidplayfp)
endif()
option(USE_CHIPTUNE_SID "Build Chiptune with SIDPLAY support" ${LIBSIDPLAYFP_FOUND})
add_feature_info("Chiptune SIDPLAY" USE_CHIPTUNE_SID "Build Chiptune with SIDPLAY support")

if(NOT DEFINED USE_PULSEAUDIO)
    pkg_check_modules(LIBPULSE libpulse-simple)
endif()
option(USE_PULSEAUDIO "Build with PulseAudio module" ${LIBPULSE_FOUND})
add_feature_info(PulseAudio USE_PULSEAUDIO "Build with PulseAudio module")

if(NOT DEFINED USE_PIPEWIRE)
    pkg_check_modules(LIBPIPEWIRE libpipewire-0.3)
endif()
option(USE_PIPEWIRE "Build with PipeWire module" ${LIBPIPEWIRE_FOUND})
add_feature_info(PipeWire USE_PIPEWIRE "Build with PipeWire module")

if(NOT DEFINED USE_XVIDEO)
    pkg_check_modules(LIB_X11_XV x11 xv)
endif()
option(USE_XVIDEO "Build with XVideo module" ${LIB_X11_XV_FOUND})
add_feature_info(XVideo USE_XVIDEO "Build with XVideo module")

option(USE_CUVID "Build with CUVID module" ${DEFAULT_CUVID})
add_feature_info(CUVID USE_CUVID "Build with CUVID module")

option(USE_NOTIFY "Build additional notifications module" ON)
add_feature_info(Notifications USE_NOTIFY "Build additional notifications module")

set(LANGUAGES "All" CACHE STRING "A space-seperated list of translations to compile into QMPlay2 or \"All\"")

if(USE_FFMPEG_VAAPI OR USE_FFMPEG_VDPAU)
    option(FIND_HWACCEL_DRIVERS_PATH "Find drivers path for hwaccel, useful for universal package" OFF)
    add_feature_info(FindHwaccelDrivers FIND_HWACCEL_DRIVERS_PATH "Find drivers path for hwaccel")
endif()

if(NOT WIN32)
    # get QMPlay2 version by running the version script
    execute_process(
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/version
        OUTPUT_VARIABLE QMPLAY2_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

option(USE_GIT_VERSION "Append Git HEAD to QMPlay2 version (if exists)" ON)
if(USE_GIT_VERSION)
    # get QMPlay2 Git HEAD by running git command
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE QMPLAY2_GIT_HEAD
        OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
add_feature_info("Git version" QMPLAY2_GIT_HEAD "Append Git HEAD to QMPlay2 version")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang") # GCC or Clang
    option(USE_LINK_TIME_OPTIMIZATION "Enable link time optimization for release builds" OFF)
    add_feature_info("Link Time Optimization" USE_LINK_TIME_OPTIMIZATION "Enable link time optimization for release builds")

    option(USE_ASAN "Use Address Sanitizer" OFF)
    add_feature_info("Address Sanitizer" USE_ASAN "Use Address Sanitizer")

    option(USE_UBSAN "Use Undefined Behavior Sanitizer" OFF)
    add_feature_info("Undefined Behavior Sanitizer" USE_UBSAN "Use Undefined Behavior Sanitizer")

    if(USE_ASAN)
        set(CMAKE_CXX_FLAGS "-fsanitize=address -fno-omit-frame-pointer ${CMAKE_CXX_FLAGS}")
        set(RELEASE_DEBUG_FLAG "-g")
    else()
        set(RELEASE_DEBUG_FLAG "-g0")
    endif()

    if(USE_UBSAN)
        set(CMAKE_CXX_FLAGS "-fsanitize=undefined ${CMAKE_CXX_FLAGS}")
    endif()

    # Hide symbols, and also make sure that debugging symbols are disabled
    set(CMAKE_CXX_FLAGS_RELEASE    "${RELEASE_DEBUG_FLAG} -fvisibility=hidden -fvisibility-inlines-hidden ${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${RELEASE_DEBUG_FLAG} -fvisibility=hidden -fvisibility-inlines-hidden ${CMAKE_CXX_FLAGS_MINSIZEREL}")
    if(NOT USE_VULKAN)
        set(CMAKE_CXX_FLAGS_RELEASE    "-fno-exceptions ${CMAKE_CXX_FLAGS_RELEASE}")
        set(CMAKE_CXX_FLAGS_MINSIZEREL "-fno-exceptions ${CMAKE_CXX_FLAGS_MINSIZEREL}")
    endif()

    if(NOT CMAKE_BUILD_TYPE MATCHES "Deb")
        if(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
            # remove '-rdynamic' for Release build
            string(REPLACE "-rdynamic" "" CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS ${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS})
        endif()
        set(CMAKE_CXX_FLAGS "-Wno-deprecated-declarations ${CMAKE_CXX_FLAGS}") # disable deprecated declarations warnings for Release build
    endif()
    # enable warnings
    set(CMAKE_CXX_FLAGS "-Wall ${CMAKE_CXX_FLAGS}")

    if(USE_LINK_TIME_OPTIMIZATION)
        set(CMAKE_CXX_FLAGS_RELEASE    "-flto ${CMAKE_CXX_FLAGS_RELEASE}")
        set(CMAKE_CXX_FLAGS_MINSIZEREL "-flto ${CMAKE_CXX_FLAGS_RELEASE}")
    endif()

    if(CMAKE_MODULE_LINKER_FLAGS)
        # allow undefined references for modules
        string(REPLACE "-Wl,--no-undefined," "-Wl," CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS})
        string(REPLACE "-Wl,--no-undefined" "" CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS})
        string(REPLACE "--no-undefined," "" CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS})
        string(REPLACE ",--no-undefined" "" CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS})
        string(REPLACE "--no-undefined" "" CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS})
    endif()
endif()

if(APPLE)
    set(MAC_BUNDLE_PATH "${CMAKE_INSTALL_PREFIX}/QMPlay2.app")

    # set paths to Bundle
    set(CMAKE_INSTALL_DATAROOTDIR "${MAC_BUNDLE_PATH}/Contents/share")
    set(CMAKE_INSTALL_LIBDIR      "${MAC_BUNDLE_PATH}/Contents/MacOS")
elseif(NOT WIN32)
    include(GNUInstallDirs)
endif()

if(NOT APPLE)
    set(INSTALL_PATH_ICONS "${CMAKE_INSTALL_DATAROOTDIR}/icons")
    set(INSTALL_PATH_MIME "${CMAKE_INSTALL_DATAROOTDIR}/mime")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" AND CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
    set(INSTALL_PATH_MAN "man") # exception for FreeBSD
else()
    set(INSTALL_PATH_MAN ${CMAKE_INSTALL_MANDIR})
endif()

if(APPLE)
    find_library(APPKIT_LIBRARY AppKit)
    find_library(IOKIT_LIBRARY IOKit)
    set(SOLID_ACTIONS_DEFAULT "None")
    set(DEFAULT_INSTALL_RPATH ON)
endif()

if(NOT WIN32 AND NOT APPLE)
    # RPATH
    option(SET_INSTALL_RPATH "Set RPATH for executable after install" ${DEFAULT_INSTALL_RPATH})

    # Solid actions
    set(SOLID_ACTIONS_INSTALL_PATH ${SOLID_ACTIONS_DEFAULT} CACHE STRING "The destination path for the Solid actions (\"None\" to disable)")
endif()

if(USE_FREEDESKTOP_NOTIFICATIONS OR USE_MPRIS2 OR USE_DBUS_SUSPEND)
    find_package(Qt5DBus REQUIRED)
endif()

if(USE_MEDIABROWSER)
    find_package(Qt5Qml REQUIRED)
    set(USE_QML ON)
endif()

find_package(Qt5Svg REQUIRED)
if(ANDROID)
    find_package(Qt5AndroidExtras REQUIRED)
endif()

if(USE_VULKAN)
    set(Vulkan_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/src/qmplay2/vulkan/headers/include")
    set(QMVK_LIBRARY_TYPE STATIC)
    set(QMVK_USE_PCH ${USE_PCH})
    set(QMVK_WAIT_TIMEOUT_MS 2500)
    add_subdirectory(src/qmvk)
endif()
add_subdirectory(src/qmplay2)
add_subdirectory(src/modules)
add_subdirectory(src/gui)
if(LANGUAGES)
    add_subdirectory(lang)
endif()

if(WIN32)
    install(FILES AUTHORS ChangeLog LICENSE README.md DESTINATION ${CMAKE_INSTALL_PREFIX})
else()
    install(FILES AUTHORS ChangeLog LICENSE README.md DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/qmplay2")
endif()

if(NOT APPLE AND NOT WIN32)
    # uninstall target
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/cmake/Uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
endif()

feature_summary(WHAT ENABLED_FEATURES DESCRIPTION "Enabled features:")
feature_summary(WHAT DISABLED_FEATURES DESCRIPTION "Disabled features:")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
