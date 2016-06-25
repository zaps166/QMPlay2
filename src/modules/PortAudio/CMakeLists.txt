cmake_minimum_required(VERSION 2.8.11)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif()
endif()
project(PortAudio)

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

pkg_check_modules(LIBPORTAUDIO portaudio-2.0 REQUIRED)
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
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    $<TARGET_FILE:qmplay2>
)

if(WIN32)
    set(STATIC_PORTAUDIO_LIBRARIES "" CACHE STRING "Static libs for PortAudio")
    mark_as_advanced(STATIC_PORTAUDIO_LIBRARIES)
endif()

if(WIN32 AND STATIC_PORTAUDIO_LIBRARIES)
    separate_arguments(STATIC_PORTAUDIO_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} ${STATIC_PORTAUDIO_LIBRARIES})
else()
    target_link_libraries(${PROJECT_NAME} ${LIBPORTAUDIO_LIBRARIES})
endif()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
