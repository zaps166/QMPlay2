TEMPLATE = lib
CONFIG += plugin

greaterThan(QT_MAJOR_VERSION, 4) {
	CONFIG -= c++11
	QT += widgets
}

win32|macx {
	DESTDIR = ../../../app/modules
	QMAKE_LIBDIR += ../../../app
}
else {
	DESTDIR = ../../../app/share/qmplay2/modules
	QMAKE_LIBDIR += ../../../app/lib
}
LIBS += -lqmplay2

RCC_DIR = build/rcc
OBJECTS_DIR = build/obj
MOC_DIR = build/moc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += OpenGL2.hpp OpenGL2Writer.hpp OpenGL2Common.hpp
SOURCES += OpenGL2.cpp OpenGL2Writer.cpp OpenGL2Common.cpp

greaterThan(QT_VERSION, 5.5.2) { #At least Qt 5.6.0
	DEFINES += OPENGL_NEW_API VSYNC_SETTINGS
	HEADERS += OpenGL2Window.hpp OpenGL2Widget.hpp OpenGL2CommonQt5.hpp
	SOURCES += OpenGL2Window.cpp OpenGL2Widget.cpp OpenGL2CommonQt5.cpp
} else {
	QT += opengl
	HEADERS += OpenGL2OldWidget.hpp
	SOURCES += OpenGL2OldWidget.cpp
	win32|unix:!macx:!android:!contains(QT_CONFIG, opengles2): DEFINES += VSYNC_SETTINGS
}

contains(QT_CONFIG, opengles2): DEFINES += OPENGL_ES2
