cmake_minimum_required(VERSION 2.8.6)
if(POLICY CMP0003)
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

set(Visualizations_RESOURCES
    icon.qrc
)

if(USE_OPENGL2 AND USE_QT5 AND NOT Qt5Widgets_VERSION LESS 5.6.0)
    add_definitions(-DUSE_OPENGL)
endif()

link_directories(${LIBAVCODEC_LIBRARY_DIRS} ${LIBAVUTIL_LIBRARY_DIRS})

include_directories(../../qmplay2/headers ${LIBAVUTIL_INCLUDE_DIRS} ${LIBAVCODEC_INCLUDE_DIRS})

if(USE_QT5)
    qt5_add_resources(Visualizations_RESOURCES_RCC ${Visualizations_RESOURCES})
else()
    qt4_add_resources(Visualizations_RESOURCES_RCC ${Visualizations_RESOURCES})
endif()

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${Visualizations_HDR}
    ${Visualizations_SRC}
    ${Visualizations_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui)
endif()

add_dependencies(${PROJECT_NAME} libqmplay2)
target_link_libraries(${PROJECT_NAME}
    ${LIBAVUTIL_LIBRARIES}
    ${LIBAVCODEC_LIBRARIES}
    ${qmplay2lib}
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
