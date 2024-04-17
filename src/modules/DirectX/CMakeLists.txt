cmake_minimum_required(VERSION 3.13)
project(DirectX)

set(DirectX_HDR
    DirectDraw.hpp
    DirectX.hpp
)

set(DirectX_SRC
    DirectDraw.cpp
    DirectX.cpp
)

set(DirectX_RESOURCES
    icon.qrc
)

add_library(${PROJECT_NAME} MODULE
    ${DirectX_HDR}
    ${DirectX_SRC}
    ${DirectX_RESOURCES}
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME}
        PRIVATE
        ${DirectX_HDR}
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    gdi32
    ddraw
    dxguid
)
libqmplay2_set_target_params()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
