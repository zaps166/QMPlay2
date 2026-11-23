cmake_minimum_required(VERSION 3.16)
project(OpenSLES-QMPlay2)

set(OpenSLES_HDR
    OpenSLESHeaders.hpp
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

add_library(${PROJECT_NAME} MODULE
    ${OpenSLES_HDR}
    ${OpenSLES_SRC}
    ${OpenSLES_RESOURCES}
)

qmplay2_add_precompile_headers(OpenSLESHeaders.hpp ${OpenSLES_HDR})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    OpenSLES
)
target_link_libraries(${PROJECT_NAME} PRIVATE libqmplay2_iface)
