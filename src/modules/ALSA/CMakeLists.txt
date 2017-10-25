cmake_minimum_required(VERSION 3.1)
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

include_directories(../../qmplay2/headers ${ALSA_INCLUDE_DIR})

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${ALSA_HDR}
    ${ALSA_SRC}
    ${ALSA_RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    ${ALSA_LIBRARY}
    libqmplay2
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
