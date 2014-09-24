TEMPLATE = lib
CONFIG += plugin

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DESTDIR = ../../../app/modules

QMAKE_LIBDIR += ../../../app
LIBS += -lddraw -ldxguid -lqmplay2

RCC_DIR = build/rcc
OBJECTS_DIR = build/obj
MOC_DIR = build/moc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += DirectDraw.hpp DirectX.hpp
SOURCES += DirectDraw.cpp DirectX.cpp
