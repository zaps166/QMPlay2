cmake_minimum_required(VERSION 2.8.6)
if(COMMAND cmake_policy)
    if(POLICY CMP0003)
        cmake_policy(SET CMP0003 NEW)
    endif()
    if(POLICY CMP0020)
        cmake_policy(SET CMP0020 NEW)
    endif()
    if(POLICY CMP0042)
        cmake_policy(SET CMP0042 NEW)
    endif()
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

if(APPLE)
    set(PortAudio_SRC ${PortAudio_SRC}
        Mac/AudioDevice.mm
        Mac/AudioDeviceList.cpp
    )
endif()

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

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${PortAudio_HDR}
    ${PortAudio_SRC}
    ${PortAudio_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui)
endif()

add_dependencies(${PROJECT_NAME} libqmplay2)
target_link_libraries(${PROJECT_NAME}
    ${qmplay2lib}
)

if(WIN32)
    set(CUSTOM_PORTAUDIO_LIBRARIES "" CACHE STRING "Custom libraries for PortAudio")
    mark_as_advanced(CUSTOM_PORTAUDIO_LIBRARIES)
endif()

if(WIN32 AND CUSTOM_PORTAUDIO_LIBRARIES)
    separate_arguments(CUSTOM_PORTAUDIO_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} ${CUSTOM_PORTAUDIO_LIBRARIES})
else()
    target_link_libraries(${PROJECT_NAME} ${LIBPORTAUDIO_LIBRARIES})
endif()
if(APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework CoreAudio -framework Foundation")
endif()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
