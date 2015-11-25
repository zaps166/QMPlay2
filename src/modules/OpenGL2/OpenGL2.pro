TEMPLATE = lib
CONFIG += plugin

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

#If you want to use QOpenGLWidget instead of QGLWidget (only Qt5, recommended for Wayland), replace first ":" by "|".
#It is not recommended for Windows and X11, but it works quite good since Qt 5.6.
greaterThan(QT_MAJOR_VERSION, 4):android: {
	DEFINES += USE_NEW_OPENGL_API
	DEFINES += QGLWidget=QOpenGLWidget QGLShaderProgram=QOpenGLShaderProgram QGLShader=QOpenGLShader
} else {
	QT += opengl
	win32|unix:!macx:!android:!contains(QT_CONFIG, opengles2): DEFINES += VSYNC_SETTINGS
}
contains(QT_CONFIG, opengles2): DEFINES += OPENGL_ES2
