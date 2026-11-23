cmake_minimum_required(VERSION 3.16)
project(OpenMPT)

set(OpenMPT_HDR
    OpenMPTHeaders.hpp
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

qmplay2_add_precompile_headers(OpenMPTHeaders.hpp ${OpenMPT_HDR})

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

target_link_libraries(${PROJECT_NAME} PRIVATE libqmplay2_iface)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
