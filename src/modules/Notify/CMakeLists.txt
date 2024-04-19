cmake_minimum_required(VERSION 3.16)
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

add_library(${PROJECT_NAME} MODULE
    ${Notify_HDR}
    ${Notify_SRC}
    ${Notify_RESOURCES}
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        ${Notify_HDR}
    )
endif()

libqmplay2_set_target_params()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
