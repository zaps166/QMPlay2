cmake_minimum_required(VERSION 2.8.6)
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

if(USE_QT5)
    find_package(Qt5LinguistTools REQUIRED)
    qt5_add_translation(QM_FILES ${QMPLAY2_TSS})
else()
    qt4_add_translation(QM_FILES ${QMPLAY2_TSS})
endif()

add_custom_target(translations ALL DEPENDS ${QM_FILES})

if(WIN32)
    install(FILES ${QM_FILES} DESTINATION "lang/")
else()
    install(FILES ${QM_FILES} DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/qmplay2/lang/")
endif()
