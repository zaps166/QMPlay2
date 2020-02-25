cmake_minimum_required(VERSION 3.1)
project(Extensions)

set(Extensions_HDR
    Extensions.hpp
    YouTube.hpp
    Downloader.hpp
    Radio.hpp
    Radio/RadioBrowserModel.hpp
)

set(Extensions_SRC
    Extensions.cpp
    YouTube.cpp
    Downloader.cpp
    Radio.cpp
    Radio/RadioBrowserModel.cpp
)

set(Extensions_FORMS
    Radio/Radio.ui
)

set(Extensions_RESOURCES
    icons.qrc
    js.qrc
)

if(USE_MPRIS2)
    list(APPEND Extensions_HDR MPRIS2.hpp)
    list(APPEND Extensions_SRC MPRIS2.cpp)
    add_definitions(-DUSE_MPRIS2)
    set(DBUS Qt5::DBus)
endif()

if(USE_LASTFM)
    list(APPEND Extensions_HDR LastFM.hpp)
    list(APPEND Extensions_SRC LastFM.cpp)
    add_definitions(-DUSE_LASTFM)
endif()

if(USE_LYRICS)
    list(APPEND Extensions_HDR Lyrics.hpp)
    list(APPEND Extensions_SRC Lyrics.cpp)
    add_definitions(-DUSE_LYRICS)
endif()

if(USE_MEDIABROWSER)
    list(APPEND Extensions_HDR MediaBrowser.hpp MediaBrowserJS.hpp)
    list(APPEND Extensions_SRC MediaBrowser.cpp MediaBrowserJS.cpp)
    add_definitions(-DUSE_MEDIABROWSER)
    if(JS_SKIP_SSE2_CHECK)
        add_definitions(-DJS_SKIP_SSE2_CHECK)
    endif()
    set(QML Qt5::Qml)
endif()

qt5_wrap_ui(Extensions_FORM_HDR ${Extensions_FORMS})
set_property(SOURCE ${Extensions_FORM_HDR} PROPERTY SKIP_AUTOMOC ON)

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${Extensions_HDR}
    ${Extensions_SRC}
    ${Extensions_FORM_HDR}
    ${Extensions_RESOURCES}
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        ${Extensions_HDR}
    )
endif()

target_link_libraries(${PROJECT_NAME}
    ${DBUS}
    ${QML}
    libqmplay2
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
