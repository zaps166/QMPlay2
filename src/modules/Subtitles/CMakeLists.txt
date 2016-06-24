cmake_minimum_required(VERSION 2.8.11)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
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

include_directories(../../qmplay2/headers)

add_library(${PROJECT_NAME} MODULE
    ${Subtitles_HDR}
    ${Subtitles_SRC}
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
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
endif()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
