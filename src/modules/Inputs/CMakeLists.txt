cmake_minimum_required(VERSION 3.16)
project(Inputs)

set(Inputs_HDR
    InputsHeaders.hpp
    Inputs.hpp
    ToneGenerator.hpp
    PCM.hpp
)

set(Inputs_SRC
    Inputs.cpp
    ToneGenerator.cpp
    PCM.cpp
)

set(Inputs_RESOURCES
    icons.qrc
)

add_library(${PROJECT_NAME} MODULE
    ${Inputs_HDR}
    ${Inputs_SRC}
    ${Inputs_RESOURCES}
)

qmplay2_add_precompile_headers(InputsHeaders.hpp ${Inputs_HDR})

target_link_libraries(${PROJECT_NAME} PRIVATE libqmplay2_iface)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
