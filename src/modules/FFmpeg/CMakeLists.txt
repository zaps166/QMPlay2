project(FFmpeg)
cmake_minimum_required(VERSION 3.0.2)
FIND_PACKAGE(PkgConfig REQUIRED)

set(FFmpeg_HDR
    FFmpeg.hpp
    FFDemux.hpp
    FFDec.hpp
    FFDecSW.hpp
    FFReader.hpp
    FFCommon.hpp
    FormatContext.hpp
)

set(FFmpeg_SRC
    FFmpeg.cpp
    FFDemux.cpp
    FFDec.cpp
    FFDecSW.cpp
    FFReader.cpp
    FFCommon.cpp
    FormatContext.cpp
)

set(FFmpeg_RESOURCES
    icons.qrc
)

PKG_CHECK_MODULES(LIBAVFORMAT libavformat REQUIRED)
PKG_CHECK_MODULES(LIBAVCODEC libavcodec REQUIRED)
PKG_CHECK_MODULES(LIBSWSCALE libswscale REQUIRED)
PKG_CHECK_MODULES(LIBAVUTIL libavutil REQUIRED)
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

add_definitions(-D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS)

if(USE_FFMPEG_AVDEVICE) # Common HWAccel
    add_definitions(-DQMPlay2_libavdevice)
    list(APPEND FFmpeg_HDR FFDecHWAccel.hpp HWAccelHelper.hpp)
    list(APPEND FFmpeg_SRC FFDecHWAccel.cpp HWAccelHelper.cpp)
    PKG_CHECK_MODULES(LIBAVDEVICE libavdevice REQUIRED)
    include_directories(${LIBAVDEVICE_INCLUDE_DIRS})
    link_directories(${LIBAVDEVICE_LIBRARY_DIRS})
    list(APPEND FFmpeg_LIBS ${LIBAVDEVICE_LIBRARIES})
endif()

if(USE_FFMPEG_VAAPI) # VAAPI
    add_definitions(-DQMPlay2_VAAPI)
    list(APPEND FFmpeg_HDR FFDecVAAPI.hpp VAAPIWriter.hpp)
    list(APPEND FFmpeg_SRC FFDecVAAPI.cpp VAAPIWriter.cpp)
    PKG_CHECK_MODULES(LIBS_VAAPI libva libva-x11 REQUIRED)
    include_directories(${LIBS_VAAPI_INCLUDE_DIRS})
    link_directories(${LIBS_VAAPI_LIBRARY_DIRS})
    list(APPEND FFmpeg_LIBS ${LIBS_VAAPI_LIBRARIES})
endif()

if(USE_FFMPEG_VDPAU) # VDPAU
    add_definitions(-DQMPlay2_VDPAU)
    list(APPEND FFmpeg_HDR FFDecVDPAU.hpp VDPAUWriter.hpp FFDecVDPAU_NW.hpp)
    list(APPEND FFmpeg_SRC FFDecVDPAU.cpp VDPAUWriter.cpp FFDecVDPAU_NW.cpp)
    PKG_CHECK_MODULES(LIBS_VDPAU vdpau REQUIRED)
    include_directories(${LIBS_VDPAU_INCLUDE_DIRS})
    link_directories(${LIBS_VDPAU_LIBRARY_DIRS})
    list(APPEND FFmpeg_LIBS ${LIBS_VDPAU_LIBRARIES})
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

add_library(${PROJECT_NAME} MODULE
    ${FFmpeg_HDR}
    ${FFmpeg_SRC}
    ${FFmpeg_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets Network)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtGui Qt4::QtCore Qt4::QtNetwork)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    ${FFmpeg_LIBS}
    $<TARGET_FILE:qmplay2>
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
