cmake_minimum_required(VERSION 3.1)
project(libqmplay2)

set(QMPLAY2_HDR
    headers/QMPlay2Lib.hpp
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
    headers/QMPlay2OSD.hpp
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
    headers/ChapterProgramInfo.hpp
    headers/PacketBuffer.hpp
    headers/Buffer.hpp
    headers/NetworkAccess.hpp
    headers/IPC.hpp
    headers/CPU.hpp
    headers/Version.hpp
    headers/PixelFormats.hpp
    headers/HWAccelInterface.hpp
    headers/VideoAdjustment.hpp
    headers/YouTubeDL.hpp
    headers/Notifies.hpp
    headers/NotifiesTray.hpp
    headers/MkvMuxer.hpp
    headers/CppUtils.hpp
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
    QMPlay2OSD.cpp
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
    NetworkAccess.cpp
    Version.cpp
    YouTubeDL.cpp
    Notifies.cpp
    NotifiesTray.cpp
    MkvMuxer.cpp
)

if(WIN32)
    list(APPEND QMPLAY2_SRC IPC_Windows.cpp)
else()
    list(APPEND QMPLAY2_SRC IPC_Unix.cpp)
endif()

if(USE_GIT_VERSION AND QMPLAY2_GIT_HEAD)
    set_source_files_properties(Version.cpp PROPERTIES COMPILE_DEFINITIONS QMPlay2GitHEAD="-git-${QMPLAY2_GIT_HEAD}")
endif()

add_definitions(-DQMPLAY2SHAREDLIB_LIBRARY)

if(USE_FREEDESKTOP_NOTIFICATIONS)
    list(APPEND QMPLAY2_HDR headers/NotifiesFreedesktop.hpp)
    list(APPEND QMPLAY2_SRC NotifiesFreedesktop.cpp)
    qt5_add_dbus_interface(QMPLAY2_SRC org.freedesktop.Notifications.xml notifications_interface)
    add_definitions(-DNOTIFIES_FREEDESKTOP)
    set(DBUS Qt5::DBus)
elseif(APPLE)
    list(APPEND QMPLAY2_HDR headers/NotifiesMacOS.hpp)
    list(APPEND QMPLAY2_SRC NotifiesMacOS.mm)
    find_package(Qt5MacExtras REQUIRED)
    add_definitions(-DNOTIFIES_MACOS)
endif()

set(QMPLAY2_RESOURCES
    languages.qrc
)

pkg_check_modules(LIBAVFORMAT libavformat>=56.15.102 REQUIRED)
pkg_check_modules(LIBAVCODEC libavcodec>=56.13.100 REQUIRED)
pkg_check_modules(LIBSWSCALE libswscale>=2.5.102 REQUIRED)
pkg_check_modules(LIBAVUTIL libavutil>=54.15.100 REQUIRED)

if(USE_LIBASS)
    add_definitions(-DQMPLAY2_LIBASS)
    pkg_check_modules(LIBASS libass REQUIRED)
    include_directories(${LIBASS_INCLUDE_DIRS})
    link_directories(${LIBASS_LIBRARY_DIRS})
endif()

link_directories(
    ${LIBAVFORMAT_LIBRARY_DIRS}
    ${LIBAVCODEC_LIBRARY_DIRS}
    ${LIBSWSCALE_LIBRARY_DIRS}
    ${LIBAVUTIL_LIBRARY_DIRS}
)

set(LIBQMPLAY2_LIBS
    ${LIBAVFORMAT_LIBRARIES}
    ${LIBAVCODEC_LIBRARIES}
    ${LIBSWSCALE_LIBRARIES}
    ${LIBAVUTIL_LIBRARIES}
    # libass will be added later if enabled
)

include_directories(
    headers
    ${LIBAVFORMAT_INCLUDE_DIRS}
    ${LIBAVCODEC_INCLUDE_DIRS}
    ${LIBSWSCALE_INCLUDE_DIRS}
    ${LIBAVUTIL_INCLUDE_DIRS}
)

if(USE_AVRESAMPLE)
    add_definitions(-DQMPLAY2_AVRESAMPLE)
    pkg_check_modules(LIBAVRESAMPLE libavresample>=1.2.0 REQUIRED)
    list(APPEND LIBQMPLAY2_LIBS ${LIBAVRESAMPLE_LIBRARIES})
    include_directories(${LIBAVRESAMPLE_INCLUDE_DIRS})
    link_directories(${LIBAVRESAMPLE_LIBRARY_DIRS})
else()
    pkg_check_modules(LIBSWRESAMPLE libswresample>=0.18.100 REQUIRED)
    list(APPEND LIBQMPLAY2_LIBS ${LIBSWRESAMPLE_LIBRARIES})
    include_directories(${LIBSWRESAMPLE_INCLUDE_DIRS})
    link_directories(${LIBSWRESAMPLE_LIBRARY_DIRS})
endif()

add_library(${PROJECT_NAME} SHARED
    ${QMPLAY2_HDR}
    ${QMPLAY2_SRC}
    ${QMPLAY2_RESOURCES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

if(APPLE)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Qt5::MacExtras
        ${APPKIT_LIBRARY}
    )
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        powrprof
        winmm
    )
    set(CUSTOM_LIBQMPLAY2_LIBRARIES "" CACHE STRING "Custom libraries for libqmplay2")
    mark_as_advanced(CUSTOM_LIBQMPLAY2_LIBRARIES)
    if(CUSTOM_LIBQMPLAY2_LIBRARIES)
        separate_arguments(CUSTOM_LIBQMPLAY2_LIBRARIES)
        list(APPEND LIBQMPLAY2_LIBS ${CUSTOM_LIBQMPLAY2_LIBRARIES})
    elseif(USE_LIBASS)
        list(APPEND LIBQMPLAY2_LIBS ${LIBASS_LIBRARIES})
    endif()

    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
else()
    install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    if(NOT APPLE)
        install(FILES ${QMPLAY2_HDR} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QMPlay2")
    endif()

    if(USE_LIBASS)
        list(APPEND LIBQMPLAY2_LIBS ${LIBASS_LIBRARIES})
    endif()
endif()

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    PRIVATE
    ${DBUS}
    ${LIBQMPLAY2_LIBS}
)
