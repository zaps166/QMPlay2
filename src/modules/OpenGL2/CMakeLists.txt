project(OpenGL2)
cmake_minimum_required(VERSION 3.0.2)

set(OpenGL2_HDR
    OpenGL2.hpp
    OpenGL2Writer.hpp
    OpenGL2Common.hpp
    Sphere.hpp
    Shaders.hpp
    Vertices.hpp
)

set(OpenGL2_SRC
    OpenGL2.cpp
    OpenGL2Writer.cpp
    OpenGL2Common.cpp
    Sphere.cpp
)

set(OpenGL2_RESOURCES
    icon.qrc
)

if(NOT USE_QT5 OR Qt5Widgets_VERSION LESS 5.6.0)
    add_definitions(-DDONT_RECREATE_SHADERS)
    list(APPEND OpenGL2_HDR OpenGL2OldWidget.hpp)
    list(APPEND OpenGL2_SRC OpenGL2OldWidget.cpp)

    # add check for opengles2 in QT_CONFIG
    add_definitions(-DVSYNC_SETTINGS)
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

add_library(${PROJECT_NAME} MODULE
    ${OpenGL2_HDR}
    ${OpenGL2_SRC}
    ${OpenGL2_RESOURCES_RCC}
)

if(NOT USE_QT5) # Qt4
    target_link_libraries(${PROJECT_NAME} Qt4::QtGui Qt4::QtCore Qt4::QtOpenGL)
elseif(Qt5Widgets_VERSION LESS 5.6.0) # Qt5 < 5.6.0
    qt5_use_modules(${PROJECT_NAME} Gui Widgets OpenGL)
else() # Qt5 >= 5.6.0
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    $<TARGET_FILE:qmplay2>
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
