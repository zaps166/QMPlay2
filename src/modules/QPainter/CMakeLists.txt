cmake_minimum_required(VERSION 3.16)
project(QPainter)

set(QPainter_HDR
    QPainterHeaders.hpp
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

qmplay2_add_precompile_headers(QPainterHeaders.hpp ${QPainter_HDR})

target_link_libraries(${PROJECT_NAME} PRIVATE libqmplay2_iface)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
