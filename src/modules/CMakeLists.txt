cmake_minimum_required(VERSION 3.0.2)

include(GNUInstallDirs)
set(MODULES_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/qmplay2/modules/")

if(USE_PORTAUDIO)
    add_subdirectory(PortAudio)
endif()

if(USE_ALSA)
    add_subdirectory(ALSA)
endif()

if(USE_PULSEAUDIO)
    add_subdirectory(PulseAudio)
endif()

if(USE_XVIDEO)
    add_subdirectory(XVideo)
endif()

if(USE_AUDIOCD)
    add_subdirectory(AudioCD)
endif()

add_subdirectory(AudioFilters)
add_subdirectory(Extensions)
add_subdirectory(FFmpeg)
add_subdirectory(Inputs)
add_subdirectory(Modplug)
add_subdirectory(Playlists)
add_subdirectory(QPainter)
add_subdirectory(Subtitles)
add_subdirectory(VideoFilters)
add_subdirectory(Visualizations)

if(USE_OPENGL)
    add_subdirectory(OpenGL2)
endif()

if(USE_CHIPTUNE_GME OR USE_CHIPTUNE_SID)
    add_subdirectory(Chiptune)
endif()
