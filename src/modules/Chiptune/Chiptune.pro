TEMPLATE = lib
CONFIG += plugin

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32|macx {
	DESTDIR = ../../../app/modules
	QMAKE_LIBDIR += ../../../app
}
else {
	DESTDIR = ../../../app/share/qmplay2/modules
	QMAKE_LIBDIR += ../../../app/lib
}

win32 {
	LIBS += -Wl,-Bstatic -lsidplayfp -Wl,-Bdynamic
	DEFINES += USE_SIDPLAY USE_GME
} else {
	macx: QT_CONFIG -= no-pkg-config
	CONFIG += link_pkgconfig
	packagesExist(libsidplayfp) {
		PKGCONFIG += libsidplayfp
		DEFINES += USE_SIDPLAY
	} else {
		message("SID will not be compiled, because libsidplayfp doesn't exist")
	}
	packagesExist(libgme) {
		PKGCONFIG += libgme
		DEFINES += USE_GME
	} else {
		message("Game-Music-Emu will not be compiled, because libgme doesn't exist")
	}
}
LIBS += -lqmplay2

OBJECTS_DIR = build/obj
RCC_DIR = build/rcc
MOC_DIR = build/moc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += Chiptune.hpp
SOURCES += Chiptune.cpp

contains(DEFINES, USE_SIDPLAY) {
	HEADERS += SIDPlay.hpp
	SOURCES += SIDPlay.cpp
}
contains(DEFINES, USE_GME) {
	HEADERS += GME.hpp
	SOURCES += GME.cpp
}
