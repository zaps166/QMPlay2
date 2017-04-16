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
project(OpenGL2)

set(OpenGL2_HDR
    OpenGL2.hpp
    OpenGL2Writer.hpp
    OpenGL2Common.hpp
    Sphere.hpp
    Vertices.hpp
)

set(OpenGL2_SRC
    OpenGL2.cpp
    OpenGL2Writer.cpp
    OpenGL2Common.cpp
    Sphere.cpp
)

set(OpenGL2_RESOURCES
    resources.qrc
)

function(gles1Error)
    message(SEND_ERROR "OpenGL|ES 1.0 is not supported!")
endfunction()

if(USE_QT5)
    if(Qt5Gui_OPENGL_IMPLEMENTATION STREQUAL GLESv2)
        set(QT_USES_OPENGLES ON)
        add_definitions(-DOPENGL_ES2)
    elseif(Qt5Gui_OPENGL_IMPLEMENTATION STREQUAL GLES)
        gles1Error()
    endif()
else()
    find_package(Qt4 REQUIRED QtOpenGL)
    include(${QT_USE_FILE})
    if(${QT_QCONFIG} MATCHES "opengles2")
        set(QT_USES_OPENGLES ON)
        add_definitions(-DOPENGL_ES2)
    elseif(${QT_QCONFIG} MATCHES "opengles1")
        gles1Error()
    endif()
endif()

if(NOT USE_QT5 OR Qt5Widgets_VERSION LESS 5.6.0)
    add_definitions(-DDONT_RECREATE_SHADERS)
    list(APPEND OpenGL2_HDR OpenGL2OldWidget.hpp)
    list(APPEND OpenGL2_SRC OpenGL2OldWidget.cpp)
    if(NOT QT_USES_OPENGLES)
        add_definitions(-DVSYNC_SETTINGS)
    endif()
else()
    add_definitions(-DOPENGL_NEW_API -DVSYNC_SETTINGS)
    list(APPEND OpenGL2_HDR OpenGL2Window.hpp OpenGL2Widget.hpp OpenGL2CommonQt5.hpp)
    list(APPEND OpenGL2_SRC OpenGL2Window.cpp OpenGL2Widget.cpp OpenGL2CommonQt5.cpp)
endif()

if(USE_QT5)
    qt5_add_resources(OpenGL2_RESOURCES_RCC ${OpenGL2_RESOURCES})
else()
    qt4_add_resources(OpenGL2_RESOURCES_RCC ${OpenGL2_RESOURCES})
endif()

include_directories(../../qmplay2/headers)

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${OpenGL2_HDR}
    ${OpenGL2_SRC}
    ${OpenGL2_RESOURCES_RCC}
)

if(NOT USE_QT5) # Qt4
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui Qt4::QtOpenGL)
elseif(Qt5Widgets_VERSION LESS 5.6.0) # Qt5 < 5.6.0
    qt5_use_modules(${PROJECT_NAME} Gui Widgets OpenGL)
else() # Qt5 >= 5.6.0
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
endif()

add_dependencies(${PROJECT_NAME} libqmplay2)
target_link_libraries(${PROJECT_NAME}
    ${qmplay2lib}
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
