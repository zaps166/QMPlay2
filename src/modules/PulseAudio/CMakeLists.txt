cmake_minimum_required(VERSION 2.8.11)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif()
endif()
project(PulseAudio)

set(PulseAudio_HDR
    PulseAudio.hpp
    PulseAudioWriter.hpp
    Pulse.hpp
)

set(PulseAudio_SRC
    PulseAudio.cpp
    PulseAudioWriter.cpp
    Pulse.cpp
)

set(PulseAudio_RESOURCES
    icon.qrc
)

pkg_check_modules(LIBPULSE libpulse-simple REQUIRED)
link_directories(${LIBPULSE_LIBRARY_DIRS})

if(USE_QT5)
    qt5_add_resources(PulseAudio_RESOURCES_RCC ${PulseAudio_RESOURCES})
else()
    qt4_add_resources(PulseAudio_RESOURCES_RCC ${PulseAudio_RESOURCES})
endif()

include_directories(../../qmplay2/headers ${LIBPULSE_INCLUDE_DIRS})

add_library(${PROJECT_NAME} MODULE
    ${PulseAudio_HDR}
    ${PulseAudio_SRC}
    ${PulseAudio_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtGui Qt4::QtCore)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    ${LIBPULSE_LIBRARIES}
    $<TARGET_FILE:qmplay2>
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
