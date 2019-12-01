cmake_minimum_required(VERSION 3.1)
project(CUVID)

set(CUVID_HDR
    Cuvid.hpp
    CuvidDec.hpp
)

set(CUVID_SRC
    Cuvid.cpp
    CuvidDec.cpp
)

set(CUVID_RESOURCES
   icon.qrc
)

link_directories(
    ${LIBAVCODEC_LIBRARY_DIRS}
    ${LIBSWSCALE_LIBRARY_DIRS}
    ${LIBAVUTIL_LIBRARY_DIRS}
)

set(CUVID_LIBS
    ${LIBAVCODEC_LIBRARIES}
    ${LIBSWSCALE_LIBRARIES}
    ${LIBAVUTIL_LIBRARIES}
)

include_directories(../../qmplay2/headers
    ${LIBAVCODEC_INCLUDE_DIRS}
    ${LIBSWSCALE_INCLUDE_DIRS}
    ${LIBAVUTIL_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${CUVID_HDR}
    ${CUVID_SRC}
    ${CUVID_RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    ${CUVID_LIBS}
    libqmplay2
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} opengl32)
endif()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
