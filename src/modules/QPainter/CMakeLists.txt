cmake_minimum_required(VERSION 3.16)
project(QPainter)

set(QPainter_HDR
    QPainterWriter.hpp
    QPainter.hpp
)

set(QPainter_SRC
    QPainterWriter.cpp
    QPainter.cpp
)

set(QPainter_RESOURCES
    icon.qrc
)

add_library(${PROJECT_NAME} MODULE
    ${QPainter_HDR}
    ${QPainter_SRC}
    ${QPainter_RESOURCES}
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        ${QPainter_HDR}
    )
endif()

libqmplay2_set_target_params()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
