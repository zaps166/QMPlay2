cmake_minimum_required(VERSION 3.13)
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
    VideoAdjustmentW.hpp
    Appearance.hpp
    VolWidget.hpp
    ScreenSaver.hpp
    ShortcutHandler.hpp
    KeyBindingsDialog.hpp
    RepeatMode.hpp
    PanGestureEventFilter.hpp
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
    VideoAdjustmentW.cpp
    Appearance.cpp
    VolWidget.cpp
    ShortcutHandler.cpp
    KeyBindingsDialog.cpp
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

if(NOT WIN32)
    list(APPEND GUI_SRC
        PanGestureEventFilter.cpp
    )
endif()

if(APPLE)
    list(APPEND GUI_HDR
        macOS/QMPlay2MacExtensions.hpp
    )
    list(APPEND GUI_SRC
        macOS/QMPlay2MacExtensions.mm
    )

    include_directories("macOS")
endif()

if(BUILD_WITH_QT6)
    qt6_wrap_ui(GUI_FORM_HDR ${GUI_FORMS})
else()
    qt5_wrap_ui(GUI_FORM_HDR ${GUI_FORMS})
endif()
set_property(SOURCE ${GUI_FORM_HDR} PROPERTY SKIP_AUTOMOC ON)

if(USE_UPDATES)
    list(APPEND GUI_HDR Updater.hpp)
    list(APPEND GUI_SRC Updater.cpp)
    add_definitions(-DUPDATES)
endif()

if(USE_TAGLIB)
    find_package(TagLib REQUIRED)
    include_directories(${TAGLIB_INCLUDE_DIRS})
    link_directories(${TAGLIB_LIBRARY_DIRS})

    add_definitions(-DQMPlay2_TagEditor)
    if(WIN32)
        add_definitions(-DTAGLIB_FULL_INCLUDE_PATH)

        option(TAGLIB_STATIC_LINK "Use TagLib static library" OFF)
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

if(USE_FFMPEG_VAAPI AND USE_OPENGL)
    set_source_files_properties(Main.cpp PROPERTIES COMPILE_DEFINITIONS CHECK_FOR_EGL)
endif()

if(WIN32)
    # set RC file
    list(APPEND GUI_SRC Windows/icons.rc)

    # updater
    if(USE_UPDATES)
        add_definitions(-DUPDATER)
    endif()

    # additional Windows sources
    list(APPEND GUI_SRC
        Windows/ScreenSaver.cpp
        Windows/PanGestureEventFilter.cpp
    )

    # win extras
    list(APPEND GUI_SRC
        "Windows/qt/QtWinExtras/qwinevent.cpp"
        "Windows/qt/QtWinExtras/qwineventfilter.cpp"
        "Windows/qt/QtWinExtras/qwinfunctions.cpp"
        "Windows/qt/QtWinExtras/qwintaskbarbutton.cpp"
        "Windows/qt/QtWinExtras/qwintaskbarprogress.cpp"
        "Windows/qt/QtWinExtras/qwinthumbnailtoolbar.cpp"
        "Windows/qt/QtWinExtras/qwinthumbnailtoolbutton.cpp"
        "Windows/qt/QtWinExtras/windowsguidsdefs.cpp"
    )
    if(NOT BUILD_WITH_QT6)
        set_source_files_properties("Windows/qt/QtWinExtras/qwinfunctions.cpp" PROPERTIES COMPILE_DEFINITIONS UNICODE)
    endif()
    include_directories("Windows/qt")
    add_definitions(-DQT_BUILD_WINEXTRAS_LIB)
