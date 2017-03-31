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
project(QMPlay2)

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
    ScreenSaver.hpp
    ShortcutHandler.hpp
    KeyBindingsDialog.hpp
    Updater.hpp
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
    ShortcutHandler.cpp
    KeyBindingsDialog.cpp
    Updater.cpp
)

set(GUI_FORMS # *.ui files
    Ui/SettingsGeneral.ui
    Ui/SettingsPlayback.ui
    Ui/SettingsPlaybackModulesList.ui
    Ui/OSDSettings.ui
)

set(GUI_RESOURCES
    resources.qrc
)

if(APPLE)
    list(APPEND GUI_HDR
        macOS/QMPlay2MacExtensions.hpp
    )
    list(APPEND GUI_SRC
        macOS/QMPlay2MacExtensions.mm
    )

    include_directories("macOS")
endif()

if(USE_QT5)
    qt5_wrap_ui(GUI_FORM_HDR ${GUI_FORMS})
    qt5_add_resources(GUI_RESOURCES_RCC ${GUI_RESOURCES})
else()
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

if(NOT APPLE)
    add_definitions(-DQMPLAY2_ALLOW_ONLY_ONE_INSTANCE)
endif()

if(WIN32)
    # set RC file
    list(APPEND GUI_SRC Windows/icons.rc)

    # updater
    add_definitions(-DUPDATER)

    # screen saver
    list(APPEND GUI_SRC Windows/ScreenSaver.cpp)
elseif(APPLE)
    set(MACOSX_BUNDLE_BUNDLE_VERSION "1")
    set(MACOSX_BUNDLE_ICON_FILE "QMPlay2.icns")
    set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${QMPLAY2_VERSION})
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright (C) 2010-2017  Błażej Szczygieł")

    set(ICNS_FILE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/macOS/QMPlay2.icns")
    set_source_files_properties(${ICNS_FILE_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    set(ICNS_FILE_FILE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/macOS/QMPlay2File.icns")
    set_source_files_properties(${ICNS_FILE_FILE_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    set(ICNS_PLAYLIST_FILE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/macOS/QMPlay2List.icns")
    set_source_files_properties(${ICNS_PLAYLIST_FILE_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    # screen saver
    list(APPEND GUI_SRC macOS/ScreenSaver.cpp)
else()
    # screen saver
    list(APPEND GUI_SRC Unix/ScreenSaver.cpp)
endif()

# RPATH
if(SET_INSTALL_RPATH)
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
endif()

if(USE_JEMALLOC)
    pkg_check_modules(JEMALLOC jemalloc REQUIRED)
    link_directories(${JEMALLOC_LIBRARY_DIRS})
endif()

if(WIN32 AND NOT USE_CMD)
    set(WIN32_EXE WIN32) # don't show CMD when running on windows
endif()

add_executable(${PROJECT_NAME}
    ${WIN32_EXE}
    MACOSX_BUNDLE ${ICNS_FILE_PATH} ${ICNS_FILE_FILE_PATH} ${ICNS_PLAYLIST_FILE_PATH}
    ${GUI_HDR}
    ${GUI_SRC}
    ${GUI_FORM_HDR}
    ${GUI_RESOURCES_RCC}
)

if(USE_JEMALLOC)
    target_link_libraries(${PROJECT_NAME} ${JEMALLOC_LIBRARIES})
endif()

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui)
endif()

add_dependencies(${PROJECT_NAME} libqmplay2)
target_link_libraries(${PROJECT_NAME} ${qmplay2lib})

if(WIN32)
    set(CUSTOM_GUI_LIBRARIES "" CACHE STRING "Custom libraries for GUI")
    mark_as_advanced(CUSTOM_GUI_LIBRARIES)
elseif(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macOS/BundleInfo.plist.in")
    target_link_libraries(${PROJECT_NAME} ${APPKIT_LIBRARY} ${IOKIT_LIBRARY})
endif()
if(WIN32 AND CUSTOM_GUI_LIBRARIES)
    separate_arguments(CUSTOM_GUI_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} ${CUSTOM_GUI_LIBRARIES})
elseif(USE_TAGLIB)
    target_link_libraries(${PROJECT_NAME} ${TAGLIB_LIBRARIES})
endif()

if(WIN32)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/)
elseif(APPLE)
    install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION ${CMAKE_INSTALL_PREFIX})

    set(QT_LIBS_DIR "${Qt5Widgets_DIR}/../..")
    set(QT_PLUGINS_DIR "${QT_LIBS_DIR}/../plugins")
    install(FILES
        ${QT_PLUGINS_DIR}/platforms/libqcocoa.dylib
        DESTINATION ${MAC_BUNDLE_PATH}/Contents/plugins/platforms)
    install(FILES
        ${QT_PLUGINS_DIR}/iconengines/libqsvgicon.dylib
        DESTINATION ${MAC_BUNDLE_PATH}/Contents/plugins/iconengines)
    install(DIRECTORY
        ${QT_PLUGINS_DIR}/imageformats
        DESTINATION ${MAC_BUNDLE_PATH}/Contents/plugins
        FILES_MATCHING REGEX "libq.+[^_debug]\\.dylib"
        PATTERN "*.dSYM" EXCLUDE)
    install(CODE "
        include(BundleUtilities)
        set(BU_CHMOD_BUNDLE_ITEMS ON)
        list(APPEND DIRS /usr/local/lib ${QT_LIBS_DIR})
        file(GLOB_RECURSE QMPLAY2_MODULES_AND_QT_PLUGINS
            \"${MAC_BUNDLE_PATH}/Contents/MacOS/modules/*\"
            \"${MAC_BUNDLE_PATH}/Contents/plugins/*.dylib\")
        fixup_bundle(${MAC_BUNDLE_PATH} \"\${QMPLAY2_MODULES_AND_QT_PLUGINS}\" \"\${DIRS}\")
    ")
else()
    # executable
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    # desktop files
    file(GLOB DESKTOP_FILES Unix/QMPlay2*.desktop)
    install(FILES ${DESKTOP_FILES} DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")

    # mime files
    file(GLOB MIME_FILES Unix/x-*.xml)
    install(FILES ${MIME_FILES} DESTINATION "${INSTALL_PATH_MIME}/packages")

    # icon
    install(DIRECTORY Icons/hicolor DESTINATION ${INSTALL_PATH_ICONS})

    # install Solid actions
    include(InstallSolidActions)

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

    # appdata
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Unix/QMPlay2.appdata.xml.in ${CMAKE_CURRENT_BINARY_DIR}/QMPlay2.appdata.xml @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/QMPlay2.appdata.xml
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo)
endif()
