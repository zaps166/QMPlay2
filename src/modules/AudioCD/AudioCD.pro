TEMPLATE = lib
CONFIG += plugin

QT += widgets

DESTDIR = ../../../app/lib/qmplay2/modules
QMAKE_LIBDIR += ../../../app/lib

CONFIG += link_pkgconfig
PKGCONFIG += libcdio libcddb
LIBS += -lqmplay2

OBJECTS_DIR = build/obj
RCC_DIR = build/rcc
MOC_DIR = build/moc

RESOURCES += icons.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += AudioCD.hpp AudioCDDemux.hpp
SOURCES += AudioCD.cpp AudioCDDemux.cpp
