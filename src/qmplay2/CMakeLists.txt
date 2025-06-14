cmake_minimum_required(VERSION 3.16)
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
    Version.hpp
    VideoAdjustment.hpp
    Notifies.hpp
    NotifiesTray.hpp
    StreamMuxer.hpp
    Sphere.hpp
    X11BypassCompositor.hpp
    VideoOutputCommon.hpp
    HWDecContext.hpp
    GPUInstance.hpp
    FFT.hpp
    PlaylistEntry.hpp
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
    Notifies.cpp
    NotifiesTray.cpp
    StreamMuxer.cpp
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
    set(QMPLAY2_OPENGL_HDR
        opengl/OpenGLInstance.hpp
        opengl/OpenGLWriter.hpp
        opengl/OpenGLCommon.hpp
        opengl/OpenGLWindow.hpp
        opengl/OpenGLWidget.hpp
        opengl/OpenGLVertices.hpp
        opengl/OpenGLHWInterop.hpp
    )
    set(QMPLAY2_OPENGL_SRC
        opengl/OpenGLInstance.cpp
        opengl/OpenGLWriter.cpp
        opengl/OpenGLCommon.cpp
        opengl/OpenGLWindow.cpp
        opengl/OpenGLWidget.cpp
        opengl/opengl.qrc
    )

    if(BUILD_WITH_QT6)
        list(APPEND OPENGL_LIBS Qt6::OpenGL Qt6::OpenGLWidgets)
        get_target_property(QtGui_Enabled_Features Qt6::Gui QT_ENABLED_PUBLIC_FEATURES)
    endif()

    if((${QT_PREFIX}Gui_OPENGL_IMPLEMENTATION STREQUAL GLESv2) OR (QtGui_Enabled_Features MATCHES "opengles"))
        if(NOT WIN32)
            list(APPEND OPENGL_LIBS GLESv2)
        endif()
    elseif(${QT_PREFIX}Gui_OPENGL_IMPLEMENTATION STREQUAL GLES)
        message(SEND_ERROR "OpenGL|ES 1.0 is not supported!")
    endif()
endif()

