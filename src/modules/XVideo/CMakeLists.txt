cmake_minimum_required(VERSION 2.8.11)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0043 NEW)
endif()
project(XVideo)

set(XVideo_HDR
    XVideoWriter.hpp
    XVideo.hpp
    xv.hpp
)

set(XVideo_SRC
    XVideoWriter.cpp
    XVideo.cpp
    xv.cpp
)

set(XVideo_RESOURCES
    icon.qrc
)

find_package(X11 REQUIRED)

if(USE_QT5)
    qt5_add_resources(XVideo_RESOURCES_RCC ${XVideo_RESOURCES})
else()
    qt4_add_resources(XVideo_RESOURCES_RCC ${XVideo_RESOURCES})
endif()

include_directories(../../qmplay2/headers ${X11_X11_INCLUDE_PATH} ${X11_Xv_INCLUDE_PATH})

add_library(${PROJECT_NAME} MODULE
    ${XVideo_HDR}
    ${XVideo_SRC}
    ${XVideo_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtGui Qt4::QtCore)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    ${X11_X11_LIB}
    ${X11_Xv_LIB}
    $<TARGET_FILE:qmplay2>
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
