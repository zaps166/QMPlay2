cmake_minimum_required(VERSION 3.16)
project(PulseAudio)

set(PulseAudio_HDR
    PulseAudioHeaders.hpp
    PulseAudio.hpp
    PulseAudioWriter.hpp
    Pulse.hpp
)

set(PulseAudio_SRC
    PulseAudio.cpp
    PulseAudioWriter.cpp
    Pulse.cpp
)

set(PulseAudio_RESOURCES
    icon.qrc
)

pkg_check_modules(LIBPULSE REQUIRED libpulse-simple)
link_directories(${LIBPULSE_LIBRARY_DIRS})

include_directories(${LIBPULSE_INCLUDE_DIRS})

add_library(${PROJECT_NAME} MODULE
    ${PulseAudio_HDR}
    ${PulseAudio_SRC}
    ${PulseAudio_RESOURCES}
)

qmplay2_add_precompile_headers(PulseAudioHeaders.hpp ${PulseAudio_HDR})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${LIBPULSE_LIBRARIES}
)
target_link_libraries(${PROJECT_NAME} PRIVATE libqmplay2_iface)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
