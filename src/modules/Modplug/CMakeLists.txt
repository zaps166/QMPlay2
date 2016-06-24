cmake_minimum_required(VERSION 2.8.11)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif()
endif()
project(Modplug)

set(Modplug_HDR
    Modplug.hpp
    MPDemux.hpp
)

set(Modplug_SRC
    Modplug.cpp
    MPDemux.cpp

    libmodplug/fastmix.cpp
    libmodplug/load_ams.cpp
    libmodplug/load_dsm.cpp
    libmodplug/load_j2b.cpp
    libmodplug/load_mod.cpp
    libmodplug/load_okt.cpp
    libmodplug/load_s3m.cpp
    libmodplug/load_umx.cpp
    libmodplug/orig_modplug.cpp
    libmodplug/snd_flt.cpp
    libmodplug/load_669.cpp
    libmodplug/load_dbm.cpp
    libmodplug/load_far.cpp
    libmodplug/load_mdl.cpp
    libmodplug/load_mt2.cpp
    libmodplug/load_psm.cpp
    libmodplug/load_stm.cpp
    libmodplug/load_xm.cpp
    libmodplug/snd_dsp.cpp
    libmodplug/snd_fx.cpp
    libmodplug/load_amf.cpp
    libmodplug/load_dmf.cpp
    libmodplug/load_it.cpp
    libmodplug/load_med.cpp
    libmodplug/load_mtm.cpp
    libmodplug/load_ptm.cpp
    libmodplug/load_ult.cpp
    libmodplug/load_sfx.cpp
    libmodplug/mmcmp.cpp
    libmodplug/sndfile.cpp
    libmodplug/sndmix.cpp
)

pkg_check_modules(ZLIB zlib REQUIRED)
link_directories(${ZLIB_LIBRARY_DIRS})

include_directories(../../qmplay2/headers libmodplug ${ZLIB_INCLUDE_DIRS})

add_library(${PROJECT_NAME} MODULE
    ${Modplug_HDR}
    ${Modplug_SRC}
)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    find_package(ZLIB REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${ZLIB_LIBRARY})
endif()

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    ${ZLIB_LIBRARIES}
    $<TARGET_FILE:qmplay2>
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
