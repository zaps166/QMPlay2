cmake_minimum_required(VERSION 3.16)
project(Subtitles)

set(Subtitles_HDR
    SubtitlesHeaders.hpp
    SRT.hpp
    Classic.hpp
    Subtitles.hpp
)

set(Subtitles_SRC
    SRT.cpp
    Classic.cpp
    Subtitles.cpp
)

set(Subtitles_RESOURCES
    icon.qrc
)

add_library(${PROJECT_NAME} MODULE
    ${Subtitles_HDR}
    ${Subtitles_SRC}
    ${Subtitles_RESOURCES}
)

qmplay2_add_precompile_headers(SubtitlesHeaders.hpp ${Subtitles_HDR})

target_link_libraries(${PROJECT_NAME} PRIVATE libqmplay2_iface)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
