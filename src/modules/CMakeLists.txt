cmake_minimum_required(VERSION 2.8.11)

if(WIN32)
    set(MODULES_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/modules/")
else()
    set(MODULES_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/qmplay2/modules/")
endif()

add_subdirectory(FFmpeg)
add_subdirectory(Inputs)
add_subdirectory(Modplug)
add_subdirectory(Playlists)
add_subdirectory(Subtitles)
add_subdirectory(QPainter)
add_subdirectory(Extensions)
add_subdirectory(Visualizations)
add_subdirectory(AudioFilters)
add_subdirectory(VideoFilters)

if(USE_OPENGL)
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

if(WIN32)
    add_subdirectory(DirectX)
    add_subdirectory(FileAssociation)
endif()
