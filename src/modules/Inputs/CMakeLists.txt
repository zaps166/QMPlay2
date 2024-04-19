cmake_minimum_required(VERSION 3.16)
project(Inputs)

set(Inputs_HDR
    Inputs.hpp
    ToneGenerator.hpp
    PCM.hpp
    Rayman2.hpp
)

set(Inputs_SRC
    Inputs.cpp
    ToneGenerator.cpp
    PCM.cpp
    Rayman2.cpp
)

set(Inputs_RESOURCES
    icons.qrc
)

add_library(${PROJECT_NAME} MODULE
    ${Inputs_HDR}
    ${Inputs_SRC}
    ${Inputs_RESOURCES}
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        ${Inputs_HDR}
    )
endif()

libqmplay2_set_target_params()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
