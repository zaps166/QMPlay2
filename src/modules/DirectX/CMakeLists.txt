cmake_minimum_required(VERSION 2.8.6)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif()
endif()
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

if(USE_QT5)
    qt5_add_resources(DirectX_RESOURCES_RCC ${DirectX_RESOURCES})
else()
    qt4_add_resources(DirectX_RESOURCES_RCC ${DirectX_RESOURCES})
endif()

include_directories(../../qmplay2/headers)

add_library(${PROJECT_NAME} MODULE
    ${DirectX_HDR}
    ${DirectX_SRC}
    ${DirectX_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    ${libqmplay2}
    gdi32
    ddraw
    dxguid
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
