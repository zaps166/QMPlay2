TEMPLATE = lib
CONFIG += plugin

QT += widgets

DESTDIR = ../../../app/lib/qmplay2/modules
QMAKE_LIBDIR += ../../../app/lib

LIBS += -lqmplay2

OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += Playlists.hpp PLS.hpp M3U.hpp XSPF.hpp
SOURCES += Playlists.cpp PLS.cpp M3U.cpp XSPF.cpp
