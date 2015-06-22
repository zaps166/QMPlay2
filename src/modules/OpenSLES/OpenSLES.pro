TEMPLATE = lib
CONFIG += plugin

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = OpenSL_ES #OpenSLES conflicts with system library

DESTDIR = ../../../app/share/qmplay2/modules

QMAKE_LIBDIR += ../../../app/lib
LIBS += -lqmplay2 -lOpenSLES

OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS +=  OpenSLES.hpp OpenSLESWriter.hpp
SOURCES +=  OpenSLES.cpp OpenSLESWriter.cpp
