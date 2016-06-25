cmake_minimum_required(VERSION 2.8.11)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif()
endif()
project(Chiptune)

set(Chiptune_HDR
    Chiptune.hpp
    Common.hpp
)

set(Chiptune_SRC
    Chiptune.cpp
    Common.cpp
)

set(Chiptune_RESOURCES
    icon.qrc
)

set(Chiptune_LIBS
)

if(USE_CHIPTUNE_GME)
    find_package(GME REQUIRED)
    list(APPEND Chiptune_HDR GME.hpp)
    list(APPEND Chiptune_SRC GME.cpp)
    add_definitions(-DUSE_GME)
    include_directories(${LIBGME_INCLUDE_DIRS})
    link_directories(${LIBGME_LIBRARY_DIRS})
    list(APPEND Chiptune_LIBS ${LIBGME_LIBRARIES})
endif()

if(USE_CHIPTUNE_SID)
    pkg_check_modules(LIBSIDPLAYFP libsidplayfp REQUIRED)
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
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    $<TARGET_FILE:qmplay2>
)

if(WIN32)
    set(STATIC_CHIPTUNE_LIBRARIES "" CACHE STRING "Static libs for Chiptune")
    mark_as_advanced(STATIC_CHIPTUNE_LIBRARIES)
endif()

if(WIN32 AND STATIC_CHIPTUNE_LIBRARIES)
    separate_arguments(STATIC_CHIPTUNE_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} ${STATIC_CHIPTUNE_LIBRARIES})
else()
    target_link_libraries(${PROJECT_NAME} ${Chiptune_LIBS})
endif()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
