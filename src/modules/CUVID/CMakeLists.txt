cmake_minimum_required(VERSION 3.1)
project(CUVID)

set(CUVID_HDR
    Cuvid.hpp
    CuvidDec.hpp
    CuvidAPI.hpp
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

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${CUVID_HDR}
    ${CUVID_SRC}
    ${CUVID_RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    libqmplay2
)

if(WIN32 AND USE_OPENGL)
    target_link_libraries(${PROJECT_NAME} opengl32)
endif()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
