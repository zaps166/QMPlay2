cmake_minimum_required(VERSION 2.8.6)
if(COMMAND cmake_policy)
    if(POLICY CMP0003)
        cmake_policy(SET CMP0003 NEW)
    endif()
    if(POLICY CMP0020)
        cmake_policy(SET CMP0020 NEW)
    endif()
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif()
endif()
project(CUVID)

set(CUVID_HDR
    Cuvid.hpp
    CuvidDec.hpp
)

set(CUVID_SRC
    Cuvid.cpp
    CuvidDec.cpp
)

set(CUVID_RESOURCES
   icon.qrc
)

link_directories(
    ${LIBAVCODEC_LIBRARY_DIRS}
    ${LIBSWSCALE_LIBRARY_DIRS}
    ${LIBAVUTIL_LIBRARY_DIRS}
)

set(CUVID_LIBS
    ${LIBAVCODEC_LIBRARIES}
    ${LIBSWSCALE_LIBRARIES}
    ${LIBAVUTIL_LIBRARIES}
)

if(USE_QT5)
	qt5_add_resources(CUVID_RESOURCES ${CUVID_RESOURCES})
else()
	qt4_add_resources(CUVID_RESOURCES ${CUVID_RESOURCES})
endif()

include_directories(../../qmplay2/headers
    ${LIBAVCODEC_INCLUDE_DIRS}
    ${LIBSWSCALE_INCLUDE_DIRS}
    ${LIBAVUTIL_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME} MODULE
    ${CUVID_HDR}
    ${CUVID_SRC}
    ${CUVID_RESOURCES}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtGui)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    ${CUVID_LIBS}
    ${libqmplay2}
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
