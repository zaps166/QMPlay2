TEMPLATE = lib
CONFIG += plugin

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += network

win32|macx {
	DESTDIR = ../../../app/modules
	QMAKE_LIBDIR += ../../../app
}
else {
	DESTDIR = ../../../app/share/qmplay2/modules
	QMAKE_LIBDIR += ../../../app/lib
}

win32: LIBS += -lws2_32 -lavformat -lavcodec -lswscale -lavutil
else {
	macx: QT_CONFIG -= no-pkg-config
	CONFIG += link_pkgconfig
	PKGCONFIG += libavformat libavcodec libswscale libavutil
}
LIBS += -lqmplay2

DEFINES += __STDC_CONSTANT_MACROS

RCC_DIR = build/rcc
OBJECTS_DIR = build/obj
MOC_DIR = build/moc

RESOURCES += icons.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += FFmpeg.hpp FFDemux.hpp FFDec.hpp FFDecSW.hpp FFReader.hpp FFCommon.hpp
SOURCES += FFmpeg.cpp FFDemux.cpp FFDec.cpp FFDecSW.cpp FFReader.cpp FFCommon.cpp

unix:!macx:!android {
#Common HWAccel
	HEADERS   += FFDecHWAccel.hpp HWAccelHelper.hpp
	SOURCES   += FFDecHWAccel.cpp HWAccelHelper.cpp

#VAAPI
	PKGCONFIG += libva libva-x11
	HEADERS   += FFDecVAAPI.hpp VAApiWriter.hpp
	SOURCES   += FFDecVAAPI.cpp VAApiWriter.cpp
	DEFINES   += QMPlay2_VAAPI

#VDPAU
	PKGCONFIG += vdpau
	HEADERS   += FFDecVDPAU.hpp VDPAUWriter.hpp
	SOURCES   += FFDecVDPAU.cpp VDPAUWriter.cpp
	DEFINES   += QMPlay2_VDPAU
#	HEADERS   += FFDecVDPAU_NW.hpp
#	SOURCES   += FFDecVDPAU_NW.cpp
#	DEFINES   += QMPlay2_VDPAU_NW
}
