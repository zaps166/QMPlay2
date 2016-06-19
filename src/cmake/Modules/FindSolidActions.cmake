# Old version for trying to find using extra-cmake-modules

#find_package(ECM NO_MODULE QUIET)
#if(ECM_FOUND)
#    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
#    include(KDEInstallDirs)
#    set(PC_INSTALL_PATH_SOLID_ACTION "/usr/${DATA_INSTALL_DIR}/solid/actions")
#endif()

if(EXISTS "${CMAKE_INSTALL_PREFIX}/share/solid/actions")
    set(LOCAL_INSTALL_PATH_SOLID_ACTION "${CMAKE_INSTALL_PREFIX}/share/solid/actions")
elseif(EXISTS "${CMAKE_INSTALL_PREFIX}/share/kde4/apps/solid/actions")
    set(LOCAL_INSTALL_PATH_SOLID_ACTION "${CMAKE_INSTALL_PREFIX}/share/kde4/apps/solid/actions")
elseif(EXISTS "${CMAKE_INSTALL_PREFIX}/share/apps/solid/actions")
    set(LOCAL_INSTALL_PATH_SOLID_ACTION "${CMAKE_INSTALL_PREFIX}/share/apps/solid/actions")
endif()
