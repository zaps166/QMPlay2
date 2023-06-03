cmake_minimum_required(VERSION 3.12)
project(langs)

if(LANGUAGES STREQUAL "All")
    # build LANGUAGES from all existing .ts files
    file(GLOB QMPLAY2_TSS *.ts)
else()
    string(REGEX MATCHALL [a-zA-Z_@]+ LANGUAGES_TR ${LANGUAGES})
    set(QMPLAY2_TSS "")
    foreach(LANG ${LANGUAGES_TR})
       LIST(APPEND QMPLAY2_TSS "${LANG}.ts")
    endforeach()
endif()

find_package(${QT_PREFIX}LinguistTools REQUIRED)
if(BUILD_WITH_QT6)
    qt6_add_translation(QM_FILES ${QMPLAY2_TSS})
else()
    qt5_add_translation(QM_FILES ${QMPLAY2_TSS})
endif()

add_custom_target(translations ALL DEPENDS ${QM_FILES})

if(WIN32)
    install(FILES ${QM_FILES} DESTINATION "lang/")
else()
    install(FILES ${QM_FILES} DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/qmplay2/lang")
endif()
