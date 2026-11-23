cmake_minimum_required(VERSION 3.16)
project(DirectX)

set(DirectX_HDR
    DirectXHeaders.hpp
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

qmplay2_add_precompile_headers(DirectXHeaders.hpp ${DirectX_HDR})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    gdi32
    ddraw
    dxguid
)
target_link_libraries(${PROJECT_NAME} PRIVATE libqmplay2_iface)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
