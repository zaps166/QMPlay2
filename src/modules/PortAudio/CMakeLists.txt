project(PortAudio)
cmake_minimum_required(VERSION 2.8.11)
FIND_PACKAGE(PkgConfig REQUIRED)

set(PortAudio_HDR
    PortAudio.hpp
    PortAudioCommon.hpp
    PortAudioWriter.hpp
)

set(PortAudio_SRC
    PortAudio.cpp
    PortAudioCommon.cpp
    PortAudioWriter.cpp
)

set(PortAudio_RESOURCES
    icon.qrc
)

PKG_CHECK_MODULES(LIBPORTAUDIO portaudio-2.0 REQUIRED)
link_directories(${LIBPORTAUDIO_LIBRARY_DIRS})

if(USE_QT5)
    qt5_add_resources(PortAudio_RESOURCES_RCC ${PortAudio_RESOURCES})
else()
    qt4_add_resources(PortAudio_RESOURCES_RCC ${PortAudio_RESOURCES})
endif()

include_directories(../../qmplay2/headers ${LIBPORTAUDIO_INCLUDE_DIRS})

add_library(${PROJECT_NAME} MODULE
    ${PortAudio_HDR}
    ${PortAudio_SRC}
    ${PortAudio_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtGui Qt4::QtCore)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    ${LIBPORTAUDIO_LIBRARIES}
    $<TARGET_FILE:qmplay2>
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
