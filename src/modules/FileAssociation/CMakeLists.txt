cmake_minimum_required(VERSION 3.16)
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

add_library(${PROJECT_NAME} MODULE
    ${FileAssociation_HDR}
    ${FileAssociation_SRC}
    ${FileAssociation_RESOURCES}
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME}
        PRIVATE
        ${FileAssociation_HDR}
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    shlwapi
)
libqmplay2_set_target_params()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
