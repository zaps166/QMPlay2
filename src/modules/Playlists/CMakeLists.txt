cmake_minimum_required(VERSION 3.1)
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

include_directories(../../qmplay2/headers)

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${Playlists_HDR}
    ${Playlists_SRC}
    ${Playlists_RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    libqmplay2
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
