cmake_minimum_required(VERSION 3.16)
project(CUVID)

set(CUVID_HDR
    CUVIDHeaders.hpp
    Cuvid.hpp
    CuvidDec.hpp
    CuvidAPI.hpp
    CuvidHWInterop.hpp
)

set(CUVID_SRC
    Cuvid.cpp
    CuvidDec.cpp
    CuvidAPI.cpp
)

set(CUVID_RESOURCES
   icon.qrc
)

if (USE_OPENGL)
    list(APPEND CUVID_HDR CuvidOpenGL.hpp)
    list(APPEND CUVID_SRC CuvidOpenGL.cpp)
endif()
if (USE_VULKAN)
    list(APPEND CUVID_HDR CuvidVulkan.hpp)
    list(APPEND CUVID_SRC CuvidVulkan.cpp)
endif()

add_library(${PROJECT_NAME} MODULE
    ${CUVID_HDR}
    ${CUVID_SRC}
    ${CUVID_RESOURCES}
)

qmplay2_add_precompile_headers(CUVIDHeaders.hpp ${CUVID_HDR})

target_link_libraries(${PROJECT_NAME} PRIVATE libqmplay2_iface)

if(WIN32 AND USE_OPENGL)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
endif()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
