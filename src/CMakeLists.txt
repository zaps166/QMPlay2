cmake_minimum_required(VERSION 2.8.11)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(USE_QT5 AND Qt5Widgets_VERSION LESS 5.6.1)
    message(AUTHOR_WARNING "Qt5 >= 5.6.1 is recommended for stable usage")
endif()

add_subdirectory(gui)
add_subdirectory(qmplay2)
add_subdirectory(modules)
