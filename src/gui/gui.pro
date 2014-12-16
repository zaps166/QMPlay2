TEMPLATE = app
CONFIG -= app_bundle

QT += network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QMPlay2

win32: DESTDIR = ../../app
else: DESTDIR = ../../app/bin

win32: QMAKE_LIBDIR += ../../app
else: {
	QMAKE_LIBDIR += ../../app/lib
	LIBS += -lrt
}
unix:!macx: LIBS += -lX11
LIBS += -lqmplay2

RESOURCES += resources.qrc
win32 {
	RC_FILE = Windows/icons.rc
	DEFINES -= UNICODE
}

OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc

INCLUDEPATH += . ../qmplay2/headers
DEPENDPATH  += . ../qmplay2/headers

HEADERS += Main.hpp MenuBar.hpp MainWidget.hpp AddressBox.hpp VideoDock.hpp InfoDock.hpp PlaylistDock.hpp PlayClass.hpp DemuxerThr.hpp AVThread.hpp VideoThr.hpp AudioThr.hpp SettingsWidget.hpp OSDSettingsW.hpp DeintSettingsW.hpp OtherVFiltersW.hpp PlaylistWidget.hpp EntryProperties.hpp AboutWidget.hpp AddressDialog.hpp VideoEqualizer.hpp Updater.hpp Appearance.hpp PacketBuffer.hpp
SOURCES += Main.cpp MenuBar.cpp MainWidget.cpp AddressBox.cpp VideoDock.cpp InfoDock.cpp PlaylistDock.cpp PlayClass.cpp DemuxerThr.cpp AVThread.cpp VideoThr.cpp AudioThr.cpp SettingsWidget.cpp OSDSettingsW.cpp DeintSettingsW.cpp OtherVFiltersW.cpp PlaylistWidget.cpp EntryProperties.cpp AboutWidget.cpp AddressDialog.cpp VideoEqualizer.cpp Updater.cpp Appearance.cpp PacketBuffer.cpp

DEFINES += QMPlay2_TagEditor
HEADERS += TagEditor.hpp
SOURCES += TagEditor.cpp
win32 {
	DEFINES += TAGLIB_STATIC
	LIBS += -Wl,-Bstatic -ltag -Wl,-Bdynamic -lz
}
else: LIBS += -ltag
