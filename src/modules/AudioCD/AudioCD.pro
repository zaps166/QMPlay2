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

win32: LIBS += -Wl,-Bstatic -lcdio -lcddb -lregex -Wl,-Bdynamic -lwinmm -lws2_32
else {
	CONFIG += link_pkgconfig
	PKGCONFIG += libcdio libcddb
}
LIBS += -lqmplay2

OBJECTS_DIR = build/obj
RCC_DIR = build/rcc
MOC_DIR = build/moc

RESOURCES += icons.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += AudioCD.hpp AudioCDDemux.hpp
SOURCES += AudioCD.cpp AudioCDDemux.cpp
