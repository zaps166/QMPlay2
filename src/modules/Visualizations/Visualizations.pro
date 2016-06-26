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

win32|android: LIBS += -lavcodec -lavutil
else {
	macx: QT_CONFIG -= no-pkg-config
	CONFIG += link_pkgconfig
	PKGCONFIG += libavcodec libavutil
}
LIBS += -lqmplay2

OBJECTS_DIR = build/obj
MOC_DIR = build/moc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += Visualizations.hpp SimpleVis.hpp FFTSpectrum.hpp VisWidget.hpp
SOURCES += Visualizations.cpp SimpleVis.cpp FFTSpectrum.cpp VisWidget.cpp

DEFINES += __STDC_CONSTANT_MACROS
