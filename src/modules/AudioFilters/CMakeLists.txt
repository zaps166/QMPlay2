cmake_minimum_required(VERSION 2.8.11)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif()
endif()
project(AudioFilters)

set(AudioFilters_HDR
    AudioFilters.hpp
    Equalizer.hpp
    EqualizerGUI.hpp
    VoiceRemoval.hpp
    PhaseReverse.hpp
    Echo.hpp
)

set(AudioFilters_SRC
    AudioFilters.cpp
    Equalizer.cpp
    EqualizerGUI.cpp
    VoiceRemoval.cpp
    PhaseReverse.cpp
    Echo.cpp
)

set(AudioFilters_RESOURCES
    icons.qrc
)

add_definitions(-D__STDC_CONSTANT_MACROS)

pkg_check_modules(LIBAVCODEC libavcodec>=55.52.102 REQUIRED)
pkg_check_modules(LIBAVUTIL libavutil>=52.66.100 REQUIRED)
link_directories(${LIBAVCODEC_LIBRARY_DIRS} ${LIBAVUTIL_LIBRARY_DIRS})

if(USE_QT5)
    qt5_add_resources(AudioFilters_RESOURCES_RCC ${AudioFilters_RESOURCES})
else()
    qt4_add_resources(AudioFilters_RESOURCES_RCC ${AudioFilters_RESOURCES})
endif()

include_directories(../../qmplay2/headers ${LIBAVCODEC_INCLUDE_DIRS} ${LIBAVUTIL_INCLUDE_DIRS})

add_library(${PROJECT_NAME} MODULE
    ${AudioFilters_HDR}
    ${AudioFilters_SRC}
    ${AudioFilters_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    ${LIBAVCODEC_LIBRARIES}
    ${LIBAVUTIL_LIBRARIES}
    $<TARGET_FILE:qmplay2>
)

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
endif()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
