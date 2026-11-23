cmake_minimum_required(VERSION 3.16)
project(Visualizations)

set(Visualizations_HDR
    VisualizationsHeaders.hpp
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

add_library(${PROJECT_NAME} MODULE
    ${Visualizations_HDR}
    ${Visualizations_SRC}
    ${Visualizations_RESOURCES}
)

qmplay2_add_precompile_headers(VisualizationsHeaders.hpp ${Visualizations_HDR})

target_link_libraries(${PROJECT_NAME} PRIVATE libqmplay2_iface)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
