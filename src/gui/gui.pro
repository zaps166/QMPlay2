TEMPLATE = app
CONFIG += link_pkgconfig

QT += widgets

android {
	QT += svg androidextras
	QTPLUGIN += svg
}

TARGET = QMPlay2

QMAKE_LIBDIR += ../../app/lib
DESTDIR = ../../app/bin
!android: LIBS += -lrt #For glibc < 2.17
LIBS += -lqmplay2

RESOURCES += resources.qrc

OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc
UI_DIR = build/ui

INCLUDEPATH += . ../qmplay2/headers
DEPENDPATH  += . ../qmplay2/headers

HEADERS += Main.hpp MenuBar.hpp MainWidget.hpp AddressBox.hpp VideoDock.hpp InfoDock.hpp PlaylistDock.hpp PlayClass.hpp DemuxerThr.hpp AVThread.hpp VideoThr.hpp AudioThr.hpp SettingsWidget.hpp OSDSettingsW.hpp DeintSettingsW.hpp OtherVFiltersW.hpp PlaylistWidget.hpp EntryProperties.hpp AboutWidget.hpp AddressDialog.hpp VideoAdjustmentW.hpp Appearance.hpp VolWidget.hpp Updater.hpp ShortcutHandler.hpp KeyBindingsDialog.hpp PanGestureEventFilter.hpp EventFilterWorkarounds.hpp ScreenSaver.hpp RepeatMode.hpp
SOURCES += Main.cpp MenuBar.cpp MainWidget.cpp AddressBox.cpp VideoDock.cpp InfoDock.cpp PlaylistDock.cpp PlayClass.cpp DemuxerThr.cpp AVThread.cpp VideoThr.cpp AudioThr.cpp SettingsWidget.cpp OSDSettingsW.cpp DeintSettingsW.cpp OtherVFiltersW.cpp PlaylistWidget.cpp EntryProperties.cpp AboutWidget.cpp AddressDialog.cpp VideoAdjustmentW.cpp Appearance.cpp VolWidget.cpp Updater.cpp ShortcutHandler.cpp KeyBindingsDialog.cpp PanGestureEventFilter.cpp EventFilterWorkarounds.cpp
FORMS += Ui/SettingsGeneral.ui Ui/SettingsPlayback.ui Ui/SettingsPlaybackModulesList.ui Ui/OSDSettings.ui

!android {
	SOURCES += Unix/ScreenSaver.cpp

	DEFINES += QMPLAY2_ALLOW_ONLY_ONE_INSTANCE

	DEFINES += QMPlay2_TagEditor
	HEADERS += TagEditor.hpp
	SOURCES += TagEditor.cpp
} else {
	SOURCES += Android/ScreenSaver.cpp
}

!android: PKGCONFIG += taglib
