cmake_minimum_required(VERSION 3.16)
project(FileAssociation)

set(FileAssociation_HDR
    FileAssociationHeaders.hpp
    FileAssociation.hpp
)

set(FileAssociation_SRC
    FileAssociation.cpp
)

set(FileAssociation_RESOURCES
    icon.qrc
)

add_library(${PROJECT_NAME} MODULE
    ${FileAssociation_HDR}
    ${FileAssociation_SRC}
    ${FileAssociation_RESOURCES}
)

qmplay2_add_precompile_headers(FileAssociationHeaders.hpp ${FileAssociation_HDR})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    shlwapi
)
target_link_libraries(${PROJECT_NAME} PRIVATE libqmplay2_iface)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
