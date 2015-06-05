TEMPLATE = lib
CONFIG += plugin

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32: DESTDIR = ../../../app/modules
else: DESTDIR = ../../../app/share/qmplay2/modules

win32: QMAKE_LIBDIR += ../../../app
else: QMAKE_LIBDIR += ../../../app/lib
LIBS += -lqmplay2
win32: LIBS += -Wl,-Bstatic -lcdio -lcddb -lregex -Wl,-Bdynamic -lwinmm -lws2_32
else {
	CONFIG += link_pkgconfig
	PKGCONFIG += libcdio libcddb
}

OBJECTS_DIR = build/obj
RCC_DIR = build/rcc
MOC_DIR = build/moc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += AudioCD.hpp AudioCDDemux.hpp
SOURCES += AudioCD.cpp AudioCDDemux.cpp
