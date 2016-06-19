cmake_minimum_required(VERSION 2.8.11)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0043 NEW)
endif()
project(qmplay2)

include(FeatureSummary)

find_package(PkgConfig REQUIRED)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/src/cmake/Modules/")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(ALSA QUIET)
    set(DEFAULT_ALSA ${ALSA_FOUND})

    set(DEFAULT_PORTAUDIO OFF)
else()
    set(DEFAULT_ALSA OFF)
    set(DEFAULT_PORTAUDIO ON)
endif()

find_package(Qt5Widgets QUIET)
find_package(Qt4 QUIET)
if(Qt5Widgets_FOUND AND (NOT Qt4_FOUND OR NOT (Qt5Widgets_VERSION VERSION_LESS 5.6.1)))
    set(DEFAULT_QT5 ON)
else()
    set(DEFAULT_QT5 OFF)
endif()

OPTION(USE_QT5 "Build with Qt5" ${DEFAULT_QT5})
add_feature_info(Qt5 USE_QT5 "Build with Qt5")
OPTION(USE_PROSTOPLEER "Build with ProstoPleer support" ON)
add_feature_info(ProstoPleer USE_PROSTOPLEER "Build with ProstoPleer support")
OPTION(USE_AVRESAMPLE "Build with support for libavresample" OFF)
add_feature_info(libavresample USE_AVRESAMPLE "Build with support for libavresample")

OPTION(USE_ALSA "Build with ALSA support" ${DEFAULT_ALSA})
add_feature_info(ALSA USE_ALSA "Build with ALSA support")
OPTION(USE_OPENGL "Build with OpenGL support" ON)
add_feature_info(OpenGL USE_OPENGL "Build with OpenGL support")
OPTION(USE_PORTAUDIO "Build with PortAudio support" ${DEFAULT_PORTAUDIO})
add_feature_info(PortAudio USE_PORTAUDIO "Build with PortAudio support")

if(NOT DEFINED USE_TAGLIB)
    PKG_CHECK_MODULES(TAGLIB taglib QUIET)
endif()
OPTION(USE_TAGLIB "Build with tags editor" ${TAGLIB_FOUND})
add_feature_info(taglib USE_TAGLIB "Build with tags editor")

if(NOT DEFINED USE_AUDIOCD)
    PKG_CHECK_MODULES(LIBCD libcdio libcddb QUIET)
endif()
OPTION(USE_AUDIOCD "Build with AudioCD support" ${LIBCD_FOUND})
add_feature_info(AudioCD USE_AUDIOCD "Build with AudioCD support")

if(NOT DEFINED USE_CHIPTUNE_GME)
    find_package(GME QUIET)
endif()
OPTION(USE_CHIPTUNE_GME "Build Chiptune with GME support" ${LIBGME_FOUND})
add_feature_info("Chiptune GME" USE_CHIPTUNE_GME "Build Chiptune with GME support")

if(NOT DEFINED LIBSIDPLAYFP_FOUND)
    PKG_CHECK_MODULES(LIBSIDPLAYFP libsidplayfp QUIET)
endif()
OPTION(USE_CHIPTUNE_SID "Build Chiptune with SIDPLAY support" ${LIBSIDPLAYFP_FOUND})
add_feature_info("Chiptune SIDPLAY" USE_CHIPTUNE_SID "Build Chiptune with SIDPLAY support")

if(NOT DEFINED USE_PULSEAUDIO)
    PKG_CHECK_MODULES(LIBPULSE libpulse-simple QUIET)
endif()
OPTION(USE_PULSEAUDIO "Build with PulseAudio support" ${LIBPULSE_FOUND})
add_feature_info(PulseAudio USE_PULSEAUDIO "Build with PulseAudio support")

if(NOT DEFINED USE_XVIDEO)
    find_package(X11 QUIET)
    if(X11_FOUND AND X11_Xv_FOUND)
        set(DEFAULT_XVIDEO ON)
    else()
        set(DEFAULT_XVIDEO OFF)
    endif()
endif()
OPTION(USE_XVIDEO "Build with XVideo support" ${DEFAULT_XVIDEO})
add_feature_info(XVideo USE_XVIDEO "Build with XVideo support")

if(NOT DEFINED USE_FFMPEG_VAAPI)
    PKG_CHECK_MODULES(LIBS_VAAPI libva libva-x11 QUIET)
endif()
OPTION(USE_FFMPEG_VAAPI "Build VAAPI acceleration into FFmpeg" ${LIBS_VAAPI_FOUND})
add_feature_info(VAAPI USE_FFMPEG_VAAPI "Build VAAPI acceleration into FFmpeg")

if(NOT DEFINED USE_FFMPEG_VDPAU)
    PKG_CHECK_MODULES(LIBS_VDPAU vdpau x11 QUIET)
endif()
OPTION(USE_FFMPEG_VDPAU "Build VDPAU acceleration into FFmpeg" ${LIBS_VDPAU_FOUND})
add_feature_info(VDPAU USE_FFMPEG_VDPAU "Build VDPAU acceleration into FFmpeg")

OPTION(USE_FFMPEG_AVDEVICE "Build FFmpeg with libavdevice suport" ON)
add_feature_info(libavdevice USE_FFMPEG_AVDEVICE "Build FFmpeg with libavdevice suport")

set(LANGUAGES "All" CACHE STRING "A space-seperated list of translations to compile in to QMPlay2, \"None\", or \"All\".")

# set default build as Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# get QMPLAY2_VERSION by running the version script
execute_process(
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/version
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE QMPLAY2_VERSION)
string(STRIP ${QMPLAY2_VERSION} QMPLAY2_VERSION)

set(CMAKE_CXX_FLAGS "-fPIC ${CMAKE_CXX_FLAGS}")
if(USE_QT5)
    find_package(Qt5Widgets REQUIRED)
    if(CMAKE_VERSION VERSION_LESS "3.1")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
        endif()
    endif()
else()
    find_package(Qt4 REQUIRED)
    include("${QT_USE_FILE}")
    add_definitions(${QT_DEFINITIONS})
endif()

include(GNUInstallDirs)
set(INSTALL_PATH_ICONS "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/icons")
set(INSTALL_PATH_MIME "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/mime")
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" AND CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
    set(INSTALL_PATH_MAN "${CMAKE_INSTALL_PREFIX}/man")
else()
    set(INSTALL_PATH_MAN "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_MANDIR}")
endif()

# solid actions
set(INSTALL_PATH_SOLID_ACTION "Auto" CACHE
    STRING "The destination path for the solid actions, \"None\", or \"Auto\"")

add_subdirectory(src)
add_subdirectory(lang)

install(FILES ChangeLog LICENSE TODO README.md DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DOCDIR}")

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cmake/Uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

message("")
PRINT_ENABLED_FEATURES()
PRINT_DISABLED_FEATURES()
