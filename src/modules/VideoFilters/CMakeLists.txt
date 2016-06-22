cmake_minimum_required(VERSION 2.8.11)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif()
endif()
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
    icons.qrc
)

pkg_check_modules(LIBAVUTIL libavutil>=52.66.100 REQUIRED)
link_directories(${LIBAVUTIL_LIBRARY_DIRS})

if(USE_QT5)
    qt5_add_resources(VideoFilters_RESOURCES_RCC ${VideoFilters_RESOURCES})
else()
    qt4_add_resources(VideoFilters_RESOURCES_RCC ${VideoFilters_RESOURCES})
endif()

include_directories(../../qmplay2/headers ${LIBAVUTIL_INCLUDE_DIRS})

add_library(${PROJECT_NAME} MODULE
    ${VideoFilters_HDR}
    ${VideoFilters_SRC}
    ${VideoFilters_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    ${LIBAVUTIL_LIBRARIES}
    $<TARGET_FILE:qmplay2>
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
