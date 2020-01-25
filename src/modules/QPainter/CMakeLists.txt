cmake_minimum_required(VERSION 3.1)
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

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${QPainter_HDR}
    ${QPainter_SRC}
    ${QPainter_RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    libqmplay2
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
