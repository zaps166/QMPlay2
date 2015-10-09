TEMPLATE = lib
CONFIG += plugin

QT += opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

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

HEADERS += OpenGL2Writer.hpp OpenGL2.hpp
SOURCES += OpenGL2Writer.cpp OpenGL2.cpp

win32|unix:!macx:!android:!contains(QT_CONFIG, opengles2): DEFINES += VSYNC_SETTINGS
contains(QT_CONFIG, opengles2): DEFINES += OPENGL_ES2
