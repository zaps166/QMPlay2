cmake_minimum_required(VERSION 2.8.6)
if(COMMAND cmake_policy)
    if(POLICY CMP0026)
        cmake_policy(SET CMP0026 OLD)
    endif()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_subdirectory(qmplay2)
get_target_property(libqmplay2 qmplay2 LOCATION)
add_subdirectory(modules)
add_subdirectory(gui)
