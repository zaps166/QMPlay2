cmake_minimum_required(VERSION 3.16)
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

add_library(${PROJECT_NAME} MODULE
    ${Visualizations_HDR}
    ${Visualizations_SRC}
    ${Visualizations_RESOURCES}
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        ${Visualizations_HDR}
    )
endif()

libqmplay2_set_target_params()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
