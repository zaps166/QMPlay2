cmake_minimum_required(VERSION 3.12)
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

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
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
    libqmplay2
    gdi32
    ddraw
    dxguid
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
