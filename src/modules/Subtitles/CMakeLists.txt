cmake_minimum_required(VERSION 3.13)
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

add_library(${PROJECT_NAME} MODULE
    ${Subtitles_HDR}
    ${Subtitles_SRC}
    ${Subtitles_RESOURCES}
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        ${Subtitles_HDR}
    )
endif()

target_link_libraries(${PROJECT_NAME}
    libqmplay2
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
