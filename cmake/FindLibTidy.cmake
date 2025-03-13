find_path(LIBTIDY_INCLUDE_DIR NAMES tidy.h PATH_SUFFIXES tidy)
find_library(LIBTIDY_LIBRARY NAMES tidy)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibTidy
    FOUND_VAR
        LIBTIDY_FOUND
    REQUIRED_VARS
        LIBTIDY_LIBRARY
        LIBTIDY_INCLUDE_DIR
)

if(LIBTIDY_FOUND AND NOT TARGET LibTidy::LibTidy)
    add_library(LibTidy::LibTidy UNKNOWN IMPORTED)
    set_target_properties(LibTidy::LibTidy PROPERTIES
                          IMPORTED_LOCATION "${LIBTIDY_LIBRARY}"
                          INTERFACE_INCLUDE_DIRECTORIES "${LIBTIDY_INCLUDE_DIR}")
endif()

mark_as_advanced(LIBTIDY_INCLUDE_DIR LIBTIDY_LIBRARY)
