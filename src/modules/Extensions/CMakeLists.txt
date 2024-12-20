cmake_minimum_required(VERSION 3.16)
project(Extensions)

set(Extensions_HDR
    Extensions.hpp
    Downloader.hpp
)

set(Extensions_SRC
    Extensions.cpp
    Downloader.cpp
)

set(Extensions_RESOURCES
    icons.qrc
)

if(USE_MPRIS2)
    list(APPEND Extensions_HDR MPRIS2.hpp)
    list(APPEND Extensions_SRC MPRIS2.cpp)
    add_definitions(-DUSE_MPRIS2)
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
    list(APPEND Extensions_RESOURCES js.qrc)
    add_definitions(-DUSE_MEDIABROWSER)
    set(QML ${QT_PREFIX}::Qml)
endif()

if(USE_RADIO)
    list(APPEND Extensions_HDR
        Radio.hpp
        Radio/RadioBrowserModel.hpp
    )
    list(APPEND Extensions_SRC
        Radio.cpp
        Radio/RadioBrowserModel.cpp
    )
    set(Extensions_FORMS
        Radio/Radio.ui
    )
    add_definitions(-DUSE_RADIO)
endif()

if(USE_YOUTUBE)
    list(APPEND Extensions_HDR YouTube.hpp)
    list(APPEND Extensions_SRC YouTube.cpp)
    add_definitions(-DUSE_YOUTUBE)
endif()

if(USE_OPENSUBTITLES)
    list(APPEND Extensions_HDR OpenSubtitles.hpp)
    list(APPEND Extensions_SRC OpenSubtitles.cpp)
    add_definitions(-DUSE_OPENSUBTITLES)
endif()

if(BUILD_WITH_QT6)
    qt6_wrap_ui(Extensions_FORM_HDR ${Extensions_FORMS})
else()
    qt5_wrap_ui(Extensions_FORM_HDR ${Extensions_FORMS})
endif()
set_property(SOURCE ${Extensions_FORM_HDR} PROPERTY SKIP_AUTOMOC ON)

add_library(${PROJECT_NAME} MODULE
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
    PRIVATE
    ${QML}
)
libqmplay2_set_target_params()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
