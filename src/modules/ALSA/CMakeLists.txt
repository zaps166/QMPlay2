cmake_minimum_required(VERSION 3.16)
project(ALSA)

set(ALSA_HDR
    ALSAHeaders.hpp
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

qmplay2_add_precompile_headers(ALSAHeaders.hpp ${ALSA_HDR})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${ALSA_LIBRARY}
)
target_link_libraries(${PROJECT_NAME} PRIVATE libqmplay2_iface)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
