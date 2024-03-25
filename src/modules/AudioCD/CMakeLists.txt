cmake_minimum_required(VERSION 3.13)
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

pkg_check_modules(LIBCD REQUIRED libcdio libcddb)
link_directories(${LIBCD_LIBRARY_DIRS})

include_directories(${LIBCD_INCLUDE_DIRS})

add_library(${PROJECT_NAME} MODULE
    ${AudioCD_HDR}
    ${AudioCD_SRC}
    ${AudioCD_RESOURCES}
)

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        ${AudioCD_HDR}
    )
endif()

target_link_libraries(${PROJECT_NAME}
    libqmplay2
)

if(WIN32)
    set(CUSTOM_AUDIOCD_LIBRARIES "" CACHE STRING "Custom libraries for AudioCD")
endif()

if(NOT WIN32 OR CMAKE_HOST_WIN32)
    target_link_libraries(${PROJECT_NAME} ${LIBCD_LIBRARIES})
endif()

if(WIN32 AND CUSTOM_AUDIOCD_LIBRARIES)
    separate_arguments(CUSTOM_AUDIOCD_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} ${CUSTOM_AUDIOCD_LIBRARIES})
endif()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
