TEMPLATE = lib
CONFIG += plugin

QT += widgets

DESTDIR = ../../../app/lib/qmplay2/modules
QMAKE_LIBDIR += ../../../app/lib

LIBS += -lqmplay2

OBJECTS_DIR = build/obj
RCC_DIR = build/rcc
MOC_DIR = build/moc
UI_DIR = build/ui

RESOURCES += icons.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH  += . ../../qmplay2/headers

HEADERS += Extensions.hpp YouTube.hpp Downloader.hpp Radio.hpp Radio/RadioBrowserModel.hpp
SOURCES += Extensions.cpp YouTube.cpp Downloader.cpp Radio.cpp Radio/RadioBrowserModel.cpp
FORMS += Radio/Radio.ui

HEADERS += MediaBrowser.hpp MediaBrowser/Common.hpp MediaBrowser/MyFreeMP3.hpp MediaBrowser/AnimeOdcinki.hpp MediaBrowser/Wbijam.hpp
SOURCES += MediaBrowser.cpp MediaBrowser/Common.cpp MediaBrowser/MyFreeMP3.cpp MediaBrowser/AnimeOdcinki.cpp MediaBrowser/Wbijam.cpp
DEFINES += USE_MEDIABROWSER USE_MYFREEMP3 USE_ANIMEODCINKI USE_WBIJAM

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
