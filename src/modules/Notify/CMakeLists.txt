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
project(Notify)

set(Notify_HDR
    NotifyExtension.hpp
    Notify.hpp
)

set(Notify_SRC
    NotifyExtension.cpp
    Notify.cpp
)

set(Notify_RESOURCES
    icon.qrc
)

include_directories(../../qmplay2/headers)

if(USE_QT5)
    qt5_add_resources(Notify_RESOURCES_RCC ${Notify_RESOURCES})
else()
    qt4_add_resources(Notify_RESOURCES_RCC ${Notify_RESOURCES})
endif()

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${Notify_HDR}
    ${Notify_SRC}
    ${Notify_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui)
endif()

add_dependencies(${PROJECT_NAME} libqmplay2)
target_link_libraries(${PROJECT_NAME}
    ${qmplay2lib}
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
