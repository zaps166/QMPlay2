cmake_minimum_required(VERSION 3.12)
project(PipeWire)

set(PipeWire_HDR
    PipeWire.hpp
    PipeWireWriter.hpp
)

set(PipeWire_SRC
    PipeWire.cpp
    PipeWireWriter.cpp
)

set(PipeWire_RESOURCES
    icon.qrc
)

pkg_check_modules(LIBPIPEWIRE REQUIRED libpipewire-0.3)
link_directories(${LIBPIPEWIRE_LIBRARY_DIRS})

include_directories(${LIBPIPEWIRE_INCLUDE_DIRS})

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${PipeWire_HDR}
    ${PipeWire_SRC}
    ${PipeWire_RESOURCES}
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        ${PipeWire_HDR}
    )
endif()

target_link_libraries(${PROJECT_NAME}
    ${LIBPIPEWIRE_LIBRARIES}
    libqmplay2
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
