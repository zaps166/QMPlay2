cmake_minimum_required(VERSION 3.1)
project(FileAssociation)

set(FileAssociation_HDR
    FileAssociation.hpp
)

set(FileAssociation_SRC
    FileAssociation.cpp
)

set(FileAssociation_RESOURCES
    icon.qrc
)

include_directories(../../qmplay2/headers)

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${FileAssociation_HDR}
    ${FileAssociation_SRC}
    ${FileAssociation_RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    libqmplay2
    shlwapi
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
