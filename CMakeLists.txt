cmake_minimum_required(VERSION 2.8.6)
project(QMPlay2)

# set default build type as Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/src/cmake/Modules/")

include(FeatureSummary)
find_package(PkgConfig REQUIRED)

if(CMAKE_VERSION VERSION_LESS 2.8.9)
    # use Qt5 only if CMake >= 2.8.9
    set(DEFAULT_QT5 OFF)
elseif(NOT DEFINED USE_QT5 OR USE_QT5)
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
else()
    set(DEFAULT_ALSA OFF)
    set(DEFAULT_PORTAUDIO ON)
endif()

add_definitions(-D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS)

if(WIN32)
    option(USE_CMD "Show CMD when running QMPlay2" OFF)
    mark_as_advanced(USE_CMD)
    add_feature_info(CMD USE_CMD "Show CMD when running QMPlay2")
endif()

option(USE_JEMALLOC "Link to jemalloc" OFF)

option(USE_QT5 "Build with Qt5" ${DEFAULT_QT5})
add_feature_info(Qt5 USE_QT5 "Build with Qt5")
option(USE_PROSTOPLEER "Build with ProstoPleer support" ON)
add_feature_info(ProstoPleer USE_PROSTOPLEER "Build with ProstoPleer support")
option(USE_LASTFM "Build with LastFM support" ON)
add_feature_info(LastFM USE_LASTFM "Build with LastFM support")
option(USE_AVRESAMPLE "Use libavresample instead of libswresample" OFF)
add_feature_info(libavresample USE_AVRESAMPLE "Use libavresample instead of libswresample")

option(USE_LIBASS "Build with libass support" ON)
add_feature_info(libass USE_LIBASS "Build with libass support")

option(USE_INPUTS "Build with Inputs module" ON)
add_feature_info(Inputs USE_INPUTS "Build with Inputs module")

option(USE_MODPLUG "Build with Modplug module" ON)
add_feature_info(Modplug USE_MODPLUG "Build with Modplug module")

option(USE_EXTENSIONS "Build with Extensions module" ON)
add_feature_info(Extensions USE_EXTENSIONS "Build with Extensions module")

