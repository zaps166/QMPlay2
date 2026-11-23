cmake_minimum_required(VERSION 3.16)
project(XVideo)

set(XVideo_HDR
    XVideoHeaders.hpp
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

pkg_check_modules(LIB_X11_XV REQUIRED x11 xv)
link_directories(${LIB_X11_XV_LIBRARY_DIRS})

include_directories(${LIB_X11_XV_INCLUDE_DIRS})

add_library(${PROJECT_NAME} MODULE
    ${XVideo_HDR}
    ${XVideo_SRC}
    ${XVideo_RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${LIB_X11_XV_LIBRARIES}
)
target_link_libraries(${PROJECT_NAME} PRIVATE libqmplay2_iface)

qmplay2_add_precompile_headers(XVideoHeaders.hpp ${XVideo_HDR})

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
