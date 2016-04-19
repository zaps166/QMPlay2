TEMPLATE = lib
!win32: CONFIG += plugin #Don't create symlinks to library

greaterThan(QT_MAJOR_VERSION, 4) {
	lessThan(QT_VERSION, 5.7.0): CONFIG -= c++11
	QT += widgets
}

TARGET = qmplay2

win32|macx: DESTDIR = ../../app
else: DESTDIR = ../../app/lib

win32 {
	LIBS += -lswscale -lswresample -lavutil -Wl,-Bstatic -lass
	!contains(QMAKE_CXX, x86_64-w64-mingw32-g++): LIBS += -lfontconfig -lexpat
	LIBS += -lfreetype -lfribidi -Wl,-Bdynamic -lwinmm -lshell32 -lpowrprof
}
else {
	macx: QT_CONFIG -= no-pkg-config
	CONFIG += link_pkgconfig
	PKGCONFIG += libswscale libswresample libavutil libass
}

OBJECTS_DIR = build/obj
MOC_DIR = build/moc

INCLUDEPATH += . headers
DEPENDPATH  += . headers

HEADERS += headers/QMPlay2Core.hpp headers/Functions.hpp headers/Settings.hpp headers/Module.hpp headers/ModuleParams.hpp headers/ModuleCommon.hpp headers/Playlist.hpp headers/Reader.hpp headers/Demuxer.hpp headers/Decoder.hpp headers/VideoFilters.hpp headers/VideoFilter.hpp headers/DeintFilter.hpp headers/AudioFilter.hpp headers/Writer.hpp headers/QMPlay2Extensions.hpp headers/LineEdit.hpp headers/Slider.hpp headers/QMPlay2_OSD.hpp headers/InDockW.hpp headers/LibASS.hpp headers/ColorButton.hpp headers/ImgScaler.hpp headers/SndResampler.hpp headers/VideoWriter.hpp headers/SubsDec.hpp headers/ByteArray.hpp headers/TimeStamp.hpp headers/Packet.hpp headers/VideoFrame.hpp headers/StreamInfo.hpp headers/DockWidget.hpp headers/IOController.hpp headers/ChapterInfo.hpp headers/PacketBuffer.hpp headers/Buffer.hpp headers/CPU.hpp
SOURCES +=         QMPlay2Core.cpp         Functions.cpp         Settings.cpp         Module.cpp                                                           Playlist.cpp         Reader.cpp         Demuxer.cpp         Decoder.cpp         VideoFilters.cpp         VideoFilter.cpp         DeintFilter.cpp         AudioFilter.cpp         Writer.cpp         QMPlay2Extensions.cpp         LineEdit.cpp         Slider.cpp         QMPlay2_OSD.cpp         InDockW.cpp         LibASS.cpp         ColorButton.cpp         ImgScaler.cpp         SndResampler.cpp                                 SubsDec.cpp                                                                        VideoFrame.cpp         StreamInfo.cpp         DockWidget.cpp                                                          PacketBuffer.cpp         Buffer.cpp

DEFINES += __STDC_CONSTANT_MACROS

# Uncomment below lines for avresample:
#DEFINES += QMPLAY2_AVRESAMPLE
#win32 {
#	LIBS -= -lswresample
#	LIBS += -lavresample
#} else {
#	PKGCONFIG -= libswresample
#	PKGCONFIG += libavresample
#}