elseif(APPLE)
    set(MACOSX_BUNDLE_BUNDLE_VERSION "1")
    set(MACOSX_BUNDLE_ICON_FILE "QMPlay2.icns")
    set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${QMPLAY2_VERSION})
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright (C) 2010-2024  Błażej Szczygieł")

    set(ICNS_FILE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/macOS/QMPlay2.icns")
    set_source_files_properties(${ICNS_FILE_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    set(ICNS_FILE_FILE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/macOS/QMPlay2File.icns")
    set_source_files_properties(${ICNS_FILE_FILE_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    set(ICNS_PLAYLIST_FILE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/macOS/QMPlay2List.icns")
    set_source_files_properties(${ICNS_PLAYLIST_FILE_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    # screen saver
    list(APPEND GUI_SRC macOS/ScreenSaver.cpp)
elseif(ANDROID)
    # screen saver
    list(APPEND GUI_SRC Android/ScreenSaver.cpp)
else()
    # screen saver
    list(APPEND GUI_SRC Unix/ScreenSaver.cpp)
endif()

# RPATH
if(SET_INSTALL_RPATH)
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
endif()

if(WIN32 AND NOT USE_CMD)
    set(WIN32_EXE WIN32) # don't show CMD when running on windows
endif()

set(GUI_FILES
    ${GUI_HDR}
    ${GUI_SRC}
    ${GUI_FORM_HDR}
    ${GUI_RESOURCES}
)
if(NOT ANDROID)
    add_executable(${PROJECT_NAME}
        ${WIN32_EXE}
        MACOSX_BUNDLE ${ICNS_FILE_PATH} ${ICNS_FILE_FILE_PATH} ${ICNS_PLAYLIST_FILE_PATH}
        ${GUI_FILES}
    )
else()
    qt_add_executable(${PROJECT_NAME}
        ${GUI_FILES}
    )
    if(ANDROID_ABI STREQUAL "armeabi-v7a")
        set(ANDROID_EXTRA_LIBS_DIR "${CMAKE_SYSROOT}/usr/lib/arm-linux-androideabi")
    endif()
    set(ANDROID_OPENSSL_DIR "${CMAKE_SYSROOT}/../../../../../../../android_openssl/ssl_1.1/${ANDROID_ABI}")
    set(ANDROID_QMPLAY2_MODULES "${CMAKE_BINARY_DIR}/src/modules")
    set_target_properties(${PROJECT_NAME} PROPERTIES QT_ANDROID_EXTRA_LIBS
        "${ANDROID_QMPLAY2_MODULES}/AudioFilters/libAudioFilters.so,${ANDROID_QMPLAY2_MODULES}/Extensions/libExtensions.so,${ANDROID_QMPLAY2_MODULES}/FFmpeg/libFFmpeg.so,${ANDROID_QMPLAY2_MODULES}/Inputs/libInputs.so,${ANDROID_QMPLAY2_MODULES}/Modplug/libModplug.so,${ANDROID_QMPLAY2_MODULES}/Notify/libNotify.so,${ANDROID_QMPLAY2_MODULES}/OpenSLES/libOpenSLES-QMPlay2.so,${ANDROID_QMPLAY2_MODULES}/Playlists/libPlaylists.so,${ANDROID_QMPLAY2_MODULES}/QPainter/libQPainter.so,${ANDROID_QMPLAY2_MODULES}/Subtitles/libSubtitles.so,${ANDROID_QMPLAY2_MODULES}/VideoFilters/libVideoFilters.so,${ANDROID_QMPLAY2_MODULES}/Visualizations/libVisualizations.so,${ANDROID_EXTRA_LIBS_DIR}/libavcodec.so,${ANDROID_EXTRA_LIBS_DIR}/libavformat.so,${ANDROID_EXTRA_LIBS_DIR}/libavutil.so,${ANDROID_EXTRA_LIBS_DIR}/libswresample.so,${ANDROID_EXTRA_LIBS_DIR}/libswscale.so,${ANDROID_OPENSSL_DIR}/libssl_1_1.so,${ANDROID_OPENSSL_DIR}/libcrypto_1_1.so"
    )
    set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/gui/Android/manifest")
endif()

libqmplay2_set_target_params()

if(USE_PCH)
    if(APPLE)
        set_source_files_properties(macOS/QMPlay2MacExtensions.mm PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
    endif()
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        ${GUI_HDR}
        ${GUI_FORM_HDR}
    )
    set_source_files_properties(Windows/PanGestureEventFilter.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
endif()

if(WIN32)
    set(CUSTOM_GUI_LIBRARIES "" CACHE STRING "Custom libraries for GUI")
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm)
    target_link_libraries(${PROJECT_NAME} PRIVATE dwmapi)
elseif(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macOS/BundleInfo.plist.in")
    target_link_libraries(${PROJECT_NAME} PRIVATE ${APPKIT_LIBRARY} ${IOKIT_LIBRARY})
endif()
if(USE_TAGLIB AND (NOT WIN32 OR CMAKE_HOST_WIN32))
    target_link_libraries(${PROJECT_NAME} PRIVATE ${TAGLIB_LIBRARIES})
endif()
if(WIN32 AND CUSTOM_GUI_LIBRARIES)
    separate_arguments(CUSTOM_GUI_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CUSTOM_GUI_LIBRARIES})
endif()

if(WIN32)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/)
elseif(APPLE)
    install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION ${CMAKE_INSTALL_PREFIX})

    set(QT_LIBS_DIR "${${QT_PREFIX}Widgets_DIR}/../..")
    set(QT_PLUGINS_DIR "${QT_LIBS_DIR}/../plugins")
    install(FILES
        "${QT_PLUGINS_DIR}/platforms/libqcocoa.dylib"
        DESTINATION "${MAC_BUNDLE_PATH}/Contents/plugins/platforms")
    install(FILES
        "${QT_PLUGINS_DIR}/iconengines/libqsvgicon.dylib"
        DESTINATION "${MAC_BUNDLE_PATH}/Contents/plugins/iconengines")
    install(FILES
        "${QT_PLUGINS_DIR}/styles/libqmacstyle.dylib"
        DESTINATION "${MAC_BUNDLE_PATH}/Contents/plugins/styles"
        OPTIONAL)
    install(DIRECTORY
        "${QT_PLUGINS_DIR}/imageformats"
        DESTINATION "${MAC_BUNDLE_PATH}/Contents/plugins"
        PATTERN "*_debug.dylib" EXCLUDE
        PATTERN "*.dSYM" EXCLUDE)
    install(DIRECTORY
        "${QT_LIBS_DIR}/../translations"
        DESTINATION "${MAC_BUNDLE_PATH}/Contents"
        FILES_MATCHING
        PATTERN "qtbase_*.qm")
    if(EXISTS "/usr/local/bin/ffmpeg")
        install(PROGRAMS
            "/usr/local/bin/ffmpeg"
            DESTINATION "${MAC_BUNDLE_PATH}/Contents/MacOS")
    else()
        message(WARNING "FFmpeg executable not copied!")
    endif()
    install(CODE "
        include(BundleUtilities)
        set(BU_CHMOD_BUNDLE_ITEMS ON)
        list(APPEND DIRS /usr/local/lib ${QT_LIBS_DIR})
        file(GLOB_RECURSE QMPLAY2_MODULES_AND_QT_PLUGINS
            \"${MAC_BUNDLE_PATH}/Contents/MacOS/modules/*\"
            \"${MAC_BUNDLE_PATH}/Contents/plugins/*.dylib\")
        file(WRITE \"${MAC_BUNDLE_PATH}/Contents/Resources/qt.conf\")
        fixup_bundle(${MAC_BUNDLE_PATH} \"\${QMPLAY2_MODULES_AND_QT_PLUGINS}\" \"\${DIRS}\")
    ")
elseif(NOT ANDROID)
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
