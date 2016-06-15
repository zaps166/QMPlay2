project(QMPlay2)
cmake_minimum_required(VERSION 3.0.2)
include(GNUInstallDirs)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(DEFAULT_ALSA ON)
    set(DEFAULT_PORTAUDIO OFF)
else()
    set(DEFAULT_ALSA OFF)
    set(DEFAULT_PORTAUDIO ON)
endif()

OPTION(USE_QT5 "Build with Qt5" ON)
OPTION(USE_TAGLIB "Build with tags editor" ON)
OPTION(USE_AVRESAMPLE "Build with support for libavresample" OFF)

OPTION(USE_ALSA "Build with ALSA support" ${DEFAULT_ALSA})
OPTION(USE_AUDIOCD "Build with AudioCD support" ON)
OPTION(USE_OPENGL "Build with OpenGL support" ON)
OPTION(USE_PORTAUDIO "Build with PortAudio support" ${DEFAULT_PORTAUDIO})
OPTION(USE_PULSEAUDIO "Build with PulseAudio support" ON)
OPTION(USE_XVIDEO "Build with XVideo support" ON)

OPTION(USE_CHIPTUNE_GME "Build Chiptune with GME support" ON)
OPTION(USE_CHIPTUNE_SID "Build Chiptune with SIDPLAY support" ON)

OPTION(USE_FFMPEG_VAAPI "Build VAAPI acceleration into FFmpeg" ON)
OPTION(USE_FFMPEG_VDPAU "Build VDPAU acceleration into FFmpeg" ON)
OPTION(USE_FFMPEG_AVDEVICE "Build FFmpeg with libavdevice suport" ON)

set(LANGUAGES "All" CACHE STRING "A space-seperated list of translations to compile in to QMPlay2, or \"None\", or \"All\".")

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

add_subdirectory(src)
add_subdirectory(lang)

install(FILES ChangeLog DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/qmplay2/)

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
