cmake_minimum_required(VERSION 3.1)
project(Notify)

set(Notify_HDR
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

include_directories(../../qmplay2/headers)

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${Notify_HDR}
    ${Notify_SRC}
    ${Notify_RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    libqmplay2
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
