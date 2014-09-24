TEMPLATE = lib
CONFIG += plugin

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DESTDIR = ../../../app/share/qmplay2/modules

QMAKE_LIBDIR += ../../../app/lib
LIBS += -lqmplay2 -lasound

OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += ALSA.hpp ALSACommon.hpp ALSAWriter.hpp
SOURCES += ALSA.cpp ALSACommon.cpp ALSAWriter.cpp
