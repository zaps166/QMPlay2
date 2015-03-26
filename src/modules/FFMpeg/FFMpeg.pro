TEMPLATE = lib
CONFIG += plugin

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += network

win32: DESTDIR = ../../../app/modules
else: DESTDIR = ../../../app/share/qmplay2/modules

win32: QMAKE_LIBDIR += ../../../app
else: QMAKE_LIBDIR += ../../../app/lib
LIBS += -lqmplay2 -lavformat -lavcodec -lswscale -lavutil
win32: LIBS += -lws2_32

DEFINES += __STDC_CONSTANT_MACROS

RCC_DIR = build/rcc
OBJECTS_DIR = build/obj
MOC_DIR = build/moc

RESOURCES += icons.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += FFMpeg.hpp FFDemux.hpp FFDec.hpp FFDecSW.hpp FFReader.hpp FFCommon.hpp
SOURCES += FFMpeg.cpp FFDemux.cpp FFDec.cpp FFDecSW.cpp FFReader.cpp FFCommon.cpp

unix:!macx {
#Common HWAccel
	HEADERS += FFDecHWAccel.hpp HWAccelHelper.hpp
	SOURCES += FFDecHWAccel.cpp HWAccelHelper.cpp

#VAAPI
	LIBS += -lva -lva-x11
	HEADERS += FFDecVAAPI.hpp VAApiWriter.hpp
	SOURCES += FFDecVAAPI.cpp VAApiWriter.cpp
	DEFINES += QMPlay2_VAAPI

#VDPAU
	LIBS += -lvdpau
	HEADERS += FFDecVDPAU.hpp VDPAUWriter.hpp
	SOURCES += FFDecVDPAU.cpp VDPAUWriter.cpp
	DEFINES += QMPlay2_VDPAU
# 	HEADERS += FFDecVDPAU_NW.hpp
# 	SOURCES += FFDecVDPAU_NW.cpp
# 	DEFINES += QMPlay2_VDPAU_NW
}
