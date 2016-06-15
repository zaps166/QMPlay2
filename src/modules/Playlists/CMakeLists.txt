project(Playlists)
cmake_minimum_required(VERSION 3.0.2)

set(Playlists_HDR
    Playlists.hpp
    PLS.hpp
    M3U.hpp
)

set(Playlists_SRC
    Playlists.cpp
    PLS.cpp
    M3U.cpp
)

include_directories(../../qmplay2/headers)

add_library(${PROJECT_NAME} MODULE
    ${Playlists_HDR}
    ${Playlists_SRC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtGui Qt4::QtCore)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    $<TARGET_FILE:qmplay2>
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
