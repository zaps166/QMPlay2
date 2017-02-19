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

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH  += . ../../qmplay2/headers

HEADERS += Notifies.hpp NotifyExtension.hpp TrayNotify.hpp Notify.hpp
SOURCES += Notifies.cpp NotifyExtension.cpp TrayNotify.cpp

 unix:!android {
	QT += dbus
	DBUS_INTERFACES += org.freedesktop.Notifications.xml
	HEADERS += FreedesktopNotify.hpp
	SOURCES += FreedesktopNotify.cpp
	DEFINES += FREEDESKTOP_NOTIFY
 }
