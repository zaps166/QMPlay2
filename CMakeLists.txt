cmake_minimum_required(VERSION 2.8.11)
project(QMPlay2)

# set default build type as Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/src/cmake/Modules/")

include(FeatureSummary)
find_package(PkgConfig REQUIRED)

if(NOT DEFINED USE_QT5 OR USE_QT5)
    find_package(Qt5Widgets QUIET)
    if(Qt5Widgets_FOUND AND (Qt5Widgets_VERSION VERSION_GREATER 5.6.0))
        set(DEFAULT_QT5 ON)
    else()
        find_package(Qt4 QUIET)
        if(NOT Qt4_FOUND AND Qt5Widgets_FOUND)
            set(DEFAULT_QT5 ON)
        else()
            set(DEFAULT_QT5 OFF)
        endif()
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(DEFAULT_ALSA ON)
    set(DEFAULT_PORTAUDIO OFF)

    pkg_check_modules(LIBAVDEVICE libavdevice)
    set(DEFAULT_AVDEVICE ${LIBAVDEVICE_FOUND})
else()
    set(DEFAULT_ALSA OFF)
    set(DEFAULT_PORTAUDIO ON)

    set(DEFAULT_AVDEVICE OFF)
endif()

if(WIN32)
    option(USE_CMD "Show CMD when running QMPlay2" OFF)
    mark_as_advanced(USE_CMD)
    add_feature_info(CMD USE_CMD "Show CMD when running QMPlay2")
endif()

option(USE_QT5 "Build with Qt5" ${DEFAULT_QT5})
add_feature_info(Qt5 USE_QT5 "Build with Qt5")
option(USE_PROSTOPLEER "Build with ProstoPleer support" ON)
add_feature_info(ProstoPleer USE_PROSTOPLEER "Build with ProstoPleer support")
option(USE_AVRESAMPLE "Use libavresample instead of libswresample" OFF)
add_feature_info(libavresample USE_AVRESAMPLE "Use libavresample instead of libswresample")

option(USE_LIBASS "Build with libass support" ON)
add_feature_info(libass USE_LIBASS "Build with libass support")

option(USE_ALSA "Build with ALSA support" ${DEFAULT_ALSA})
add_feature_info(ALSA USE_ALSA "Build with ALSA support")
option(USE_OPENGL "Build with OpenGL support" ON)
add_feature_info(OpenGL USE_OPENGL "Build with OpenGL support")
option(USE_PORTAUDIO "Build with PortAudio support" ${DEFAULT_PORTAUDIO})
add_feature_info(PortAudio USE_PORTAUDIO "Build with PortAudio support")
option(USE_TAGLIB "Build with tags editor" ON)
add_feature_info(TagLib USE_TAGLIB "Build with tags editor")

if(NOT DEFINED USE_FFMPEG_VAAPI)
    pkg_check_modules(LIBS_VAAPI libva libva-x11 x11)
endif()
option(USE_FFMPEG_VAAPI "Build VAAPI acceleration into FFmpeg" ${LIBS_VAAPI_FOUND})
add_feature_info(VAAPI USE_FFMPEG_VAAPI "Build VAAPI acceleration into FFmpeg")

if(NOT DEFINED USE_FFMPEG_VDPAU)
    pkg_check_modules(LIBS_VDPAU vdpau x11)
endif()
option(USE_FFMPEG_VDPAU "Build VDPAU acceleration into FFmpeg" ${LIBS_VDPAU_FOUND})
add_feature_info(VDPAU USE_FFMPEG_VDPAU "Build VDPAU acceleration into FFmpeg")

option(USE_FFMPEG_AVDEVICE "Build FFmpeg with libavdevice suport" ${DEFAULT_AVDEVICE})
add_feature_info(libavdevice USE_FFMPEG_AVDEVICE "Build FFmpeg with libavdevice suport")

if(NOT DEFINED USE_AUDIOCD)
    pkg_check_modules(LIBCD libcdio libcddb)
endif()
option(USE_AUDIOCD "Build with AudioCD support" ${LIBCD_FOUND})
add_feature_info(AudioCD USE_AUDIOCD "Build with AudioCD support")

if(NOT DEFINED USE_CHIPTUNE_GME)
    find_package(GME)
