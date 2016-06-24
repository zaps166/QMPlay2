cmake_minimum_required(VERSION 2.8.11)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    if(POLICY CMP0042)
        cmake_policy(SET CMP0042 NEW)
    endif()
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif()
endif()
project(qmplay2)

set(QMPLAY2_HDR
    headers/QMPlay2Core.hpp
    headers/Functions.hpp
    headers/Settings.hpp
    headers/Module.hpp
    headers/ModuleParams.hpp
    headers/ModuleCommon.hpp
    headers/Playlist.hpp
    headers/Reader.hpp
    headers/Demuxer.hpp
    headers/Decoder.hpp
    headers/VideoFilters.hpp
    headers/VideoFilter.hpp
    headers/DeintFilter.hpp
    headers/AudioFilter.hpp
    headers/Writer.hpp
    headers/QMPlay2Extensions.hpp
    headers/LineEdit.hpp
    headers/Slider.hpp
    headers/QMPlay2_OSD.hpp
    headers/InDockW.hpp
    headers/LibASS.hpp
    headers/ColorButton.hpp
    headers/ImgScaler.hpp
    headers/SndResampler.hpp
    headers/VideoWriter.hpp
    headers/SubsDec.hpp
    headers/ByteArray.hpp
    headers/TimeStamp.hpp
    headers/Packet.hpp
    headers/VideoFrame.hpp
    headers/StreamInfo.hpp
    headers/DockWidget.hpp
    headers/IOController.hpp
    headers/ChapterInfo.hpp
    headers/PacketBuffer.hpp
    headers/Buffer.hpp
    headers/CPU.hpp
    headers/Version.hpp
    headers/PixelFormats.hpp
)

set(QMPLAY2_SRC
    QMPlay2Core.cpp
    Functions.cpp
    Settings.cpp
    Module.cpp
    ModuleParams.cpp
    ModuleCommon.cpp
    Playlist.cpp
    Reader.cpp
    Demuxer.cpp
    Decoder.cpp
    VideoFilters.cpp
    VideoFilter.cpp
    DeintFilter.cpp
    AudioFilter.cpp
    Writer.cpp
    QMPlay2Extensions.cpp
    LineEdit.cpp
    Slider.cpp
    QMPlay2_OSD.cpp
    InDockW.cpp
    LibASS.cpp
    ColorButton.cpp
    ImgScaler.cpp
    SndResampler.cpp
    VideoWriter.cpp
    SubsDec.cpp
    VideoFrame.cpp
    StreamInfo.cpp
    DockWidget.cpp
    PacketBuffer.cpp
    Buffer.cpp
)

set(QMPLAY2_RESOURCES
    languages.qrc
)

pkg_check_modules(LIBSWSCALE libswscale>=2.5.102 REQUIRED)
pkg_check_modules(LIBAVUTIL libavutil>=52.66.100 REQUIRED)
pkg_check_modules(LIBASS libass REQUIRED)

link_directories(
    ${LIBSWSCALE_LIBRARY_DIRS}
    ${LIBAVUTIL_LIBRARY_DIRS}
    ${LIBASS_LIBRARY_DIRS}
)

set(QMPLAY2_LIBS
    ${LIBSWSCALE_LIBRARIES}
    ${LIBAVUTIL_LIBRARIES}
    ${LIBASS_LIBRARIES}
)

include_directories(
    headers
    ${LIBSWSCALE_INCLUDE_DIRS}
    ${LIBAVUTIL_INCLUDE_DIRS}
    ${LIBASS_INCLUDE_DIRS}
)

if(USE_AVRESAMPLE)
    add_definitions(-DQMPLAY2_AVRESAMPLE)
    pkg_check_modules(LIBAVRESAMPLE libavresample>=1.2.0 REQUIRED)
    list(APPEND QMPLAY2_LIBS ${LIBAVRESAMPLE_LIBRARIES})
    include_directories(${LIBAVRESAMPLE_INCLUDE_DIRS})
    link_directories(${LIBAVRESAMPLE_LIBRARY_DIRS})
else()
    pkg_check_modules(LIBSWRESAMPLE libswresample>=0.18.100 REQUIRED)
    list(APPEND QMPLAY2_LIBS ${LIBSWRESAMPLE_LIBRARIES})
    include_directories(${LIBSWRESAMPLE_INCLUDE_DIRS})
    link_directories(${LIBSWRESAMPLE_LIBRARY_DIRS})
endif()

if(USE_QT5)
    qt5_add_resources(QMPLAY2_RESOURCES_RCC ${QMPLAY2_RESOURCES})
else()
    qt4_add_resources(QMPLAY2_RESOURCES_RCC ${QMPLAY2_RESOURCES})
endif()

add_definitions(-D__STDC_CONSTANT_MACROS)

add_library(${PROJECT_NAME} SHARED
    ${QMPLAY2_HDR}
    ${QMPLAY2_SRC}
    ${QMPLAY2_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui)
endif()

target_link_libraries(${PROJECT_NAME}
    ${QMPLAY2_LIBS}
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} powrprof winmm)
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/")
else()
    install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        install(FILES ${QMPLAY2_HDR} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/QMPlay2)
    endif()
endif()
