cmake_minimum_required(VERSION 3.16)
project(PipeWire)

set(PipeWire_HDR
    PipeWireHeaders.hpp
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

add_library(${PROJECT_NAME} MODULE
    ${PipeWire_HDR}
    ${PipeWire_SRC}
    ${PipeWire_RESOURCES}
)

qmplay2_add_precompile_headers(PipeWireHeaders.hpp ${PipeWire_HDR})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${LIBPIPEWIRE_LIBRARIES}
)
target_link_libraries(${PROJECT_NAME} PRIVATE libqmplay2_iface)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
