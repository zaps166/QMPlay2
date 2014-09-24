TEMPLATE = lib
CONFIG += plugin

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32: DESTDIR = ../../../app/modules
else: DESTDIR = ../../../app/share/qmplay2/modules

win32: QMAKE_LIBDIR += ../../../app
else: QMAKE_LIBDIR += ../../../app/lib
LIBS += -lqmplay2 -lavcodec -lavutil

OBJECTS_DIR = build/obj
MOC_DIR = build/moc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += Visualizations.hpp SimpleVis.hpp FFTSpectrum.hpp VisWidget.hpp
SOURCES += Visualizations.cpp SimpleVis.cpp FFTSpectrum.cpp VisWidget.cpp

DEFINES += __STDC_CONSTANT_MACROS
