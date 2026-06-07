cmake_minimum_required(VERSION 3.16)
project(OpenMPT)

set(OpenMPT_HDR
    OpenMPT.hpp
    OpenMPTDemux.hpp
    OpenMPTVisBase.hpp
    OpenMPTVisPatterns.hpp
    OpenMPTVisSamples.hpp
)

set(OpenMPT_SRC
    OpenMPT.cpp
    OpenMPTDemux.cpp
    OpenMPTVisBase.cpp
    OpenMPTVisPatterns.cpp
    OpenMPTVisSamples.cpp
)

set(OpenMPT_RESOURCES
    icons.qrc
)

add_library(${PROJECT_NAME} MODULE
    ${OpenMPT_HDR}
    ${OpenMPT_SRC}
    ${OpenMPT_RESOURCES}
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        ${OpenMPT_HDR}
    )
endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${OPENMPT_INCLUDE_DIRS}
)
target_link_directories(${PROJECT_NAME}
    PRIVATE
    ${OPENMPT_LIBRARY_DIRS}
)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${OPENMPT_LIBRARIES}
)

libqmplay2_set_target_params()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
