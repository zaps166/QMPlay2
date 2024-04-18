cmake_minimum_required(VERSION 3.13)
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

set(FFmpeg_LIBS
    ${LIBAVFORMAT_LIBRARIES}
    ${LIBAVCODEC_LIBRARIES}
    ${LIBSWSCALE_LIBRARIES}
    ${LIBAVUTIL_LIBRARIES}
)

if(USE_FFMPEG_AVDEVICE)
    add_definitions(-DQMPlay2_libavdevice)
    pkg_check_modules(LIBAVDEVICE REQUIRED libavdevice)
    include_directories(${LIBAVDEVICE_INCLUDE_DIRS})
    link_directories(${LIBAVDEVICE_LIBRARY_DIRS})
    list(APPEND FFmpeg_LIBS ${LIBAVDEVICE_LIBRARIES})
endif()

if(USE_FFMPEG_VKVIDEO OR USE_FFMPEG_VAAPI OR USE_FFMPEG_DXVA2 OR USE_FFMPEG_D3D11VA OR USE_FFMPEG_VTB) # Common HWAccel
    list(APPEND FFmpeg_HDR FFDecHWAccel.hpp)
    list(APPEND FFmpeg_SRC FFDecHWAccel.cpp)

    if(USE_FFMPEG_VKVIDEO) # VkVideo
        add_definitions(-DQMPlay2_VKVIDEO)
        list(APPEND FFmpeg_HDR FFDecVkVideo.hpp VkVideoVulkan.hpp)
        list(APPEND FFmpeg_SRC FFDecVkVideo.cpp VkVideoVulkan.cpp)
    endif()

    if(USE_FFMPEG_VAAPI) # VA-API
        add_definitions(-DQMPlay2_VAAPI)
        list(APPEND FFmpeg_HDR FFDecVAAPI.hpp VAAPI.hpp)
        list(APPEND FFmpeg_SRC FFDecVAAPI.cpp VAAPI.cpp)
        if(USE_OPENGL)
            list(APPEND FFmpeg_HDR VAAPIOpenGL.hpp)
            list(APPEND FFmpeg_SRC VAAPIOpenGL.cpp)
        endif()
        if(USE_VULKAN)
            list(APPEND FFmpeg_HDR VAAPIVulkan.hpp)
            list(APPEND FFmpeg_SRC VAAPIVulkan.cpp)
        endif()
        if(USE_OPENGL)
            pkg_check_modules(LBVA REQUIRED libva libva-drm egl)
        else()
            pkg_check_modules(LBVA REQUIRED libva libva-drm)
        endif()
        include_directories(${LBVA_INCLUDE_DIRS})
        link_directories(${LBVA_LIBRARY_DIRS})
        list(APPEND FFmpeg_LIBS ${LBVA_LIBRARIES})
    endif()

    if (FIND_HWACCEL_DRIVERS_PATH AND USE_FFMPEG_VAAPI)
        add_definitions(-DFIND_HWACCEL_DRIVERS_PATH)
    endif()

    if(USE_FFMPEG_DXVA2) # DXVA2
        add_definitions(-DQMPlay2_DXVA2 -DINITGUID)
        list(APPEND FFmpeg_HDR FFDecDXVA2.hpp)
        list(APPEND FFmpeg_SRC FFDecDXVA2.cpp)
        if(USE_OPENGL)
            list(APPEND FFmpeg_HDR DXVA2OpenGL.hpp)
            list(APPEND FFmpeg_SRC DXVA2OpenGL.cpp)
        endif()
    endif()

    if(USE_FFMPEG_D3D11VA) #D3D11VA
        add_definitions(-DQMPlay2_D3D11VA)
        list(APPEND FFmpeg_HDR FFDecD3D11VA.hpp D3D11VAVulkan.hpp)
        list(APPEND FFmpeg_SRC FFDecD3D11VA.cpp D3D11VAVulkan.cpp)
        list(APPEND FFmpeg_RESOURCES D3D11VA.qrc)
    endif()

    if(USE_FFMPEG_VTB)
        add_definitions(-DQMPlay2_VTB)
        list(APPEND FFmpeg_HDR FFDecVTB.hpp)
        list(APPEND FFmpeg_SRC FFDecVTB.cpp)
        if(USE_OPENGL)
            list(APPEND FFmpeg_HDR VTBOpenGL.hpp)
            list(APPEND FFmpeg_SRC VTBOpenGL.cpp)
        endif()
        find_library(OPENGL_LIBRARY OpenGL)
        find_library(COREVIDEO_LIBRARY CoreVideo)
        find_library(IOSURFACE_LIBRARY IOSurface)
        list(APPEND FFmpeg_LIBS ${OPENGL_LIBRARY} ${COREVIDEO_LIBRARY} ${IOSURFACE_LIBRARY})
    endif()
endif()

add_definitions(-DFFMPEG_MODULE)

add_library(${PROJECT_NAME} MODULE
    ${FFmpeg_HDR}
    ${FFmpeg_SRC}
    ${FFmpeg_RESOURCES}
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME}
        PRIVATE
        ${FFmpeg_HDR}
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${FFmpeg_LIBS}
)
libqmplay2_set_target_params()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