if(USE_EXTENSIONS AND NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    option(USE_MPRIS2 "Build Extensions with MPRIS2 support" ON)
    add_feature_info(MPRIS2 USE_MPRIS2 "Build Extensions with MPRIS2 support")
endif()

option(USE_VISUALIZATIONS "Build with Visualizations module" ON)
add_feature_info(Visualizations USE_VISUALIZATIONS "Build with Visualizations module")

option(USE_AUDIOFILTERS "Build with AudioFilters module" ON)
add_feature_info(AudioFilters USE_AUDIOFILTERS "Build with AudioFilters module")

option(USE_VIDEOFILTERS "Build with VideoFilters module" ON)
add_feature_info(VideoFilters USE_VIDEOFILTERS "Build with VideoFilters module")

option(USE_OPENGL2 "Build with OpenGL2 module" ON)
add_feature_info(OpenGL2 USE_OPENGL2 "Build with OpenGL2 module")

option(USE_PORTAUDIO "Build with PortAudio module" ${DEFAULT_PORTAUDIO})
add_feature_info(PortAudio USE_PORTAUDIO "Build with PortAudio module")

option(USE_TAGLIB "Build with tags editor" ON)
add_feature_info(TagLib USE_TAGLIB "Build with tags editor")

option(USE_FFMPEG "Build with FFmpeg module" ON)
add_feature_info(FFmpeg USE_FFMPEG "Build with FFmpeg module")

if(USE_FFMPEG)
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

    if(WIN32)
        option(USE_FFMPEG_DXVA2 "Build DXVA2 acceleration into FFmpeg" ON)
        add_feature_info(DXVA2 USE_FFMPEG_DXVA2 "Build DXVA2 acceleration into FFmpeg")
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
endif()

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

if(NOT DEFINED USE_XVIDEO)
    pkg_check_modules(LIB_X11_XV x11 xv)
endif()
option(USE_XVIDEO "Build with XVideo module" ${LIB_X11_XV_FOUND})
add_feature_info(XVideo USE_XVIDEO "Build with XVideo module")

option(USE_CUVID "Build with CUVID module" ON)
add_feature_info(CUVID USE_CUVID "Build with CUVID module")

option(USE_NOTIFIES "Build with Notifications support" ON)
add_feature_info(Notifications USE_NOTIFIES "Build with Notifications support")

if(USE_QT5 AND NOT (Qt5Widgets_VERSION VERSION_LESS 5.6.0))
    option(USE_OPENGL_FOR_VISUALIZATIONS "Use \"QOpenGLWidget\" for Visualizations" OFF)
    add_feature_info(VisualizationsOpenGL USE_OPENGL_FOR_VISUALIZATIONS "Use \"QOpenGLWidget\" for Visualizations")
endif()

set(LANGUAGES "All" CACHE STRING "A space-seperated list of translations to compile into QMPlay2 or \"All\"")

if(NOT WIN32)
    # get QMPlay2 version by running the version script
    execute_process(
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/version
        OUTPUT_VARIABLE QMPLAY2_VERSION)
    string(STRIP ${QMPLAY2_VERSION} QMPLAY2_VERSION)

    #get QMPlay2 Git HEAD by running git command
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE QMPLAY2_GIT_HEAD)
    if(QMPLAY2_GIT_HEAD)
        string(STRIP ${QMPLAY2_GIT_HEAD} QMPLAY2_GIT_HEAD)
        add_definitions(-DQMPlay2GitHEAD="-git-${QMPLAY2_GIT_HEAD}")
    endif()
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
        find_package(Qt4 REQUIRED QtCore QtGui)
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

    # disable deprecated declarations warnings for Release build
    if(NOT CMAKE_BUILD_TYPE MATCHES "Deb")
        set(CMAKE_CXX_FLAGS "-Wno-deprecated-declarations ${CMAKE_CXX_FLAGS}")
    endif()
    # enable warnings
    set(CMAKE_CXX_FLAGS "-Wall ${CMAKE_CXX_FLAGS}")

    if(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
        # remove '-rdynamic'
        string(REPLACE "-rdynamic" "" CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS ${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS})
    endif()

    if(CMAKE_MODULE_LINKER_FLAGS)
        # allow undefined references for modules
        string(REPLACE "-Wl,--no-undefined," "-Wl," CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS})
        string(REPLACE "-Wl,--no-undefined" "" CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS})
        string(REPLACE "--no-undefined," "" CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS})
        string(REPLACE ",--no-undefined" "" CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS})
        string(REPLACE "--no-undefined" "" CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS})
    endif()

    if(USE_JEMALLOC AND CMAKE_EXE_LINKER_FLAGS)
        # link to all libraries also when unneeded (allows to use jemalloc)
        string(REPLACE "-Wl,--as-needed," "-Wl," CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
        string(REPLACE "-Wl,--as-needed" "" CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
        string(REPLACE "--as-needed," "" CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
        string(REPLACE ",--as-needed" "" CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
        string(REPLACE "--as-needed" "" CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
    endif()
endif()

include(GNUInstallDirs)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(INSTALL_PATH_ICONS "${CMAKE_INSTALL_DATAROOTDIR}/icons")
    set(INSTALL_PATH_MIME "${CMAKE_INSTALL_DATAROOTDIR}/mime")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" AND CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
    set(INSTALL_PATH_MAN "man") # exception for FreeBSD
else()
    set(INSTALL_PATH_MAN "${CMAKE_INSTALL_MANDIR}")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(SOLID_ACTIONS_DEFAULT "None")
    set(DEFAULT_INSTALL_RPATH ON)
endif()

if(NOT WIN32)
    # RPATH
    option(SET_INSTALL_RPATH "Set RPATH for executable after install" ${DEFAULT_INSTALL_RPATH})

    # Solid actions
    set(SOLID_ACTIONS_INSTALL_PATH ${SOLID_ACTIONS_DEFAULT} CACHE STRING "The destination path for the Solid actions (\"None\" to disable)")
endif()

add_subdirectory(src)
if(LANGUAGES)
    add_subdirectory(lang)
endif()

if(WIN32)
    install(FILES AUTHORS ChangeLog LICENSE TODO README.md DESTINATION "${CMAKE_INSTALL_PREFIX}/")
else()
    install(FILES AUTHORS ChangeLog LICENSE TODO README.md DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/qmplay2")
endif()

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cmake/Uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

message(STATUS "")
PRINT_ENABLED_FEATURES()
PRINT_DISABLED_FEATURES()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
