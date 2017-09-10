TEMPLATE = lib
CONFIG += plugin

QT += widgets

DESTDIR = ../../../app/lib/qmplay2/modules
QMAKE_LIBDIR += ../../../app/lib

LIBS += -lqmplay2

RCC_DIR = build/rcc
OBJECTS_DIR = build/obj
MOC_DIR = build/moc

RESOURCES += resources.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += OpenGL2.hpp OpenGL2Writer.hpp OpenGL2Common.hpp OpenGL2Window.hpp OpenGL2Widget.hpp Sphere.hpp Vertices.hpp
SOURCES += OpenGL2.cpp OpenGL2Writer.cpp OpenGL2Common.cpp OpenGL2Window.cpp OpenGL2Widget.cpp Sphere.cpp

contains(QT_CONFIG, opengles2): DEFINES += OPENGL_ES2
