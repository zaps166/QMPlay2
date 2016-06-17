# - Try to find LibGME
# Once done this will define
#  LIBGME_FOUND - System has LibGME
#  LIBGME_INCLUDE_DIRS - The LibGME include directories
#  LIBGME_LIBRARIES - The libraries needed to use LibGME
#  LIBGME_DEFINITIONS - Compiler switches required for using LibGME

find_package(PkgConfig)
pkg_check_modules(PC_LIBGME QUIET libgme)
set(LIBGME_DEFINITIONS ${PC_LIBGME_CFLAGS_OTHER})

find_path(LIBGME_INCLUDE_DIR gme/gme.h
          HINTS ${PC_LIBGME_INCLUDEDIR} ${PC_LIBGME_INCLUDE_DIRS})

find_library(LIBGME_LIBRARY NAMES gme
             HINTS ${PC_LIBGME_LIBDIR} ${PC_LIBGME_LIBRARY_DIRS} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBGME_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LibGME DEFAULT_MSG
                                  LIBGME_LIBRARY LIBGME_INCLUDE_DIR)

mark_as_advanced(LIBGME_INCLUDE_DIR LIBGME_LIBRARY)

set(LIBGME_LIBRARIES ${LIBGME_LIBRARY})
set(LIBGME_INCLUDE_DIRS ${LIBGME_INCLUDE_DIR})
