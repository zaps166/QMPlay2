TEMPLATE = lib
CONFIG += plugin

QT += widgets

DESTDIR = ../../../app/lib/qmplay2/modules
QMAKE_LIBDIR += ../../../app/lib

CONFIG += link_pkgconfig
PKGCONFIG += portaudio-2.0

LIBS += -lqmplay2

OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += PortAudio.hpp PortAudioCommon.hpp PortAudioWriter.hpp
SOURCES += PortAudio.cpp PortAudioCommon.cpp PortAudioWriter.cpp
