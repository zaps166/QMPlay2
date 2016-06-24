cmake_minimum_required(VERSION 2.8.11)
if(POLICY CMP0003)
    cmake_policy(SET CMP0003 NEW)
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif()
endif(POLICY CMP0003)
project(Visualizations)

set(Visualizations_HDR
    Visualizations.hpp
    SimpleVis.hpp
    FFTSpectrum.hpp
    VisWidget.hpp
)

set(Visualizations_SRC
    Visualizations.cpp
    SimpleVis.cpp
    FFTSpectrum.cpp
    VisWidget.cpp
)

add_definitions(-D__STDC_CONSTANT_MACROS)

pkg_check_modules(LIBAVUTIL libavutil>=52.66.100 REQUIRED)
pkg_check_modules(LIBAVCODEC libavcodec>=55.52.102 REQUIRED)
link_directories(${LIBAVCODEC_LIBRARY_DIRS} ${LIBAVUTIL_LIBRARY_DIRS})

include_directories(../../qmplay2/headers ${LIBAVUTIL_INCLUDE_DIRS} ${LIBAVCODEC_INCLUDE_DIRS})

add_library(${PROJECT_NAME} MODULE
    ${Visualizations_HDR}
    ${Visualizations_SRC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    ${LIBAVUTIL_LIBRARIES}
    ${LIBAVCODEC_LIBRARIES}
    $<TARGET_FILE:qmplay2>
)

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
endif()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
