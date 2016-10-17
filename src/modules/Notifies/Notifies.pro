TEMPLATE = lib
CONFIG += plugin

greaterThan(QT_MAJOR_VERSION, 4) {
	lessThan(QT_VERSION, 5.7.0): CONFIG -= c++11
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

# RESOURCES += icons.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH  += . ../../qmplay2/headers

HEADERS += Notifies.hpp NotifyExtension.hpp TrayNotify.hpp Notify.hpp
SOURCES += Notifies.cpp NotifyExtension.cpp TrayNotify.cpp

unix:!macx:!android {
		QT += dbus
		HEADERS += X11Notify.hpp
		SOURCES += X11Notify.cpp
}
