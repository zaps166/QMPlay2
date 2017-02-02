cmake_minimum_required(VERSION 2.8.6)

if(WIN32)
    set(MODULES_INSTALL_PATH "modules/")
    set(CMAKE_SHARED_MODULE_PREFIX "") # remove lib prefix
    set(QMPLAY2_MODULE MODULE)
elseif(APPLE)
    set(MODULES_INSTALL_PATH "${CMAKE_INSTALL_LIBDIR}/modules/")
    set(QMPLAY2_MODULE SHARED) # otherwise CMake uses ".so" extension
else()
    set(MODULES_INSTALL_PATH "${CMAKE_INSTALL_LIBDIR}/qmplay2/modules/")
    set(QMPLAY2_MODULE MODULE)
endif()

if(USE_FFMPEG)
    add_subdirectory(FFmpeg)
endif()

if(USE_INPUTS)
    add_subdirectory(Inputs)
endif()

if(USE_MODPLUG)
    add_subdirectory(Modplug)
endif()

if(USE_EXTENSIONS)
    add_subdirectory(Extensions)
endif()

if(USE_VISUALIZATIONS)
    add_subdirectory(Visualizations)
endif()

if(USE_AUDIOFILTERS)
    add_subdirectory(AudioFilters)
endif()

if(USE_VIDEOFILTERS)
    add_subdirectory(VideoFilters)
endif()

if(USE_OPENGL2)
    add_subdirectory(OpenGL2)
endif()

if(USE_AUDIOCD)
    add_subdirectory(AudioCD)
endif()

if(USE_ALSA)
    add_subdirectory(ALSA)
endif()

if(USE_PORTAUDIO)
    add_subdirectory(PortAudio)
endif()

if(USE_PULSEAUDIO)
    add_subdirectory(PulseAudio)
endif()

if(USE_XVIDEO)
    add_subdirectory(XVideo)
endif()

if(USE_CHIPTUNE_GME OR USE_CHIPTUNE_SID)
    add_subdirectory(Chiptune)
endif()

add_subdirectory(Playlists)
add_subdirectory(Subtitles)
add_subdirectory(QPainter)

if(WIN32)
    add_subdirectory(DirectX)
    add_subdirectory(FileAssociation)
endif()

if(USE_CUVID)
    add_subdirectory(CUVID)
endif()

if(USE_NOTIFIES)
    add_subdirectory(Notifies)
endif()
