# Try to find TagLib, once done this will define:
#  - TAGLIB_INCLUDE_DIRS
#  - TAGLIB_LIBRARY_DIRS
#  - TAGLIB_LIBRARIES
#  - TAGLIB_FOUND

if(NOT WIN32)
    if (TagLib_FIND_REQUIRED)
        pkg_check_modules(TAGLIB taglib REQUIRED)
    elseif(TagLib_FIND_QUIETLY)
        pkg_check_modules(TAGLIB taglib QUIET)
    else()
        pkg_check_modules(TAGLIB taglib)
    endif()
else()
    find_path(TAGLIB_INCLUDE_DIR taglib/taglib.h PATH_SUFFIXES taglib)
    find_library(TAGLIB_LIBRARY NAMES tag)

    include(FindPackageHandleStandardArgs)
    # handle the QUIET and REQUIRED arguments
    # set TAGLIB_FOUND to TRUE if all listed variables are TRUE
    find_package_handle_standard_args(TAGLIB DEFAULT_MSG
                                      TAGLIB_LIBRARY TAGLIB_INCLUDE_DIR)

    mark_as_advanced(TAGLIB_INCLUDE_DIR TAGLIB_LIBRARY)

    set(TAGLIB_INCLUDE_DIRS ${TAGLIB_INCLUDE_DIR})
    set(TAGLIB_LIBRARIES ${TAGLIB_LIBRARY})
endif()
