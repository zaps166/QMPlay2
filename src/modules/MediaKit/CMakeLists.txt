cmake_minimum_required(VERSION 3.1)
project(MediaKit)

set(MediaKit_HDR
    MediaKit.hpp
    MediaKitWriter.hpp
    RingBuffer.hpp
    SndPlayer.hpp
)

set(MediaKit_SRC
    MediaKit.cpp
    MediaKitWriter.cpp
    RingBuffer.cpp
    SndPlayer.cpp
)

set(MediaKit_RESOURCES
    icon.qrc
)

include_directories(../../qmplay2/headers)

add_library(${PROJECT_NAME} MODULE
    ${MediaKit_HDR}
    ${MediaKit_SRC}
    ${MediaKit_RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    be
    media
    libqmplay2
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
