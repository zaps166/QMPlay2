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

win32: LIBS += -lavcodec -lavutil
else {
	macx: QT_CONFIG -= no-pkg-config
	CONFIG += link_pkgconfig
	PKGCONFIG += libavcodec libavutil
}
LIBS += -lqmplay2

OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc

RESOURCES += icons.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += AudioFilters.hpp Equalizer.hpp EqualizerGUI.hpp VoiceRemoval.hpp PhaseReverse.hpp Echo.hpp
SOURCES += AudioFilters.cpp Equalizer.cpp EqualizerGUI.cpp VoiceRemoval.cpp PhaseReverse.cpp Echo.cpp

DEFINES += __STDC_CONSTANT_MACROS
