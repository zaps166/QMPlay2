cmake_minimum_required(VERSION 3.1)
project(AudioCD)

set(AudioCD_HDR
    AudioCD.hpp
    AudioCDDemux.hpp
)

set(AudioCD_SRC
    AudioCD.cpp
    AudioCDDemux.cpp
)

set(AudioCD_RESOURCES
    icons.qrc
)

pkg_check_modules(LIBCD libcdio libcddb REQUIRED)
link_directories(${LIBCD_LIBRARY_DIRS})

include_directories(${LIBCD_INCLUDE_DIRS})

add_library(${PROJECT_NAME} ${QMPLAY2_MODULE}
    ${AudioCD_HDR}
    ${AudioCD_SRC}
    ${AudioCD_RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    libqmplay2
)

if(WIN32)
    set(CUSTOM_AUDIOCD_LIBRARIES "" CACHE STRING "Custom libraries for AudioCD")
    mark_as_advanced(CUSTOM_AUDIOCD_LIBRARIES)
endif()

if(WIN32 AND CUSTOM_AUDIOCD_LIBRARIES)
    separate_arguments(CUSTOM_AUDIOCD_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} ${CUSTOM_AUDIOCD_LIBRARIES})
else()
    target_link_libraries(${PROJECT_NAME} ${LIBCD_LIBRARIES})
endif()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
