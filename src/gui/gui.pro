TEMPLATE = app
!win32: CONFIG += link_pkgconfig

QT += network
greaterThan(QT_MAJOR_VERSION, 4) {
	lessThan(QT_VERSION, 5.7.0): CONFIG -= c++11
	QT += widgets
}

TARGET = QMPlay2

win32 {
	QMAKE_LIBDIR += ../../app
	DESTDIR = ../../app
} else {
	QMAKE_LIBDIR += ../../app/lib
	DESTDIR = ../../app/bin
	!android:!macx: LIBS += -lrt #For glibc < 2.17
}
LIBS += -lqmplay2

RESOURCES += resources.qrc
win32: RC_FILE = Windows/icons.rc

OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc
UI_DIR = build/ui

INCLUDEPATH += . ../qmplay2/headers
DEPENDPATH  += . ../qmplay2/headers

HEADERS += Main.hpp MenuBar.hpp MainWidget.hpp AddressBox.hpp VideoDock.hpp InfoDock.hpp PlaylistDock.hpp PlayClass.hpp DemuxerThr.hpp AVThread.hpp VideoThr.hpp AudioThr.hpp SettingsWidget.hpp OSDSettingsW.hpp DeintSettingsW.hpp OtherVFiltersW.hpp PlaylistWidget.hpp EntryProperties.hpp AboutWidget.hpp AddressDialog.hpp VideoAdjustment.hpp Appearance.hpp VolWidget.hpp ScreenSaver.hpp ShortcutHandler.hpp KeyBindingsDialog.hpp RepeatMode.hpp
SOURCES += Main.cpp MenuBar.cpp MainWidget.cpp AddressBox.cpp VideoDock.cpp InfoDock.cpp PlaylistDock.cpp PlayClass.cpp DemuxerThr.cpp AVThread.cpp VideoThr.cpp AudioThr.cpp SettingsWidget.cpp OSDSettingsW.cpp DeintSettingsW.cpp OtherVFiltersW.cpp PlaylistWidget.cpp EntryProperties.cpp AboutWidget.cpp AddressDialog.cpp VideoAdjustment.cpp Appearance.cpp VolWidget.cpp ScreenSaver.cpp ShortcutHandler.cpp KeyBindingsDialog.cpp
FORMS += Ui/SettingsGeneral.ui Ui/SettingsPlayback.ui Ui/SettingsPlaybackModulesList.ui Ui/OSDSettings.ui

!android {
	DEFINES += QMPlay2_TagEditor
	HEADERS += TagEditor.hpp
	SOURCES += TagEditor.cpp
}

win32 {
	DEFINES += UPDATER
	HEADERS += Updater.hpp
	SOURCES += Updater.cpp

	DEFINES += TAGLIB_STATIC TAGLIB_FULL_INCLUDE_PATH
	LIBS += -Wl,-Bstatic -ltag -Wl,-Bdynamic -lz
}
else {
	macx: QT_CONFIG -= no-pkg-config
	!android: PKGCONFIG += taglib
}
