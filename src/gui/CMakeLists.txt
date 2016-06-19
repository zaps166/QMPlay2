cmake_minimum_required(VERSION 2.8.11)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif()
endif()
project(gui)
find_package(PkgConfig REQUIRED)

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
    VideoEqualizer.hpp
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
    VideoEqualizer.cpp
    Appearance.cpp
    VolWidget.cpp
)

set(GUI_FORMS # *.ui files

)

set(GUI_RESOURCES
    resources.qrc
)

if(USE_TAGLIB)
    add_definitions(-DQMPlay2_TagEditor)
    list(APPEND GUI_HDR TagEditor.hpp)
    list(APPEND GUI_SRC TagEditor.cpp)

    PKG_CHECK_MODULES(TAGLIB taglib REQUIRED)
    include_directories(${TAGLIB_INCLUDE_DIRS})
    link_directories(${TAGLIB_LIBRARY_DIRS})
endif()

if(USE_QT5)
    qt5_wrap_ui(GUI_FORM_HDR ${GUI_FORMS})
    qt5_add_resources(GUI_RESOURCES_RCC ${GUI_RESOURCES})
else()
    qt4_wrap_ui(GUI_FORM_HDR ${GUI_FORMS})
    qt4_add_resources(GUI_RESOURCES_RCC ${GUI_RESOURCES})
endif()

include_directories(../qmplay2/headers)

add_executable(QMPlay2
    ${GUI_HDR}
    ${GUI_SRC}
    ${GUI_FORM_HDR}
    ${GUI_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(QMPlay2 Gui Widgets Network)
else()
    target_link_libraries(QMPlay2 Qt4::QtGui Qt4::QtCore Qt4::QtNetwork)
endif()

add_dependencies(QMPlay2 qmplay2)
target_link_libraries(QMPlay2 $<TARGET_FILE:qmplay2>)

if(USE_TAGLIB)
    target_link_libraries(QMPlay2 ${TAGLIB_LIBRARIES})
endif()

install(TARGETS QMPlay2 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# desktop files
file(GLOB DESKTOP_FILES Unix/QMPlay2*.desktop)
install(FILES ${DESKTOP_FILES} DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/applications/")

# mime files
file(GLOB MIME_FILES Unix/x-*.xml)
install(FILES ${MIME_FILES} DESTINATION ${INSTALL_PATH_MIME}/packages/)

install(FILES Icons/QMPlay2.png DESTINATION ${INSTALL_PATH_ICONS}/hicolor/128x128/apps/)

# install solid-actions
if(INSTALL_PATH_SOLID_ACTION AND NOT INSTALL_PATH_SOLID_ACTION STREQUAL "None")
    if(INSTALL_PATH_SOLID_ACTION STREQUAL "Auto")
        include(FindSolidActions)
    else()
        set(LOCAL_INSTALL_PATH_SOLID_ACTION "${INSTALL_PATH_SOLID_ACTION}")
    endif()
    if(LOCAL_INSTALL_PATH_SOLID_ACTION)
        install(FILES Unix/solid-actions/QMPlay2-opencda.desktop DESTINATION "${LOCAL_INSTALL_PATH_SOLID_ACTION}")
    else()
        message(WARNING "No path found for solid actions - won't be installed")
    endif()
endif()

# man page
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Unix/QMPlay2.1.in ${CMAKE_CURRENT_BINARY_DIR}/QMPlay2.1 @ONLY)
find_program(GZIP gzip)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/QMPlay2.1.gz
    COMMAND ${GZIP} -c ${CMAKE_CURRENT_BINARY_DIR}/QMPlay2.1 > QMPlay2.1.gz
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/QMPlay2.1)
add_custom_target(man ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/QMPlay2.1.gz)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/QMPlay2.1.gz
    DESTINATION ${INSTALL_PATH_MAN}/man1/)
