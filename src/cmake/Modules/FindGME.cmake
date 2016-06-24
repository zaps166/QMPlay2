# Try to find libGME, once done this will define:
#  - LIBGME_INCLUDE_DIRS
#  - LIBGME_LIBRARY_DIRS
#  - LIBGME_LIBRARIES
#  - LIBGME_FOUND

pkg_check_modules(LIBGME QUIET libgme)
if(NOT LIBGME_FOUND)
    find_path(LIBGME_INCLUDE_DIR gme/gme.h)
    find_library(LIBGME_LIBRARY NAMES gme)

    include(FindPackageHandleStandardArgs)
    # handle the QUIET and REQUIRED arguments
    # set LIBGME_FOUND to TRUE if all listed variables are TRUE
    find_package_handle_standard_args(LIBGME DEFAULT_MSG
                                      LIBGME_LIBRARY LIBGME_INCLUDE_DIR)

    mark_as_advanced(LIBGME_INCLUDE_DIR LIBGME_LIBRARY)

    set(LIBGME_INCLUDE_DIRS ${LIBGME_INCLUDE_DIR})
    set(LIBGME_LIBRARIES ${LIBGME_LIBRARY})
endif()
