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

win32|android: LIBS += -lavcodec -lavutil
else {
	macx: QT_CONFIG -= no-pkg-config
	CONFIG += link_pkgconfig
	PKGCONFIG += libavcodec libavutil
}
LIBS += -lqmplay2

OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += AudioFilters.hpp Equalizer.hpp EqualizerGUI.hpp VoiceRemoval.hpp PhaseReverse.hpp Echo.hpp DysonCompressor.hpp BS2B.hpp
SOURCES += AudioFilters.cpp Equalizer.cpp EqualizerGUI.cpp VoiceRemoval.cpp PhaseReverse.cpp Echo.cpp DysonCompressor.cpp BS2B.cpp

HEADERS += bs2b/bs2b.h bs2b/bs2bversion.h
SOURCES += bs2b/bs2b.c

DEFINES += __STDC_CONSTANT_MACROS
