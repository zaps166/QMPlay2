TEMPLATE = lib
CONFIG += plugin link_pkgconfig

QT += widgets

DESTDIR = ../../../app/lib/qmplay2/modules

QMAKE_LIBDIR += ../../../app/lib
LIBS += -lqmplay2
PKGCONFIG += alsa

OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += ALSA.hpp ALSACommon.hpp ALSAWriter.hpp
SOURCES += ALSA.cpp ALSACommon.cpp ALSAWriter.cpp
