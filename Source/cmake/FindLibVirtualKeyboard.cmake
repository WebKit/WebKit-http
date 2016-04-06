# - Try to find libvirtualkeyboard
# Once done this will define
#  LIBVIRTUAL_KEYBOARD_FOUND - System has libvirtualkeyboard
#  LIBVIRTUAL_KEYBOARD_INCLUDE_DIRS - The libvirtualkeyboard include directories
#  LIBVIRTUAL_KEYBOARD_LIBRARIES - The libraries needed to use libvirtualkeyboard
#
# Copyright (C) 2016 Metrological.
#
find_package(PkgConfig)
pkg_check_modules(VIRTUALKEYBOARD REQUIRED virtualkeyboard)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(VIRTUALKEYBOARD DEFAULT_MSG
        VIRTUALKEYBOARD_FOUND
        VIRTUALKEYBOARD_INCLUDE_DIRS
        VIRTUALKEYBOARD_LIBRARIES
        )

mark_as_advanced(VIRTUALKEYBOARD_INCLUDE_DIRS VIRTUALKEYBOARD_LIBRARIES)
