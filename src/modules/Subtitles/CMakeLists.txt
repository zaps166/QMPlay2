cmake_minimum_required(VERSION 2.8.6)
if(COMMAND cmake_policy)
    if(POLICY CMP0003)
        cmake_policy(SET CMP0003 NEW)
    endif()
    if(POLICY CMP0020)
        cmake_policy(SET CMP0020 NEW)
    endif()
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif()
endif()
project(Subtitles)

set(Subtitles_HDR
    SRT.hpp
    Classic.hpp
    Subtitles.hpp
)

set(Subtitles_SRC
    SRT.cpp
    Classic.cpp
    Subtitles.cpp
)

set(Subtitles_RESOURCES
    icon.qrc
)

include_directories(../../qmplay2/headers)

if(USE_QT5)
    qt5_add_resources(Subtitles_RESOURCES_RCC ${Subtitles_RESOURCES})
else()
    qt4_add_resources(Subtitles_RESOURCES_RCC ${Subtitles_RESOURCES})
endif()

add_library(${PROJECT_NAME} MODULE
    ${Subtitles_HDR}
    ${Subtitles_SRC}
    ${Subtitles_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    ${libqmplay2}
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
