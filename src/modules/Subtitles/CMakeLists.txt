cmake_minimum_required(VERSION 3.1)
project(Subtitles)

set(Subtitles_HDR
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

include_directories(../../qmplay2/headers)

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${Subtitles_HDR}
    ${Subtitles_SRC}
    ${Subtitles_RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    libqmplay2
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
