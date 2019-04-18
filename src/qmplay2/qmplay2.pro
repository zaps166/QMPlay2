TEMPLATE = lib
CONFIG += plugin #Don't create symlinks to library

QT += widgets qml
android {
    QT += androidextras
}

!qtHaveModule(svg): message("Missing QtSvg module - SVG icons will not be visible!")

TARGET = qmplay2

DESTDIR = ../../app/lib

android {
    LIBS += -lavformat -lavcodec -lswscale -lswresample -lavutil #-lass
} else {
    CONFIG += link_pkgconfig
    PKGCONFIG += libavformat libavcodec libswscale libswresample libavutil libass
}

RESOURCES += languages.qrc

OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc

INCLUDEPATH += . headers
DEPENDPATH  += . headers

HEADERS += headers/QMPlay2Core.hpp headers/Functions.hpp headers/Settings.hpp headers/Module.hpp headers/ModuleParams.hpp headers/ModuleCommon.hpp headers/Playlist.hpp headers/Reader.hpp headers/Demuxer.hpp headers/Decoder.hpp headers/VideoFilters.hpp headers/VideoFilter.hpp headers/DeintFilter.hpp headers/AudioFilter.hpp headers/Writer.hpp headers/QMPlay2Extensions.hpp headers/LineEdit.hpp headers/Slider.hpp headers/QMPlay2OSD.hpp headers/InDockW.hpp headers/LibASS.hpp headers/ColorButton.hpp headers/ImgScaler.hpp headers/SndResampler.hpp headers/VideoWriter.hpp headers/SubsDec.hpp headers/ByteArray.hpp headers/TimeStamp.hpp headers/Packet.hpp headers/VideoFrame.hpp headers/StreamInfo.hpp headers/DockWidget.hpp headers/IOController.hpp headers/ChapterProgramInfo.hpp headers/PacketBuffer.hpp headers/Buffer.hpp headers/NetworkAccess.hpp headers/YouTubeDL.hpp headers/Notifies.hpp headers/NotifiesTray.hpp headers/Version.hpp headers/IPC.hpp headers/MkvMuxer.hpp headers/PixelFormats.hpp headers/CommonJS.hpp headers/NetworkAccessJS.hpp headers/TreeWidgetJS.hpp headers/CPU.hpp headers/HWAccelInterface.hpp headers/VideoAdjustment.hpp headers/CppUtils.hpp
SOURCES +=         QMPlay2Core.cpp         Functions.cpp         Settings.cpp         Module.cpp         ModuleParams.cpp         ModuleCommon.cpp         Playlist.cpp         Reader.cpp         Demuxer.cpp         Decoder.cpp         VideoFilters.cpp         VideoFilter.cpp         DeintFilter.cpp         AudioFilter.cpp         Writer.cpp         QMPlay2Extensions.cpp         LineEdit.cpp         Slider.cpp         QMPlay2OSD.cpp         InDockW.cpp         LibASS.cpp         ColorButton.cpp         ImgScaler.cpp         SndResampler.cpp         VideoWriter.cpp         SubsDec.cpp                                                                        VideoFrame.cpp         StreamInfo.cpp         DockWidget.cpp                                                                 PacketBuffer.cpp         Buffer.cpp         NetworkAccess.cpp         YouTubeDL.cpp         Notifies.cpp         NotifiesTray.cpp         Version.cpp    IPC_Unix.cpp         MkvMuxer.cpp         PixelFormats.cpp         CommonJS.cpp         NetworkAccessJS.cpp         TreeWidgetJS.cpp

unix:!android {
    QT += dbus
    DBUS_INTERFACES += org.freedesktop.Notifications.xml
    HEADERS += headers/NotifiesFreedesktop.hpp
    SOURCES +=         NotifiesFreedesktop.cpp
}
android {
    DEFINES += NOTIFIES_ANDROID
    HEADERS += headers/NotifiesAndroid.hpp
    SOURCES +=         NotifiesAndroid.cpp
}

DEFINES += __STDC_CONSTANT_MACROS USE_QML
!android: DEFINES += QMPLAY2_LIBASS

# Uncomment below lines for avresample:
#DEFINES += QMPLAY2_AVRESAMPLE
#PKGCONFIG -= libswresample
#PKGCONFIG += libavresample
