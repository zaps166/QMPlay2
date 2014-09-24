TEMPLATE = lib
CONFIG += plugin

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32: DESTDIR = ../../../app/modules
else: DESTDIR = ../../../app/share/qmplay2/modules

win32: QMAKE_LIBDIR += ../../../app
else: QMAKE_LIBDIR += ../../../app/lib
LIBS += -lqmplay2

OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc

RESOURCES += icons.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += VFilters.hpp BobDeint.hpp BlendDeint.hpp DiscardDeint.hpp MotionBlur.hpp
SOURCES += VFilters.cpp BobDeint.cpp BlendDeint.cpp DiscardDeint.cpp MotionBlur.cpp
