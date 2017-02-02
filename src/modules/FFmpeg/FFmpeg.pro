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

win32: LIBS += -lws2_32
win32|android: LIBS += -lavformat -lavcodec -lswscale -lavutil
else {
	CONFIG += link_pkgconfig
	PKGCONFIG += libavformat libavcodec libswscale libavutil
}
LIBS += -lqmplay2

DEFINES += __STDC_CONSTANT_MACROS __STDC_LIMIT_MACROS

RCC_DIR = build/rcc
OBJECTS_DIR = build/obj
MOC_DIR = build/moc

RESOURCES += icons.qrc

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += FFmpeg.hpp FFDemux.hpp FFDec.hpp FFDecSW.hpp FFReader.hpp FFCommon.hpp FormatContext.hpp OggHelper.hpp OpenThr.hpp
SOURCES += FFmpeg.cpp FFDemux.cpp FFDec.cpp FFDecSW.cpp FFReader.cpp FFCommon.cpp FormatContext.cpp OggHelper.cpp OpenThr.cpp

unix:!android {
	PKGCONFIG += libavdevice
	DEFINES   += QMPlay2_libavdevice

#Common HWAccel
	HEADERS   += FFDecHWAccel.hpp HWAccelHelper.hpp
	SOURCES   += FFDecHWAccel.cpp HWAccelHelper.cpp

#VAAPI
	PKGCONFIG += libva libva-x11 libva-glx
	HEADERS   += FFDecVAAPI.hpp VAAPI.hpp VAAPIWriter.hpp
	SOURCES   += FFDecVAAPI.cpp VAAPI.cpp VAAPIWriter.cpp
	DEFINES   += QMPlay2_VAAPI

#VDPAU
	PKGCONFIG += vdpau
	HEADERS   += FFDecVDPAU.hpp VDPAUWriter.hpp
	SOURCES   += FFDecVDPAU.cpp VDPAUWriter.cpp
	DEFINES   += QMPlay2_VDPAU
#VDPAU without its own video output (decoded video will be copied to system RAM, can be slow)
	HEADERS   += FFDecVDPAU_NW.hpp
	SOURCES   += FFDecVDPAU_NW.cpp
}

win32 {
#Common HWAccel
	HEADERS   += FFDecHWAccel.hpp HWAccelHelper.hpp
	SOURCES   += FFDecHWAccel.cpp HWAccelHelper.cpp

#DXVA2
	HEADERS   += FFDecDXVA2.hpp
	SOURCES   += FFDecDXVA2.cpp
	DEFINES   += QMPlay2_DXVA2

	LIBS      += -lole32
}
