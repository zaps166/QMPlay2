cmake_minimum_required(VERSION 3.1)
project(Modplug)

set(Modplug_HDR
    Modplug.hpp
    MPDemux.hpp
)

set(Modplug_SRC
    Modplug.cpp
    MPDemux.cpp

    libmodplug/fastmix.cpp
    libmodplug/libmodplug.cpp
    libmodplug/load_669.cpp
    libmodplug/load_amf.cpp
    libmodplug/load_ams.cpp
    libmodplug/load_dbm.cpp
    libmodplug/load_dsm.cpp
    libmodplug/load_dmf.cpp
    libmodplug/load_far.cpp
    libmodplug/load_it.cpp
    libmodplug/load_j2b.cpp
    libmodplug/load_mdl.cpp
    libmodplug/load_med.cpp
    libmodplug/load_mod.cpp
    libmodplug/load_mt2.cpp
    libmodplug/load_mtm.cpp
    libmodplug/load_okt.cpp
    libmodplug/load_psm.cpp
    libmodplug/load_ptm.cpp
    libmodplug/load_s3m.cpp
    libmodplug/load_sfx.cpp
    libmodplug/load_stm.cpp
    libmodplug/load_ult.cpp
    libmodplug/load_umx.cpp
    libmodplug/load_xm.cpp
    libmodplug/mmcmp.cpp
    libmodplug/snd_dsp.cpp
    libmodplug/snd_flt.cpp
    libmodplug/snd_fx.cpp
    libmodplug/sndfile.cpp
    libmodplug/sndmix.cpp
)

set(Modplug_RESOURCES
    icons.qrc
)

include_directories(libmodplug)

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${Modplug_HDR}
    ${Modplug_SRC}
    ${Modplug_RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    libqmplay2
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
