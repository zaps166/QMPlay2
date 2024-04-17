cmake_minimum_required(VERSION 3.13)
project(ALSA)

set(ALSA_HDR
    ALSA.hpp
    ALSACommon.hpp
    ALSAWriter.hpp
)

set(ALSA_SRC
    ALSA.cpp
    ALSACommon.cpp
    ALSAWriter.cpp
)

set(ALSA_RESOURCES
    icon.qrc
)

find_package(ALSA REQUIRED)

include_directories(${ALSA_INCLUDE_DIR})

add_library(${PROJECT_NAME} MODULE
    ${ALSA_HDR}
    ${ALSA_SRC}
    ${ALSA_RESOURCES}
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        ${ALSA_HDR}
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${ALSA_LIBRARY}
)
libqmplay2_set_target_params()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
