cmake_minimum_required(VERSION 3.1)
project(VideoFilters)

set(VideoFilters_HDR
    VFilters.hpp
    BobDeint.hpp
    BlendDeint.hpp
    DiscardDeint.hpp
    MotionBlur.hpp
    YadifDeint.hpp
)

set(VideoFilters_SRC
    VFilters.cpp
    BobDeint.cpp
    BlendDeint.cpp
    DiscardDeint.cpp
    MotionBlur.cpp
    YadifDeint.cpp
)

set(VideoFilters_RESOURCES
    icon.qrc
)

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${VideoFilters_HDR}
    ${VideoFilters_SRC}
    ${VideoFilters_RESOURCES}
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        ${VideoFilters_HDR}
    )
endif()

target_link_libraries(${PROJECT_NAME}
    libqmplay2
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
