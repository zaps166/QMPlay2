cmake_minimum_required(VERSION 3.16)
project(MediaKit)

set(MediaKit_HDR
    MediaKitHeaders.hpp
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

add_library(${PROJECT_NAME} MODULE
    ${MediaKit_HDR}
    ${MediaKit_SRC}
    ${MediaKit_RESOURCES}
)

qmplay2_add_precompile_headers(MediaKitHeaders.hpp ${MediaKit_HDR})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    be
    media
)
target_link_libraries(${PROJECT_NAME} PRIVATE libqmplay2_iface)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
