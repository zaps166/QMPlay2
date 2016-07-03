cmake_minimum_required(VERSION 2.8.6)
if(COMMAND cmake_policy)
    if(POLICY CMP0003)
        cmake_policy(SET CMP0003 NEW)
    endif()
    if(POLICY CMP0020)
        cmake_policy(SET CMP0020 NEW)
    endif()
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif()
endif()
project(gui)

set(GUI_HDR
    Main.hpp
    MenuBar.hpp
    MainWidget.hpp
    AddressBox.hpp
    VideoDock.hpp
    InfoDock.hpp
    PlaylistDock.hpp
    PlayClass.hpp
    DemuxerThr.hpp
    AVThread.hpp
    VideoThr.hpp
    AudioThr.hpp
    SettingsWidget.hpp
    OSDSettingsW.hpp
    DeintSettingsW.hpp
    OtherVFiltersW.hpp
    PlaylistWidget.hpp
    EntryProperties.hpp
    AboutWidget.hpp
    AddressDialog.hpp
    VideoAdjustment.hpp
    Appearance.hpp
    VolWidget.hpp
)

set(GUI_SRC
    Main.cpp
    MenuBar.cpp
    MainWidget.cpp
    AddressBox.cpp
    VideoDock.cpp
    InfoDock.cpp
    PlaylistDock.cpp
    PlayClass.cpp
    DemuxerThr.cpp
    AVThread.cpp
    VideoThr.cpp
    AudioThr.cpp
    SettingsWidget.cpp
    OSDSettingsW.cpp
    DeintSettingsW.cpp
    OtherVFiltersW.cpp
    PlaylistWidget.cpp
    EntryProperties.cpp
    AboutWidget.cpp
    AddressDialog.cpp
    VideoAdjustment.cpp
    Appearance.cpp
    VolWidget.cpp
)

set(GUI_FORMS # *.ui files
    Ui/SettingsGeneral.ui
    Ui/SettingsPlayback.ui
    Ui/SettingsPlaybackModuleslist.ui
    Ui/OSDSettings.ui
)

set(GUI_RESOURCES
    resources.qrc
)

if(USE_QT5)
    qt5_wrap_ui(GUI_FORM_HDR ${GUI_FORMS})
    qt5_add_resources(GUI_RESOURCES_RCC ${GUI_RESOURCES})
else()
    find_package(Qt4 REQUIRED QtNetwork)
    include("${QT_USE_FILE}")
    qt4_wrap_ui(GUI_FORM_HDR ${GUI_FORMS})
    qt4_add_resources(GUI_RESOURCES_RCC ${GUI_RESOURCES})
endif()

include_directories(../qmplay2/headers)

if(USE_TAGLIB)
    find_package(TagLib REQUIRED)
    include_directories(${TAGLIB_INCLUDE_DIRS})
    link_directories(${TAGLIB_LIBRARY_DIRS})

    add_definitions(-DQMPlay2_TagEditor)
    if(WIN32)
        add_definitions(-DTAGLIB_FULL_INCLUDE_PATH)

        option(TAGLIB_STATIC_LINK "Use TagLib static library" OFF)
        mark_as_advanced(TAGLIB_STATIC_LINK)
        if(TAGLIB_STATIC_LINK)
            add_definitions(-DTAGLIB_STATIC)
        endif()
    endif()

    list(APPEND GUI_HDR TagEditor.hpp)
    list(APPEND GUI_SRC TagEditor.cpp)
endif()

if(WIN32)
    # set RC file
    list(APPEND GUI_SRC Windows/icons.rc)

    # updater
    add_definitions(-DUPDATER)
    list(APPEND GUI_HDR Updater.hpp)
    list(APPEND GUI_SRC Updater.cpp)
endif()

# RPATH
if(SET_INSTALL_RPATH)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
    else()
        set(CMAKE_INSTALL_RPATH "@executable_path/../${CMAKE_INSTALL_LIBDIR}")
    endif()
endif()

if(USE_JEMALLOC)
    pkg_check_modules(JEMALLOC jemalloc REQUIRED)
    link_directories(${JEMALLOC_LIBRARY_DIRS})
endif()

if(WIN32 AND NOT USE_CMD)
    set(WIN32_EXE WIN32) # don't show CMD when running on windows
endif()

add_executable(QMPlay2 ${WIN32_EXE}
    ${GUI_HDR}
    ${GUI_SRC}
    ${GUI_FORM_HDR}
    ${GUI_RESOURCES_RCC}
)

if(USE_JEMALLOC)
    target_link_libraries(QMPlay2 ${JEMALLOC_LIBRARIES})
endif()

if(USE_QT5)
    qt5_use_modules(QMPlay2 Gui Widgets Network)
else()
    target_link_libraries(QMPlay2 Qt4::QtCore Qt4::QtGui Qt4::QtNetwork)
endif()

add_dependencies(QMPlay2 qmplay2)
target_link_libraries(QMPlay2 ${libqmplay2})

if(WIN32)
    set(CUSTOM_GUI_LIBRARIES "" CACHE STRING "Custom libraries for GUI")
    mark_as_advanced(CUSTOM_GUI_LIBRARIES)
endif()
if(WIN32 AND CUSTOM_GUI_LIBRARIES)
    separate_arguments(CUSTOM_GUI_LIBRARIES)
    target_link_libraries(QMPlay2 ${CUSTOM_GUI_LIBRARIES})
elseif(USE_TAGLIB)
    target_link_libraries(QMPlay2 ${TAGLIB_LIBRARIES})
endif()

if(WIN32)
    install(TARGETS QMPlay2 RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/)
else()
    # executable
    install(TARGETS QMPlay2 RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        # desktop files
        file(GLOB DESKTOP_FILES Unix/QMPlay2*.desktop)
        install(FILES ${DESKTOP_FILES} DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications/")

        # mime files
        file(GLOB MIME_FILES Unix/x-*.xml)
        install(FILES ${MIME_FILES} DESTINATION "${INSTALL_PATH_MIME}/packages/")

        # icon
        install(DIRECTORY Icons/hicolor DESTINATION "${INSTALL_PATH_ICONS}")

        # install Solid actions
        include(InstallSolidActions)
    endif()

    # man page
    find_program(GZIP gzip)
    if(GZIP)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Unix/QMPlay2.1.in ${CMAKE_CURRENT_BINARY_DIR}/QMPlay2.1 @ONLY)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/QMPlay2.1.gz
            COMMAND ${GZIP} -c ${CMAKE_CURRENT_BINARY_DIR}/QMPlay2.1 > QMPlay2.1.gz
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/QMPlay2.1)
        add_custom_target(man ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/QMPlay2.1.gz)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/QMPlay2.1.gz
                DESTINATION ${INSTALL_PATH_MAN}/man1/)
    endif()
endif()
