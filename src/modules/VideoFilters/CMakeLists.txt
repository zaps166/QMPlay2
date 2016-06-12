project(VideoFilters)
cmake_minimum_required(VERSION 3.0.2)
FIND_PACKAGE(PkgConfig REQUIRED)

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

PKG_CHECK_MODULES(LIBAVUTIL libavutil REQUIRED)
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
    target_link_libraries(${PROJECT_NAME} Qt4::QtGui Qt4::QtCore)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    ${LIBAVUTIL_LIBRARIES}
    $<TARGET_FILE:qmplay2>
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
