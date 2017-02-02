TEMPLATE = lib
CONFIG += plugin

greaterThan(QT_MAJOR_VERSION, 4) {
	lessThan(QT_VERSION, 5.7.0): CONFIG -= c++11
	QT += widgets
}

win32 {
	DESTDIR = ../../../app/modules
	QMAKE_LIBDIR += ../../../app
} else {
	DESTDIR = ../../../app/lib/qmplay2/modules
	QMAKE_LIBDIR += ../../../app/lib
}

win32|android: LIBS += -lavcodec -lswscale -lavutil
else {
	CONFIG += link_pkgconfig
	PKGCONFIG += libavcodec libswscale libavutil
}
LIBS += -lqmplay2

DEFINES += __STDC_CONSTANT_MACROS __STDC_LIMIT_MACROS

RCC_DIR = build/rcc
OBJECTS_DIR = build/obj
MOC_DIR = build/moc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += Cuvid.hpp CuvidDec.hpp
SOURCES += Cuvid.cpp CuvidDec.cpp
