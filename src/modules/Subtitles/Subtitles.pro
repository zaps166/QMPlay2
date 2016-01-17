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

OBJECTS_DIR = build/obj
MOC_DIR = build/moc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += SRT.hpp Classic.hpp Subtitles.hpp
SOURCES += SRT.cpp Classic.cpp Subtitles.cpp