if(USE_VULKAN)
    set(QMPLAY2_VULKAN_HDR
        vulkan/VulkanBufferPool.hpp
        vulkan/VulkanHWInterop.hpp
        vulkan/VulkanImagePool.hpp
        vulkan/VulkanInstance.hpp
        vulkan/VulkanWindow.hpp
        vulkan/VulkanWriter.hpp
        vulkan/VulkanYadifDeint.hpp
    )
    set(QMPLAY2_VULKAN_SRC
        vulkan/VulkanBufferPool.cpp
        vulkan/VulkanHWInterop.cpp
        vulkan/VulkanImagePool.cpp
        vulkan/VulkanInstance.cpp
        vulkan/VulkanWindow.cpp
        vulkan/VulkanWriter.cpp
        vulkan/VulkanYadifDeint.cpp
    )
    set(VULKAN_SHADERS
        vulkan/shaders/osd.vert
        vulkan/shaders/osd_ass.frag
        vulkan/shaders/osd_av.frag
        vulkan/shaders/video.frag
        vulkan/shaders/video.vert
        vulkan/shaders/yadif.comp
    )
    set(VULKAN_SHADERS_INCLUDE
        shaderscommon/colorspace.glsl
        shaderscommon/hsl.glsl
    )

    if(USE_GLSLC) # Shaders compilation
        foreach(VULKAN_SHADER ${VULKAN_SHADERS})
            qmvk_add_shader(${CMAKE_CURRENT_SOURCE_DIR}/${VULKAN_SHADER} ${VULKAN_SHADERS_INCLUDE})
        endforeach()

        add_custom_target( # Needed to display GLSL files in QtCreator
            VulkanShaders
            SOURCES ${VULKAN_SHADERS} ${VULKAN_SHADERS_INCLUDE}
            DEPENDS ${QMVK_SHADER_COMPILED_FILES}
        )
    else()
        file(GLOB QMVK_SHADER_COMPILED_FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/vulkan/shaders/spv/*.spv"
        )
    endif()

    # Compiled shaders QRC generation

    set(VULKAN_SHADER_QRC_FILE "${CMAKE_CURRENT_BINARY_DIR}/vulkan_shaders.qrc")
    list(APPEND QMPLAY2_VULKAN_SRC
        ${VULKAN_SHADER_QRC_FILE}
    )

    set(VULKAN_SHADER_QRC_FILE_TMP "${VULKAN_SHADER_QRC_FILE}.tmp")

    if(ANDROID)
        set(VULKAN_SHADER_QRC_THRESHOLD " threshold=\"100\"") # Otherwise doesn't link with zstd in 6.6.2
    endif()

    foreach(SHADER_COMPILED_FILE ${QMVK_SHADER_COMPILED_FILES})
        get_filename_component(SHADER_COMPILED_FILE_NAME ${SHADER_COMPILED_FILE} NAME)
        set(SHADER_QRC_CONTENT
            ${SHADER_QRC_CONTENT}
            "  <file\ alias=\"${SHADER_COMPILED_FILE_NAME}\"${VULKAN_SHADER_QRC_THRESHOLD}>${SHADER_COMPILED_FILE}</file>\n"
        )
    endforeach()
    file(WRITE ${VULKAN_SHADER_QRC_FILE_TMP}
        "<RCC>\n <qresource\ prefix=\"/vulkan\">\n"
        ${SHADER_QRC_CONTENT}
        " </qresource>\n</RCC>\n"
    )

    add_custom_command(
        OUTPUT ${VULKAN_SHADER_QRC_FILE}
        DEPENDS ${QMVK_SHADER_COMPILED_FILES}
        COMMAND ${CMAKE_COMMAND} -E copy ${VULKAN_SHADER_QRC_FILE_TMP} ${VULKAN_SHADER_QRC_FILE}
    )

    # No need to add VulkanShaders target dependency to libqmplay2,
    # because generated QRC file is added into source file list.
endif()

add_definitions(-DQMPLAY2SHAREDLIB_LIBRARY)
if(USE_VULKAN)
    add_definitions(-DQMVK_LIBRARY)
endif()

if(USE_FREEDESKTOP_NOTIFICATIONS)
    list(APPEND QMPLAY2_HDR NotifiesFreedesktop.hpp)
    list(APPEND QMPLAY2_SRC NotifiesFreedesktop.cpp)
    if(BUILD_WITH_QT6)
        qt6_add_dbus_interface(QMPLAY2_SRC org.freedesktop.Notifications.xml notifications_interface)
    else()
        qt5_add_dbus_interface(QMPLAY2_SRC org.freedesktop.Notifications.xml notifications_interface)
    endif()
    add_definitions(-DNOTIFIES_FREEDESKTOP)
elseif(APPLE)
    list(APPEND QMPLAY2_HDR NotifiesMacOS.hpp)
    list(APPEND QMPLAY2_SRC NotifiesMacOS.mm)
    add_definitions(-DNOTIFIES_MACOS)
endif()

if(USE_FREEDESKTOP_NOTIFICATIONS OR USE_DBUS_PM OR USE_MPRIS2)
    set(DBUS ${QT_PREFIX}::DBus)
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
    set(QML ${QT_PREFIX}::Qml)
endif()

if(USE_YOUTUBEDL)
    list(APPEND QMPLAY2_HDR
        YouTubeDL.hpp
    )
    list(APPEND QMPLAY2_SRC
        YouTubeDL.cpp
    )
endif()

set(QMPLAY2_RESOURCES
    languages.qrc
)

if(USE_LIBASS)
    add_definitions(-DQMPLAY2_LIBASS)
    pkg_check_modules(LIBASS REQUIRED libass)
    include_directories(${LIBASS_INCLUDE_DIRS})
    link_directories(${LIBASS_LIBRARY_DIRS})
endif()

if(USE_RUBBERBAND)
    add_definitions(-DQMPLAY2_RUBBERBAND)
    pkg_check_modules(RUBBERBAND REQUIRED rubberband>=3.0.0)
    include_directories(${RUBBERBAND_INCLUDE_DIRS})
    link_directories(${RUBBERBAND_LIBRARY_DIRS})
    list(APPEND LIBQMPLAY2_LIBS ${RUBBERBAND_LIBRARIES})
endif()

add_library(${PROJECT_NAME} SHARED
    ${QMPLAY2_HDR}
    ${QMPLAY2_SRC}
    ${QMPLAY2_RESOURCES}
    ${QMPLAY2_OPENGL_HDR}
    ${QMPLAY2_OPENGL_SRC}
    ${QMPLAY2_VULKAN_HDR}
    ${QMPLAY2_VULKAN_SRC}
)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

if(USE_PCH)
    if(APPLE)
        set_source_files_properties(NotifiesMacOS.mm PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
    endif()
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        ${QMPLAY2_HDR}
        ${QMPLAY2_OPENGL_HDR}
        ${QMPLAY2_VULKAN_HDR}
    )
endif()

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
target_link_directories(
    ${PROJECT_NAME}
    PUBLIC
    ${LIBAVFORMAT_LIBRARY_DIRS}
    ${LIBAVCODEC_LIBRARY_DIRS}
    ${LIBSWSCALE_LIBRARY_DIRS}
    ${LIBAVUTIL_LIBRARY_DIRS}
    ${LIBSWRESAMPLE_LIBRARY_DIRS}
)

if(APPLE)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ${APPKIT_LIBRARY}
    )
endif()

if(USE_LIBASS AND (NOT WIN32 OR CMAKE_HOST_WIN32))
    list(APPEND LIBQMPLAY2_LIBS ${LIBASS_LIBRARIES})
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        powrprof
    )

    set(CUSTOM_LIBQMPLAY2_LIBRARIES "" CACHE STRING "Custom libraries for libqmplay2")
    if(CUSTOM_LIBQMPLAY2_LIBRARIES)
        separate_arguments(CUSTOM_LIBQMPLAY2_LIBRARIES)
        list(APPEND LIBQMPLAY2_LIBS ${CUSTOM_LIBQMPLAY2_LIBRARIES})
    endif()

    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
else()
    install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    if(NOT APPLE)
        install(FILES ${QMPLAY2_HDR} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QMPlay2")
    endif()
endif()

if(USE_DBUS_PM)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
        DBUS_PM
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    ${QT_PREFIX}::Core
    ${QT_PREFIX}::Gui
    ${QT_PREFIX}::Widgets
    ${QT_PREFIX}::Svg
    ${LIBAVFORMAT_LIBRARIES}
    ${LIBAVCODEC_LIBRARIES}
    ${LIBSWSCALE_LIBRARIES}
    ${LIBAVUTIL_LIBRARIES}
    ${LIBSWRESAMPLE_LIBRARIES}
    ${OPENGL_LIBS}
    ${DBUS}
    PRIVATE
    ${QML}
    ${LIBQMPLAY2_LIBS}
)
if(USE_VULKAN)
    if(ANDROID)
        target_link_libraries(${PROJECT_NAME}
            PRIVATE
            $<TARGET_OBJECTS:QmVk>
        )
    else()
        target_link_libraries(${PROJECT_NAME}
            PRIVATE
            $<TARGET_LINKER_FILE:QmVk>
        )
    endif()
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
        $<TARGET_PROPERTY:QmVk,INTERFACE_COMPILE_DEFINITIONS>
    )
    target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<TARGET_PROPERTY:QmVk,INTERFACE_INCLUDE_DIRECTORIES>
    )
endif()
if(BUILD_WITH_QT6)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
        Qt6::Core5Compat # QTextCodec
    )
endif()

if(USE_VULKAN AND NOT WIN32 AND NOT ANDROID)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        dl
    )
endif()

macro(libqmplay2_set_target_params)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        $<TARGET_PROPERTY:libqmplay2,INTERFACE_LINK_LIBRARIES>
        $<TARGET_LINKER_FILE:libqmplay2>
    )
    target_link_directories(${PROJECT_NAME}
        PRIVATE
        $<TARGET_PROPERTY:libqmplay2,INTERFACE_LINK_DIRECTORIES>
    )
    target_include_directories(${PROJECT_NAME}
        PRIVATE
        $<TARGET_PROPERTY:libqmplay2,INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        $<TARGET_PROPERTY:libqmplay2,INTERFACE_COMPILE_DEFINITIONS>
    )
endmacro()
