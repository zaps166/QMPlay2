cmake_minimum_required(VERSION 3.16)
project(AudioFilters)

set(AudioFilters_HDR
    AudioFilters.hpp
    Equalizer.hpp
    EqualizerGUI.hpp
    VoiceRemoval.hpp
    PhaseReverse.hpp
    SwapStereo.hpp
    Echo.hpp
    DysonCompressor.hpp
    BS2B.hpp
    bs2b/bs2b.hpp
    bs2b/bs2bversion.hpp
)

set(AudioFilters_SRC
    AudioFilters.cpp
    Equalizer.cpp
    EqualizerGUI.cpp
    VoiceRemoval.cpp
    PhaseReverse.cpp
    SwapStereo.cpp
    Echo.cpp
    DysonCompressor.cpp
    BS2B.cpp
    bs2b/bs2b_lib.cpp
)

if(USE_AVAUDIOFILTER)
    list(APPEND AudioFilters_HDR
        AVAudioFilter.hpp
    )
    list(APPEND AudioFilters_SRC
        AVAudioFilter.cpp
    )
endif()

set(AudioFilters_RESOURCES
    icon.qrc
)

add_library(${PROJECT_NAME} MODULE
    ${AudioFilters_HDR}
    ${AudioFilters_SRC}
    ${AudioFilters_RESOURCES}
)

if(USE_AVAUDIOFILTER)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        -DUSE_AVAUDIOFILTER
    )
    target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${LIBAVFILTER_INCLUDE_DIRS}
    )
    target_link_directories(${PROJECT_NAME}
        PRIVATE
        ${LIBAVFILTER_LIBRARY_DIRS}
    )
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ${LIBAVFILTER_LIBRARIES}
    )
endif()

if(USE_PCH)
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        ${AudioFilters_HDR}
    )
endif()

libqmplay2_set_target_params()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
