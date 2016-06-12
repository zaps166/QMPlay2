project(XVideo)
cmake_minimum_required(VERSION 3.0.2)
FIND_PACKAGE(PkgConfig REQUIRED)

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

PKG_CHECK_MODULES(LIBX11 xv x11 REQUIRED)
link_directories(${LIBX11_LIBRARY_DIRS})

if(USE_QT5)
    qt5_add_resources(XVideo_RESOURCES_RCC ${XVideo_RESOURCES})
else()
    qt4_add_resources(XVideo_RESOURCES_RCC ${XVideo_RESOURCES})
endif()

include_directories(../../qmplay2/headers ${LIBX11_INCLUDE_DIRS})

add_library(${PROJECT_NAME} MODULE
    ${XVideo_HDR}
    ${XVideo_SRC}
    ${XVideo_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets X11Extras)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtGui Qt4::QtCore)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    ${LIBX11_LIBRARIES}
    $<TARGET_FILE:qmplay2>
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
