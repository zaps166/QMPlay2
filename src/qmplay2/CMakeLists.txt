cmake_minimum_required(VERSION 3.1)
project(libqmplay2)

set(QMPLAY2_HDR
    QMPlay2Lib.hpp
    QMPlay2Core.hpp
    Functions.hpp
    Settings.hpp
    Module.hpp
    ModuleParams.hpp
    ModuleCommon.hpp
    Playlist.hpp
    Reader.hpp
    Demuxer.hpp
    Decoder.hpp
    VideoFilters.hpp
    VideoFilter.hpp
    DeintHWPrepareFilter.hpp
    AudioFilter.hpp
    Writer.hpp
    QMPlay2Extensions.hpp
    LineEdit.hpp
    Slider.hpp
    QMPlay2OSD.hpp
    InDockW.hpp
    LibASS.hpp
    ColorButton.hpp
    ImgScaler.hpp
    SndResampler.hpp
    VideoWriter.hpp
    SubsDec.hpp
    ByteArray.hpp
    Packet.hpp
    Frame.hpp
    StreamInfo.hpp
    DockWidget.hpp
    IOController.hpp
    ChapterProgramInfo.hpp
    PacketBuffer.hpp
    NetworkAccess.hpp
    IPC.hpp
    CPU.hpp
    Version.hpp
    HWOpenGLInterop.hpp
    VideoAdjustment.hpp
    YouTubeDL.hpp
    Notifies.hpp
    NotifiesTray.hpp
    MkvMuxer.hpp
    CppUtils.hpp
    Sphere.hpp
    X11BypassCompositor.hpp
    VideoOutputCommon.hpp
    HWDecContext.hpp
    GPUInstance.hpp
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
    DeintHWPrepareFilter.cpp
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
    Packet.cpp
    Frame.cpp
    StreamInfo.cpp
    DockWidget.cpp
    PacketBuffer.cpp
    NetworkAccess.cpp
    Version.cpp
    YouTubeDL.cpp
    Notifies.cpp
    NotifiesTray.cpp
    MkvMuxer.cpp
    Sphere.cpp
    X11BypassCompositor.cpp
    VideoOutputCommon.cpp
    GPUInstance.cpp
)

if(WIN32)
    list(APPEND QMPLAY2_SRC IPC_Windows.cpp)
else()
    list(APPEND QMPLAY2_SRC IPC_Unix.cpp)
endif()

if(USE_GIT_VERSION AND QMPLAY2_GIT_HEAD)
    set_source_files_properties(Version.cpp PROPERTIES COMPILE_DEFINITIONS QMPlay2GitHEAD="-git-${QMPLAY2_GIT_HEAD}")
endif()

if(USE_OPENGL)
    set(QMPLAY2_OPENGL
        opengl/OpenGLInstance.cpp
        opengl/OpenGLInstance.hpp
        opengl/OpenGLWriter.cpp
        opengl/OpenGLWriter.hpp
        opengl/OpenGLCommon.cpp
        opengl/OpenGLCommon.hpp
        opengl/OpenGLWindow.cpp
        opengl/OpenGLWindow.hpp
        opengl/OpenGLWidget.cpp
        opengl/OpenGLWidget.hpp
        opengl/Vertices.hpp
        opengl/opengl.qrc
    )
    if(APPLE)
        find_package(OpenGL REQUIRED)
        list(APPEND OPENGL_LIBS ${OPENGL_LIBRARIES})
    elseif(WIN32)
        list(APPEND OPENGL_LIBS opengl32)
    endif()

endif()

add_definitions(-DQMPLAY2SHAREDLIB_LIBRARY)

if(USE_FREEDESKTOP_NOTIFICATIONS)
    list(APPEND QMPLAY2_HDR NotifiesFreedesktop.hpp)
    list(APPEND QMPLAY2_SRC NotifiesFreedesktop.cpp)
    qt5_add_dbus_interface(QMPLAY2_SRC org.freedesktop.Notifications.xml notifications_interface)
    add_definitions(-DNOTIFIES_FREEDESKTOP)
    set(DBUS Qt5::DBus)
elseif(APPLE)
    list(APPEND QMPLAY2_HDR NotifiesMacOS.hpp)
    list(APPEND QMPLAY2_SRC NotifiesMacOS.mm)
    find_package(Qt5MacExtras REQUIRED)
    add_definitions(-DNOTIFIES_MACOS)
endif()

if(USE_QML)
    list(APPEND QMPLAY2_HDR
        CommonJS.hpp
        TreeWidgetJS.hpp
        NetworkAccessJS.hpp
    )
    list(APPEND QMPLAY2_SRC
        CommonJS.cpp
        TreeWidgetJS.cpp
        NetworkAccessJS.cpp
    )
    add_definitions(-DUSE_QML)
    set(QML Qt5::Qml)
endif()

set(QMPLAY2_RESOURCES
    languages.qrc
)

if(USE_LIBASS)
    add_definitions(-DQMPLAY2_LIBASS)
    pkg_check_modules(LIBASS libass REQUIRED)
    include_directories(${LIBASS_INCLUDE_DIRS})
    link_directories(${LIBASS_LIBRARY_DIRS})
endif()

add_library(${PROJECT_NAME} SHARED
    ${QMPLAY2_HDR}
    ${QMPLAY2_SRC}
    ${QMPLAY2_RESOURCES}
    ${QMPLAY2_OPENGL}
)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBAVFORMAT_INCLUDE_DIRS}
    ${LIBAVCODEC_INCLUDE_DIRS}
    ${LIBSWSCALE_INCLUDE_DIRS}
    ${LIBAVUTIL_INCLUDE_DIRS}
    ${LIBSWRESAMPLE_INCLUDE_DIRS}
)

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
    Qt5::Svg
    ${LIBAVFORMAT_LIBRARIES}
    ${LIBAVCODEC_LIBRARIES}
    ${LIBSWSCALE_LIBRARIES}
    ${LIBAVUTIL_LIBRARIES}
    ${LIBSWRESAMPLE_LIBRARIES}
    ${OPENGL_LIBS}
    PRIVATE
    ${DBUS}
    ${QML}
    ${LIBQMPLAY2_LIBS}
)

if(ANDROID)
    # Android libs must be public, because androiddeployqt copies them
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
        Qt5::AndroidExtras
        ${QML}
    )

    if (ANDROID_ABI STREQUAL "armeabi-v7a")
        set(ANDROID_EXTRA_LIBS_DIR "${CMAKE_SYSROOT}/usr/lib/arm-linux-androideabi")
    endif()
    set(ANDROID_EXTRA_LIBS
        "${ANDROID_EXTRA_LIBS_DIR}/libavcodec.so,${ANDROID_EXTRA_LIBS_DIR}/libavformat.so,${ANDROID_EXTRA_LIBS_DIR}/libavutil.so,${ANDROID_EXTRA_LIBS_DIR}/libswresample.so,${ANDROID_EXTRA_LIBS_DIR}/libswscale.so"
        CACHE INTERNAL ""
    )

    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/gui/Android/manifest" CACHE INTERNAL "")
endif()
