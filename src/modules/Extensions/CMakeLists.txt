cmake_minimum_required(VERSION 2.8.6)
if(COMMAND cmake_policy)
    if(POLICY CMP0003)
        cmake_policy(SET CMP0003 NEW)
    endif()
    if(POLICY CMP0020)
        cmake_policy(SET CMP0020 NEW)
    endif()
    if(POLICY CMP0042)
        cmake_policy(SET CMP0042 NEW)
    endif()
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
)

set(Extensions_SRC
    Extensions.cpp
    YouTube.cpp
    Downloader.cpp
    Radio.cpp
)

set(Extensions_RESOURCES
    icons.qrc
)

if(USE_MPRIS2)
    list(APPEND Extensions_HDR MPRIS2.hpp)
    list(APPEND Extensions_SRC MPRIS2.cpp)

    add_definitions(-DUSE_MPRIS2)

    if(USE_QT5)
        set(DBUS DBus)
        find_package(Qt5DBus REQUIRED)
    else()
        set(DBUS Qt4::QtDBus)
        find_package(Qt4 REQUIRED QtDBus)
    endif()
endif()

if(USE_LASTFM)
    list(APPEND Extensions_HDR LastFM.hpp)
    list(APPEND Extensions_SRC LastFM.cpp)
    add_definitions(-DUSE_LASTFM)
endif()

if(USE_PROSTOPLEER)
    list(APPEND Extensions_HDR MediaBrowser/ProstoPleer.hpp)
    list(APPEND Extensions_SRC MediaBrowser/ProstoPleer.cpp)
    add_definitions(-DUSE_PROSTOPLEER)
endif()
if(USE_SOUNDCLOUD)
    list(APPEND Extensions_HDR MediaBrowser/SoundCloud.hpp)
    list(APPEND Extensions_SRC MediaBrowser/SoundCloud.cpp)
    add_definitions(-DUSE_SOUNDCLOUD)
endif()
if(USE_PROSTOPLEER OR USE_SOUNDCLOUD)
    list(APPEND Extensions_HDR MediaBrowser.hpp MediaBrowser/Common.hpp)
    list(APPEND Extensions_SRC MediaBrowser.cpp MediaBrowser/Common.cpp)
    add_definitions(-DUSE_MEDIABROWSER)
endif()

if(USE_QT5)
    qt5_add_resources(Extensions_RESOURCES_RCC ${Extensions_RESOURCES})
else()
    qt4_add_resources(Extensions_RESOURCES_RCC ${Extensions_RESOURCES})
endif()

include_directories(../../qmplay2/headers)

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${Extensions_HDR}
    ${Extensions_SRC}
    ${Extensions_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets ${DBUS})
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui ${DBUS})
endif()

add_dependencies(${PROJECT_NAME} libqmplay2)
target_link_libraries(${PROJECT_NAME}
    ${qmplay2lib}
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
