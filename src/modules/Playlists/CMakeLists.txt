cmake_minimum_required(VERSION 3.16)
project(Playlists)

set(Playlists_HDR
    Playlists.hpp
    PLS.hpp
    M3U.hpp
    XSPF.hpp
)

set(Playlists_SRC
    Playlists.cpp
    PLS.cpp
    M3U.cpp
    XSPF.cpp
)

set(Playlists_RESOURCES
    icon.qrc
)

add_library(${PROJECT_NAME} MODULE
    ${Playlists_HDR}
    ${Playlists_SRC}
    ${Playlists_RESOURCES}
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        ${Playlists_HDR}
    )
endif()

libqmplay2_set_target_params()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
