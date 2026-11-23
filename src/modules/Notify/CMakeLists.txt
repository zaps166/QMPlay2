cmake_minimum_required(VERSION 3.16)
project(Notify)

set(Notify_HDR
    NotifyHeaders.hpp
    NotifyExtension.hpp
    Notify.hpp
)

set(Notify_SRC
    NotifyExtension.cpp
    Notify.cpp
)

set(Notify_RESOURCES
    icon.qrc
)

add_library(${PROJECT_NAME} MODULE
    ${Notify_HDR}
    ${Notify_SRC}
    ${Notify_RESOURCES}
)

qmplay2_add_precompile_headers(NotifyHeaders.hpp ${Notify_HDR})

target_link_libraries(${PROJECT_NAME} PRIVATE libqmplay2_iface)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
