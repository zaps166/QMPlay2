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
RCC_DIR = build/rcc
MOC_DIR = build/moc

RESOURCES += icons.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += Inputs.hpp ToneGenerator.hpp PCM.hpp Rayman2.hpp
SOURCES += Inputs.cpp ToneGenerator.cpp PCM.cpp Rayman2.cpp
