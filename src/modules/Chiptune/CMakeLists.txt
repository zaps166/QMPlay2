project(Chiptune)
cmake_minimum_required(VERSION 3.0.2)
FIND_PACKAGE(PkgConfig REQUIRED)

set(Chiptune_HDR
    Chiptune.hpp
)

set(Chiptune_SRC
    Chiptune.cpp
)

set(Chiptune_RESOURCES
    icon.qrc
)

set(Chiptune_LIBS
)

if(USE_CHIPTUNE_GME)
    PKG_CHECK_MODULES(LIBGME libgme REQUIRED)
    list(APPEND Chiptune_HDR GME.hpp)
    list(APPEND Chiptune_SRC GME.cpp)
    add_definitions(-DUSE_GME)
    include_directories(${LIBGME_INCLUDE_DIRS})
    link_directories(${LIBGME_LIBRARY_DIRS})
    list(APPEND Chiptune_LIBS ${LIBGME_LIBRARIES})
endif()

if(USE_CHIPTUNE_SID)
    PKG_CHECK_MODULES(LIBSIDPLAYFP libsidplayfp REQUIRED)
    list(APPEND Chiptune_HDR SIDPlay.hpp)
    list(APPEND Chiptune_SRC SIDPlay.cpp)
    add_definitions(-DUSE_SIDPLAY)
    include_directories(${LIBSIDPLAYFP_INCLUDE_DIRS})
    link_directories(${LIBSIDPLAYFP_LIBRARY_DIRS})
    list(APPEND Chiptune_LIBS ${LIBSIDPLAYFP_LIBRARIES})
endif()

if(USE_QT5)
    qt5_add_resources(Chiptune_RESOURCES_RCC ${Chiptune_RESOURCES})
else()
    qt4_add_resources(Chiptune_RESOURCES_RCC ${Chiptune_RESOURCES})
endif()

include_directories(../../qmplay2/headers)

add_library(${PROJECT_NAME} MODULE
    ${Chiptune_HDR}
    ${Chiptune_SRC}
    ${Chiptune_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtGui Qt4::QtCore)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    ${Chiptune_LIBS}
    $<TARGET_FILE:qmplay2>
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
