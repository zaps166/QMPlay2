TEMPLATE = lib
CONFIG += plugin

DEFINES += USE_OPENGL
QT += widgets

DESTDIR = ../../../app/lib/qmplay2/modules
QMAKE_LIBDIR += ../../../app/lib

android: LIBS += -lavcodec -lavutil
else {
    CONFIG += link_pkgconfig
    PKGCONFIG += libavcodec libavutil
}
LIBS += -lqmplay2

OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += Visualizations.hpp SimpleVis.hpp FFTSpectrum.hpp VisWidget.hpp
SOURCES += Visualizations.cpp SimpleVis.cpp FFTSpectrum.cpp VisWidget.cpp

DEFINES += __STDC_CONSTANT_MACROS
