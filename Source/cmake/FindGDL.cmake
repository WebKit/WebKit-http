
find_package(PkgConfig)
pkg_check_modules(GDL gdl)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GDL DEFAULT_MSG GDL_LIBRARIES)

mark_as_advanced(GDL_INCLUDE_DIRS GDL_LIBRARIES)
