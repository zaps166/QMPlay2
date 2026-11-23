cmake_minimum_required(VERSION 3.16)
project(Playlists)

set(Playlists_HDR
    PlaylistsHeaders.hpp
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

qmplay2_add_precompile_headers(PlaylistsHeaders.hpp ${Playlists_HDR})

target_link_libraries(${PROJECT_NAME} PRIVATE libqmplay2_iface)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
