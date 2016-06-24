cmake_minimum_required(VERSION 2.8.11)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif()
endif()
project(Extensions)

set(Extensions_HDR
    Extensions.hpp
    YouTube.hpp
    Downloader.hpp
    Radio.hpp
    LastFM.hpp
)

set(Extensions_SRC
    Extensions.cpp
    YouTube.cpp
    Downloader.cpp
    Radio.cpp
    LastFM.cpp
)

set(Extensions_RESOURCES
    icons.qrc
)

if(NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND Extensions_HDR MPRIS2.hpp)
    list(APPEND Extensions_SRC MPRIS2.cpp)

    add_definitions(-DUSE_MPRIS2)

    if(USE_QT5)
        set(DBUS DBus)
    else()
        set(DBUS Qt4::QtDBus)
    endif()
endif()

if(USE_PROSTOPLEER)
    list(APPEND Extensions_HDR ProstoPleer.hpp)
    list(APPEND Extensions_SRC ProstoPleer.cpp)
    add_definitions(-DUSE_PROSTOPLEER)
endif()

if(USE_QT5)
    qt5_add_resources(Extensions_RESOURCES_RCC ${Extensions_RESOURCES})
else()
    qt4_add_resources(Extensions_RESOURCES_RCC ${Extensions_RESOURCES})
endif()

include_directories(../../qmplay2/headers)

add_library(${PROJECT_NAME} MODULE
    ${Extensions_HDR}
    ${Extensions_SRC}
    ${Extensions_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets Network ${DBUS})
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui Qt4::QtNetwork ${DBUS})
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    $<TARGET_FILE:qmplay2>
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
