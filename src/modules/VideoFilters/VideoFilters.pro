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

win32|android: LIBS += -lavutil
else {
	CONFIG += link_pkgconfig
	PKGCONFIG += libavutil
}
LIBS += -lqmplay2

OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += VFilters.hpp BobDeint.hpp BlendDeint.hpp DiscardDeint.hpp MotionBlur.hpp YadifDeint.hpp
SOURCES += VFilters.cpp BobDeint.cpp BlendDeint.cpp DiscardDeint.cpp MotionBlur.cpp YadifDeint.cpp
