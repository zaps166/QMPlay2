TEMPLATE = lib
CONFIG += plugin link_pkgconfig

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DESTDIR = ../../../app/share/qmplay2/modules
QMAKE_LIBDIR += ../../../app/lib
LIBS += -lqmplay2

RCC_DIR = build/rcc
OBJECTS_DIR = build/obj
MOC_DIR = build/moc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += OpenGLESWriter.hpp OpenGLES.hpp
SOURCES += OpenGLESWriter.cpp OpenGLES.cpp

PKGCONFIG += egl glesv2
