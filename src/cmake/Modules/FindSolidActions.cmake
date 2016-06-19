# Old version for trying to find using extra-cmake-modules

if(EXISTS "${CMAKE_INSTALL_PREFIX}/share/solid/actions")
    set(LOCAL_INSTALL_PATH_SOLID_ACTION "${CMAKE_INSTALL_PREFIX}/share/solid/actions")
elseif(EXISTS "${CMAKE_INSTALL_PREFIX}/share/kde4/apps/solid/actions")
    set(LOCAL_INSTALL_PATH_SOLID_ACTION "${CMAKE_INSTALL_PREFIX}/share/kde4/apps/solid/actions")
elseif(EXISTS "${CMAKE_INSTALL_PREFIX}/share/apps/solid/actions")
    set(LOCAL_INSTALL_PATH_SOLID_ACTION "${CMAKE_INSTALL_PREFIX}/share/apps/solid/actions")
endif()
