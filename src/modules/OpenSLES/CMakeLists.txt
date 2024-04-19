cmake_minimum_required(VERSION 3.16)
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

add_library(${PROJECT_NAME} MODULE
    ${OpenSLES_HDR}
    ${OpenSLES_SRC}
    ${OpenSLES_RESOURCES}
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME}
        PRIVATE
        ${OpenSLES_HDR}
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    OpenSLES
)
libqmplay2_set_target_params()
