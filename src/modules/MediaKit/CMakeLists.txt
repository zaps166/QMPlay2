cmake_minimum_required(VERSION 3.16)
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

add_library(${PROJECT_NAME} MODULE
    ${MediaKit_HDR}
    ${MediaKit_SRC}
    ${MediaKit_RESOURCES}
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        ${MediaKit_HDR}
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    be
    media
)
libqmplay2_set_target_params()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
