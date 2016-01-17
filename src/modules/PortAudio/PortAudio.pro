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

win32: LIBS += -Wl,-Bstatic -lportaudio -Wl,-Bdynamic -lwinmm -luuid
else {
	macx: QT_CONFIG -= no-pkg-config
	CONFIG += link_pkgconfig
	PKGCONFIG += portaudio-2.0
}
LIBS += -lqmplay2

OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += PortAudio.hpp PortAudioCommon.hpp PortAudioWriter.hpp
SOURCES += PortAudio.cpp PortAudioCommon.cpp PortAudioWriter.cpp
