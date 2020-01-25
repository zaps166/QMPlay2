cmake_minimum_required(VERSION 3.1)
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

pkg_check_modules(LIB_X11_XV x11 xv REQUIRED)
link_directories(${LIB_X11_XV_LIBRARY_DIRS})

include_directories(${LIB_X11_XV_INCLUDE_DIRS})

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${XVideo_HDR}
    ${XVideo_SRC}
    ${XVideo_RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    ${LIB_X11_XV_LIBRARIES}
    libqmplay2
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
