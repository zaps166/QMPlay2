cmake_minimum_required(VERSION 3.12)
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

pkg_check_modules(LIB_X11_XV REQUIRED x11 xv)
link_directories(${LIB_X11_XV_LIBRARY_DIRS})

include_directories(${LIB_X11_XV_INCLUDE_DIRS})

add_library(${PROJECT_NAME} MODULE
    ${XVideo_HDR}
    ${XVideo_SRC}
    ${XVideo_RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    ${LIB_X11_XV_LIBRARIES}
    libqmplay2
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME}
        PRIVATE
        ${XVideo_HDR}
    )
    set_source_files_properties(xv.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
endif()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
