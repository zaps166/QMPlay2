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

HEADERS += ProstoPleer.hpp
SOURCES += ProstoPleer.cpp
DEFINES += USE_PROSTOPLEER

HEADERS += LastFM.hpp
SOURCES += LastFM.cpp
DEFINES += USE_LASTFM

greaterThan(QT_MAJOR_VERSION, 4) {
        HEADERS += SoundCloud.hpp
        SOURCES += SoundCloud.cpp
        DEFINES += USE_SoundCloud
}

unix:!android {
	QT += dbus
	HEADERS += MPRIS2.hpp
	SOURCES += MPRIS2.cpp
	DEFINES += USE_MPRIS2
}
