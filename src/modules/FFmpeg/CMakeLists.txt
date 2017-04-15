cmake_minimum_required(VERSION 2.8.6)
if(COMMAND cmake_policy)
    if(POLICY CMP0003)
        cmake_policy(SET CMP0003 NEW)
    endif()
    if(POLICY CMP0020)
        cmake_policy(SET CMP0020 NEW)
    endif()
    if(POLICY CMP0042)
        cmake_policy(SET CMP0042 NEW)
    endif()
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif()
endif()
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
    list(APPEND FFmpeg_HDR FFDecHWAccel.hpp HWAccelHelper.hpp)
    list(APPEND FFmpeg_SRC FFDecHWAccel.cpp HWAccelHelper.cpp)

    if(USE_FFMPEG_VAAPI OR USE_FFMPEG_VDPAU) # libX11 for VA-API and VDPAU
        pkg_check_modules(LIBX11 x11 REQUIRED)
        include_directories(${LIBX11_INCLUDE_DIRS})
        link_directories(${LIBX11_LIBRARY_DIRS})
        list(APPEND FFmpeg_LIBS ${LIBX11_LIBRARIES})
    endif()

    if(USE_FFMPEG_VAAPI) # VA-API
        add_definitions(-DQMPlay2_VAAPI)
        list(APPEND FFmpeg_HDR FFDecVAAPI.hpp VAAPI.hpp VAAPIWriter.hpp)
        list(APPEND FFmpeg_SRC FFDecVAAPI.cpp VAAPI.cpp VAAPIWriter.cpp)
        pkg_check_modules(LBVA libva libva-x11 libva-glx REQUIRED)
        include_directories(${LBVA_INCLUDE_DIRS})
        link_directories(${LBVA_LIBRARY_DIRS})
        list(APPEND FFmpeg_LIBS ${LBVA_LIBRARIES})
    endif()

    if(USE_FFMPEG_VDPAU) # VDPAU
        add_definitions(-DQMPlay2_VDPAU)
        list(APPEND FFmpeg_HDR FFDecVDPAU.hpp VDPAUWriter.hpp FFDecVDPAU_NW.hpp)
        list(APPEND FFmpeg_SRC FFDecVDPAU.cpp VDPAUWriter.cpp FFDecVDPAU_NW.cpp)
        pkg_check_modules(LIBVDPAU vdpau REQUIRED)
        include_directories(${LIBVDPAU_INCLUDE_DIRS})
        link_directories(${LIBVDPAU_LIBRARY_DIRS})
        list(APPEND FFmpeg_LIBS ${LIBVDPAU_LIBRARIES})
    endif()

    if(USE_FFMPEG_DXVA2) # DXVA2
        add_definitions(-DQMPlay2_DXVA2)
        list(APPEND FFmpeg_HDR FFDecDXVA2.hpp)
        list(APPEND FFmpeg_SRC FFDecDXVA2.cpp)
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

if(USE_QT5)
    qt5_add_resources(FFmpeg_RESOURCES_RCC ${FFmpeg_RESOURCES})
else()
    qt4_add_resources(FFmpeg_RESOURCES_RCC ${FFmpeg_RESOURCES})
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
    ${FFmpeg_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui)
endif()

add_dependencies(${PROJECT_NAME} libqmplay2)
target_link_libraries(${PROJECT_NAME}
    ${FFmpeg_LIBS}
    ${qmplay2lib}
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
