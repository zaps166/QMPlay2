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
project(Notifies)

set(Notifies_HDR
    Notify.hpp
    NotifyExtension.hpp
    Notifies.hpp
    TrayNotify.hpp
)

set(Notifies_SRC
    Notifies.cpp
    NotifyExtension.cpp
    TrayNotify.cpp
)

set(Notifies_RESOURCES
    icon.qrc
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    list(APPEND Notifies_HDR FreedesktopNotify.hpp)
    list(APPEND Notifies_SRC FreedesktopNotify.cpp)
    if(USE_QT5)
        set(DBUS DBus)
        find_package(Qt5DBus REQUIRED)
        qt5_add_dbus_interface(Notifies_SRC org.freedesktop.Notifications.xml notifications_interface)
    else()
        set(DBUS Qt4::QtDBus)
        find_package(Qt4 REQUIRED QtDBus)
        qt4_add_dbus_interface(Notifies_SRC org.freedesktop.Notifications.xml notifications_interface)
    endif()
    add_definitions(-DFREEDESKTOP_NOTIFY)
endif()

include_directories(../../qmplay2/headers)

if(USE_QT5)
    qt5_add_resources(Notifies_RESOURCES_RCC ${Notifies_RESOURCES})
else()
    qt4_add_resources(Notifies_RESOURCES_RCC ${Notifies_RESOURCES})
endif()

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${Notifies_HDR}
    ${Notifies_SRC}
    ${Notifies_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets ${DBUS})
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui ${DBUS})
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    ${libqmplay2}
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
