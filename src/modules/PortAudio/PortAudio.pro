TEMPLATE = lib
CONFIG += plugin

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32: DESTDIR = ../../../app/modules
else: DESTDIR = ../../../app/share/qmplay2/modules

win32: QMAKE_LIBDIR += ../../../app
else: QMAKE_LIBDIR += ../../../app/lib
LIBS += -lqmplay2 -lportaudio
win32: LIBS += -lwinmm -luuid

OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += PortAudio.hpp PortAudioCommon.hpp PortAudioWriter.hpp
SOURCES += PortAudio.cpp PortAudioCommon.cpp PortAudioWriter.cpp
