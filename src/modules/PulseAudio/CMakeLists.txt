cmake_minimum_required(VERSION 3.12)
project(PulseAudio)

set(PulseAudio_HDR
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

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        ${PulseAudio_HDR}
    )
endif()

target_link_libraries(${PROJECT_NAME}
    ${LIBPULSE_LIBRARIES}
    libqmplay2
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
