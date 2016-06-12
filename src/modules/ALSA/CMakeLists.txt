project(ALSA)
cmake_minimum_required(VERSION 3.0.2)
FIND_PACKAGE(PkgConfig REQUIRED)

set(ALSA_HDR
    ALSA.hpp
    ALSACommon.hpp
    ALSAWriter.hpp
)

set(ALSA_SRC
    ALSA.cpp
    ALSACommon.cpp
    ALSAWriter.cpp
)

set(ALSA_RESOURCES
    icon.qrc
)

PKG_CHECK_MODULES(ALSA alsa REQUIRED)
link_directories(${ALSA_LIBRARY_DIRS})

if(USE_QT5)
    qt5_add_resources(ALSA_RESOURCES_RCC ${ALSA_RESOURCES})
else()
    qt4_add_resources(ALSA_RESOURCES_RCC ${ALSA_RESOURCES})
endif()

include_directories(../../qmplay2/headers ${ALSA_INCLUDE_DIRS})

add_library(${PROJECT_NAME} MODULE
    ${ALSA_HDR}
    ${ALSA_SRC}
    ${ALSA_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtGui Qt4::QtCore)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    ${ALSA_LIBRARIES}
    $<TARGET_FILE:qmplay2>
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
