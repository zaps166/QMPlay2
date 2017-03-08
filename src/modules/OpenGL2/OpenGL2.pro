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

HEADERS += OpenGL2.hpp OpenGL2Writer.hpp OpenGL2Common.hpp Sphere.hpp Vertices.hpp
SOURCES += OpenGL2.cpp OpenGL2Writer.cpp OpenGL2Common.cpp Sphere.cpp

equals(QT_VERSION, 5.6.0)|greaterThan(QT_VERSION, 5.6.0) {
	DEFINES += OPENGL_NEW_API VSYNC_SETTINGS
	HEADERS += OpenGL2Window.hpp OpenGL2Widget.hpp OpenGL2CommonQt5.hpp
	SOURCES += OpenGL2Window.cpp OpenGL2Widget.cpp OpenGL2CommonQt5.cpp
} else {
	QT += opengl
	DEFINES += DONT_RECREATE_SHADERS
	HEADERS += OpenGL2OldWidget.hpp
	SOURCES += OpenGL2OldWidget.cpp
	unix:!android:!contains(QT_CONFIG, opengles2): DEFINES += VSYNC_SETTINGS
}

contains(QT_CONFIG, opengles2): DEFINES += OPENGL_ES2
