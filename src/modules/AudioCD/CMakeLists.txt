project(AudioCD)
cmake_minimum_required(VERSION 2.8.11)

set(AudioCD_HDR
    AudioCD.hpp
    AudioCDDemux.hpp
)

set(AudioCD_SRC
    AudioCD.cpp
    AudioCDDemux.cpp
)

set(AudioCD_RESOURCES
    icon.qrc
)

PKG_CHECK_MODULES(LIBCD libcdio libcddb REQUIRED)
link_directories(${LIBCD_LIBRARY_DIRS})

if(USE_QT5)
    qt5_add_resources(AudioCD_RESOURCES_RCC ${AudioCD_RESOURCES})
else()
    qt4_add_resources(AudioCD_RESOURCES_RCC ${AudioCD_RESOURCES})
endif()

include_directories(../../qmplay2/headers ${LIBCD_INCLUDE_DIRS})

add_library(${PROJECT_NAME} MODULE
    ${AudioCD_HDR}
    ${AudioCD_SRC}
    ${AudioCD_RESOURCES_RCC}
)

if(USE_QT5)
    qt5_use_modules(${PROJECT_NAME} Gui Widgets)
else()
    target_link_libraries(${PROJECT_NAME} Qt4::QtGui Qt4::QtCore)
endif()

add_dependencies(${PROJECT_NAME} qmplay2)
target_link_libraries(${PROJECT_NAME}
    ${LIBCD_LIBRARIES}
    $<TARGET_FILE:qmplay2>
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${MODULES_INSTALL_PATH}")
