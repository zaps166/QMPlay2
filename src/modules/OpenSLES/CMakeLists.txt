cmake_minimum_required(VERSION 3.1)
project(OpenSLES-QMPlay2)

set(OpenSLES_HDR
    OpenSLES.hpp
    OpenSLESWriter.hpp
)

set(OpenSLES_SRC
    OpenSLES.cpp
    OpenSLESWriter.cpp
)

set(OpenSLES_RESOURCES
    icon.qrc
)

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${OpenSLES_HDR}
    ${OpenSLES_SRC}
    ${OpenSLES_RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    libqmplay2
    OpenSLES
)
