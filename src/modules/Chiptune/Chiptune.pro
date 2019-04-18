TEMPLATE = lib
CONFIG += plugin

QT += widgets

DESTDIR = ../../../app/lib/qmplay2/modules
QMAKE_LIBDIR += ../../../app/lib

CONFIG += link_pkgconfig
packagesExist(libgme) {
    PKGCONFIG += libgme
    DEFINES += USE_GME
} else {
    #Some distributions (e.g. Ubuntu) doesn't provide pkg-config for libgme
    exists("/usr/include/gme") {
        LIBS += -lgme
        DEFINES += USE_GME
    }
    else {
        message("Game-Music-Emu will not be compiled, because libgme doesn't exist")
    }
}
packagesExist(libsidplayfp) {
    PKGCONFIG += libsidplayfp
    DEFINES += USE_SIDPLAY
} else {
    message("SID will not be compiled, because libsidplayfp doesn't exist")
}

LIBS += -lqmplay2

OBJECTS_DIR = build/obj
RCC_DIR = build/rcc
MOC_DIR = build/moc

RESOURCES += icons.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += Chiptune.hpp Common.hpp
SOURCES += Chiptune.cpp Common.cpp

contains(DEFINES, USE_SIDPLAY) {
    HEADERS += SIDPlay.hpp
    SOURCES += SIDPlay.cpp
}
contains(DEFINES, USE_GME) {
    HEADERS += GME.hpp
    SOURCES += GME.cpp
}