endif()
option(USE_CHIPTUNE_GME "Build Chiptune with GME support" ${LIBGME_FOUND})
add_feature_info("Chiptune GME" USE_CHIPTUNE_GME "Build Chiptune with GME support")

if(NOT DEFINED LIBSIDPLAYFP_FOUND)
    pkg_check_modules(LIBSIDPLAYFP libsidplayfp)
endif()
option(USE_CHIPTUNE_SID "Build Chiptune with SIDPLAY support" ${LIBSIDPLAYFP_FOUND})
add_feature_info("Chiptune SIDPLAY" USE_CHIPTUNE_SID "Build Chiptune with SIDPLAY support")

if(NOT DEFINED USE_PULSEAUDIO)
    pkg_check_modules(LIBPULSE libpulse-simple)
endif()
option(USE_PULSEAUDIO "Build with PulseAudio support" ${LIBPULSE_FOUND})
add_feature_info(PulseAudio USE_PULSEAUDIO "Build with PulseAudio support")

if(NOT DEFINED USE_XVIDEO)
    pkg_check_modules(LIB_X11_XV x11 xv)
endif()
option(USE_XVIDEO "Build with XVideo support" ${LIB_X11_XV_FOUND})
add_feature_info(XVideo USE_XVIDEO "Build with XVideo support")

set(LANGUAGES "All" CACHE STRING "A space-seperated list of translations to compile into QMPlay2 or \"All\"")

if(NOT WIN32)
    # get QMPlay2 version by running the version script
    execute_process(
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/version
        OUTPUT_VARIABLE QMPLAY2_VERSION)
    string(STRIP ${QMPLAY2_VERSION} QMPLAY2_VERSION)
endif()

if(USE_QT5)
    if(NOT Qt5Widgets_FOUND)
        find_package(Qt5Widgets REQUIRED)
    endif()
    if(CMAKE_VERSION VERSION_LESS 2.8.12)
        set(CMAKE_CXX_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS} ${CMAKE_CXX_FLAGS}")
    endif()
    if(Qt5Widgets_VERSION VERSION_LESS 5.6.1)
        message(AUTHOR_WARNING "Qt5 >= 5.6.1 is recommended for stable usage")
    endif()
else()
    if(NOT Qt4_FOUND)
        find_package(Qt4 REQUIRED)
    endif()
    include("${QT_USE_FILE}")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang") # GCC or Clang
    # don't use C++11 if not necessary - QMPlay2 doesn't use it yet, so speed up compilation time by disabling it especially for GCC 6
    if(NOT USE_QT5 OR (Qt5Widgets_VERSION VERSION_LESS 5.7.0))
        set(CMAKE_CXX_FLAGS "-std=gnu++98 ${CMAKE_CXX_FLAGS}")
    elseif(CMAKE_VERSION VERSION_LESS 3.1) # CMake < 3.1 && Qt5 >= 5.7.0
        set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
    endif()

    # QMPlay2 doesn't use exceptions - disable it and also make sure that debugging symbols are disabled
    set(CMAKE_CXX_FLAGS_RELEASE "-fno-exceptions -g0 ${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-fno-exceptions -g0 ${CMAKE_CXX_FLAGS_MINSIZEREL}")
endif()

include(GNUInstallDirs)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(INSTALL_PATH_ICONS "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/icons")
    set(INSTALL_PATH_MIME "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/mime")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" AND CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
    set(INSTALL_PATH_MAN "${CMAKE_INSTALL_PREFIX}/man") # exception for FreeBSD
else()
    set(INSTALL_PATH_MAN "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_MANDIR}")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(SOLID_ACTIONS_DEFAULT "None")
    set(DEFAULT_INSTALL_RPATH ON)
endif()

# RPATH
option(SET_INSTALL_RPATH "Set RPATH for executable after install" ${DEFAULT_INSTALL_RPATH})

# Solid actions
set(SOLID_ACTIONS_INSTALL_PATH ${SOLID_ACTIONS_DEFAULT} CACHE STRING "The destination path for the Solid actions (\"None\" to disable)")

add_subdirectory(src)
if(LANGUAGES)
    add_subdirectory(lang)
endif()

if(WIN32)
    install(FILES ChangeLog LICENSE TODO README.md DESTINATION "${CMAKE_INSTALL_PREFIX}/")
else()
    install(FILES ChangeLog LICENSE TODO README.md DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/qmplay2")
endif()

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
