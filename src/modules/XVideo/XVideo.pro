TEMPLATE = lib
CONFIG += plugin

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets x11extras

DESTDIR = ../../../app/share/qmplay2/modules

QMAKE_LIBDIR += ../../../app/lib
LIBS += -lqmplay2 -lXv

RCC_DIR = build/rcc
OBJECTS_DIR = build/obj
MOC_DIR = build/moc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += XVideoWriter.hpp XVideo.hpp xv.hpp
SOURCES += XVideoWriter.cpp XVideo.cpp xv.cpp
