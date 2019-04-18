TEMPLATE = lib
CONFIG += plugin

QT += widgets qml

DESTDIR = ../../../app/lib/qmplay2/modules
QMAKE_LIBDIR += ../../../app/lib

LIBS += -lqmplay2

OBJECTS_DIR = build/obj
RCC_DIR = build/rcc
MOC_DIR = build/moc
UI_DIR = build/ui

RESOURCES += icons.qrc js.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH  += . ../../qmplay2/headers

HEADERS += Extensions.hpp YouTube.hpp Downloader.hpp Radio.hpp Radio/RadioBrowserModel.hpp
SOURCES += Extensions.cpp YouTube.cpp Downloader.cpp Radio.cpp Radio/RadioBrowserModel.cpp
FORMS += Radio/Radio.ui

HEADERS += MediaBrowser.hpp MediaBrowserJS.hpp
SOURCES += MediaBrowser.cpp MediaBrowserJS.cpp
DEFINES += USE_MEDIABROWSER

HEADERS += LastFM.hpp
SOURCES += LastFM.cpp
DEFINES += USE_LASTFM

HEADERS += Tekstowo.hpp
SOURCES += Tekstowo.cpp
DEFINES += USE_TEKSTOWO

unix:!android {
    QT += dbus
    HEADERS += MPRIS2.hpp
    SOURCES += MPRIS2.cpp
    DEFINES += USE_MPRIS2
}
