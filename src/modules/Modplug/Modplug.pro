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

INCLUDEPATH += . ../../qmplay2/headers
DEPENDPATH += . ../../qmplay2/headers

HEADERS += Modplug.hpp MPDemux.hpp
SOURCES += Modplug.cpp MPDemux.cpp

#libmodplug
win32: DEFINES -= UNICODE
INCLUDEPATH += libmodplug
DEPENDPATH += libmodplug
SOURCES += libmodplug/fastmix.cpp libmodplug/load_ams.cpp libmodplug/load_dsm.cpp libmodplug/load_j2b.cpp libmodplug/load_mod.cpp libmodplug/load_okt.cpp libmodplug/load_s3m.cpp libmodplug/load_umx.cpp libmodplug/orig_modplug.cpp libmodplug/snd_flt.cpp libmodplug/load_669.cpp libmodplug/load_dbm.cpp libmodplug/load_far.cpp libmodplug/load_mdl.cpp libmodplug/load_mt2.cpp libmodplug/load_psm.cpp libmodplug/load_stm.cpp libmodplug/load_xm.cpp libmodplug/snd_dsp.cpp libmodplug/snd_fx.cpp libmodplug/load_amf.cpp libmodplug/load_dmf.cpp libmodplug/load_it.cpp libmodplug/load_med.cpp libmodplug/load_mtm.cpp libmodplug/load_ptm.cpp libmodplug/load_ult.cpp libmodplug/load_sfx.cpp libmodplug/mmcmp.cpp libmodplug/sndfile.cpp libmodplug/sndmix.cpp
