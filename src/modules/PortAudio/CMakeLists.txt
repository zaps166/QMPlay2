cmake_minimum_required(VERSION 3.1)
project(PortAudio)

set(PortAudio_HDR
    PortAudio.hpp
    PortAudioCommon.hpp
    PortAudioWriter.hpp
)

set(PortAudio_SRC
    PortAudio.cpp
    PortAudioCommon.cpp
    PortAudioWriter.cpp
)

set(PortAudio_RESOURCES
    icon.qrc
)

pkg_check_modules(LIBPORTAUDIO portaudio-2.0 REQUIRED)
link_directories(${LIBPORTAUDIO_LIBRARY_DIRS})

include_directories(../../qmplay2/headers ${LIBPORTAUDIO_INCLUDE_DIRS})

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${PortAudio_HDR}
    ${PortAudio_SRC}
    ${PortAudio_RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    libqmplay2
)

if(WIN32)
    set(CUSTOM_PORTAUDIO_LIBRARIES "" CACHE STRING "Custom libraries for PortAudio")
    mark_as_advanced(CUSTOM_PORTAUDIO_LIBRARIES)
endif()

if(WIN32 AND CUSTOM_PORTAUDIO_LIBRARIES)
    separate_arguments(CUSTOM_PORTAUDIO_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} ${CUSTOM_PORTAUDIO_LIBRARIES})
else()
    target_link_libraries(${PROJECT_NAME} ${LIBPORTAUDIO_LIBRARIES})
endif()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
