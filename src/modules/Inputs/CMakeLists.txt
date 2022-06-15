cmake_minimum_required(VERSION 3.12)
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

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${Inputs_HDR}
    ${Inputs_SRC}
    ${Inputs_RESOURCES}
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        ${Inputs_HDR}
    )
endif()

target_link_libraries(${PROJECT_NAME}
    libqmplay2
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
