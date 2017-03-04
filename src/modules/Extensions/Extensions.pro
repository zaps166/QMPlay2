TEMPLATE = lib
CONFIG += plugin

greaterThan(QT_MAJOR_VERSION, 4) {
	QT += widgets
}

win32 {
	DESTDIR = ../../../app/modules
	QMAKE_LIBDIR += ../../../app
} else {
	DESTDIR = ../../../app/lib/qmplay2/modules
	QMAKE_LIBDIR += ../../../app/lib
}
LIBS += -lqmplay2

OBJECTS_DIR = build/obj
RCC_DIR = build/rcc
MOC_DIR = build/moc

RESOURCES += icons.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH  += . ../../qmplay2/headers

HEADERS += Extensions.hpp YouTube.hpp Downloader.hpp Radio.hpp
SOURCES += Extensions.cpp YouTube.cpp Downloader.cpp Radio.cpp

HEADERS += MediaBrowser.hpp MediaBrowser/Common.hpp MediaBrowser/ProstoPleer.hpp MediaBrowser/SoundCloud.hpp MediaBrowser/AnimeOdcinki.hpp
SOURCES += MediaBrowser.cpp MediaBrowser/Common.cpp MediaBrowser/ProstoPleer.cpp MediaBrowser/SoundCloud.cpp MediaBrowser/AnimeOdcinki.hpp
DEFINES += USE_MEDIABROWSER USE_PROSTOPLEER USE_SOUNDCLOUD USE_ANIMEODCINKI

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
