cmake_minimum_required(VERSION 3.1)
project(OpenGL2)

set(OpenGL2_HDR
    OpenGL2.hpp
    OpenGL2Writer.hpp
    OpenGL2Common.hpp
    OpenGL2Window.hpp
    OpenGL2Widget.hpp
    Sphere.hpp
    Vertices.hpp
)

set(OpenGL2_SRC
    OpenGL2.cpp
    OpenGL2Writer.cpp
    OpenGL2Common.cpp
    OpenGL2Window.cpp
    OpenGL2Widget.cpp
    Sphere.cpp
)

set(OpenGL2_RESOURCES
    res.qrc
)

if(NOT WIN32)
    if(Qt5Gui_OPENGL_IMPLEMENTATION STREQUAL GLESv2)
        set(QT_USES_OPENGLES ON)
        add_definitions(-DOPENGL_ES2)
    elseif(Qt5Gui_OPENGL_IMPLEMENTATION STREQUAL GLES)
        message(SEND_ERROR "OpenGL|ES 1.0 is not supported!")
    endif()
endif()

include_directories(../../qmplay2/headers)

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${OpenGL2_HDR}
    ${OpenGL2_SRC}
    ${OpenGL2_RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    libqmplay2
)

if(APPLE)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})
elseif(WIN32)
    if(QT_USES_OPENGLES)
        target_link_libraries(${PROJECT_NAME} GLESv2)
    else()
        target_link_libraries(${PROJECT_NAME} opengl32)
    endif()
endif()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
