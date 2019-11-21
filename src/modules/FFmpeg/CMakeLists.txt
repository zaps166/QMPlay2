cmake_minimum_required(VERSION 3.1)
project(FFmpeg)

set(FFmpeg_HDR
    FFmpeg.hpp
    FFDemux.hpp
    FFDec.hpp
    FFDecSW.hpp
    FFReader.hpp
    FFCommon.hpp
    FormatContext.hpp
    OggHelper.hpp
    OpenThr.hpp
)

set(FFmpeg_SRC
    FFmpeg.cpp
    FFDemux.cpp
    FFDec.cpp
    FFDecSW.cpp
    FFReader.cpp
    FFCommon.cpp
    FormatContext.cpp
    OggHelper.cpp
    OpenThr.cpp
)

set(FFmpeg_RESOURCES
    icons.qrc
)

link_directories(
    ${LIBAVFORMAT_LIBRARY_DIRS}
    ${LIBAVCODEC_LIBRARY_DIRS}
    ${LIBSWSCALE_LIBRARY_DIRS}
    ${LIBAVUTIL_LIBRARY_DIRS}
)

set(FFmpeg_LIBS
    ${LIBAVFORMAT_LIBRARIES}
    ${LIBAVCODEC_LIBRARIES}
    ${LIBSWSCALE_LIBRARIES}
    ${LIBAVUTIL_LIBRARIES}
)

if(USE_FFMPEG_AVDEVICE)
    add_definitions(-DQMPlay2_libavdevice)
    pkg_check_modules(LIBAVDEVICE libavdevice REQUIRED)
    include_directories(${LIBAVDEVICE_INCLUDE_DIRS})
    link_directories(${LIBAVDEVICE_LIBRARY_DIRS})
    list(APPEND FFmpeg_LIBS ${LIBAVDEVICE_LIBRARIES})
endif()

if(USE_FFMPEG_VAAPI OR USE_FFMPEG_VDPAU OR USE_FFMPEG_DXVA2 OR USE_FFMPEG_VTB) # Common HWAccel
    list(APPEND FFmpeg_HDR FFDecHWAccel.hpp)
    list(APPEND FFmpeg_SRC FFDecHWAccel.cpp)

    if(USE_OPENGL2)
        add_definitions(-DUSE_OPENGL)
    endif()

    if(USE_FFMPEG_VAAPI) # VA-API
        find_package(Qt5X11Extras 5.6 REQUIRED)
        set(X11Extras Qt5::X11Extras)
        add_definitions(-DQMPlay2_VAAPI)
        list(APPEND FFmpeg_HDR FFDecVAAPI.hpp VAAPI.hpp)
        list(APPEND FFmpeg_SRC FFDecVAAPI.cpp VAAPI.cpp)
        pkg_check_modules(LBVA libva libva-glx libva-drm egl REQUIRED)
        include_directories(${LBVA_INCLUDE_DIRS})
        link_directories(${LBVA_LIBRARY_DIRS})
        list(APPEND FFmpeg_LIBS ${LBVA_LIBRARIES})
    endif()

    if(USE_FFMPEG_VDPAU) # VDPAU
        add_definitions(-DQMPlay2_VDPAU)
        list(APPEND FFmpeg_HDR FFDecVDPAU.hpp VDPAU.hpp)
        list(APPEND FFmpeg_SRC FFDecVDPAU.cpp VDPAU.cpp)
        pkg_check_modules(LIBVDPAU vdpau REQUIRED)
        include_directories(${LIBVDPAU_INCLUDE_DIRS})
        link_directories(${LIBVDPAU_LIBRARY_DIRS})
        list(APPEND FFmpeg_LIBS ${LIBVDPAU_LIBRARIES})
    endif()

    if (FIND_HWACCEL_DRIVERS_PATH AND (USE_FFMPEG_VAAPI OR USE_FFMPEG_VDPAU))
        add_definitions(-DFIND_HWACCEL_DRIVERS_PATH)
    endif()

    if(USE_FFMPEG_DXVA2) # DXVA2
        add_definitions(-DQMPlay2_DXVA2 -DINITGUID)
        list(APPEND FFmpeg_HDR FFDecDXVA2.hpp)
        list(APPEND FFmpeg_SRC FFDecDXVA2.cpp)
        set(DXVA2 dxva2)
    endif()

    if(USE_FFMPEG_VTB)
        add_definitions(-DQMPlay2_VTB)
        list(APPEND FFmpeg_HDR FFDecVTB.hpp)
        list(APPEND FFmpeg_SRC FFDecVTB.cpp)
        find_library(OPENGL_LIBRARY OpenGL)
        find_library(COREVIDEO_LIBRARY CoreVideo)
        find_library(IOSURFACE_LIBRARY IOSurface)
        list(APPEND FFmpeg_LIBS ${OPENGL_LIBRARY} ${COREVIDEO_LIBRARY} ${IOSURFACE_LIBRARY})
    endif()
endif()

include_directories(../../qmplay2/headers
    ${LIBAVFORMAT_INCLUDE_DIRS}
    ${LIBAVCODEC_INCLUDE_DIRS}
    ${LIBSWSCALE_INCLUDE_DIRS}
    ${LIBAVUTIL_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${FFmpeg_HDR}
    ${FFmpeg_SRC}
    ${FFmpeg_RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    ${FFmpeg_LIBS}
    ${X11Extras}
    ${DXVA2}
    libqmplay2
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
