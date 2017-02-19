TEMPLATE = lib
CONFIG += plugin

greaterThan(QT_MAJOR_VERSION, 4) {
	QT += widgets
}

DESTDIR = ../../../app/modules

QMAKE_LIBDIR += ../../../app
LIBS += -lqmplay2 -lshlwapi

OBJECTS_DIR = build/obj
RCC_DIR = build/rcc
MOC_DIR = build/moc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += FileAssociation.hpp
SOURCES += FileAssociation.cpp
