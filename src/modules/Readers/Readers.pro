TEMPLATE = lib
CONFIG += plugin

QT += network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32: DESTDIR = ../../../app/modules
else: DESTDIR = ../../../app/share/qmplay2/modules

win32: QMAKE_LIBDIR += ../../../app
else: QMAKE_LIBDIR += ../../../app/lib
LIBS += -lqmplay2

OBJECTS_DIR = build/obj
MOC_DIR = build/moc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += FileReader.hpp FileWriter.hpp HttpReader.hpp Readers.hpp
SOURCES += FileReader.cpp FileWriter.cpp HttpReader.cpp Readers.cpp
