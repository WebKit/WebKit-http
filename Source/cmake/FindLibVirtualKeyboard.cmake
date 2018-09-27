# - Try to find libvirtualkeyboard
# Once done this will define
#  LIBVIRTUAL_KEYBOARD_FOUND - System has libvirtualkeyboard
#  LIBVIRTUAL_KEYBOARD_INCLUDE_DIRS - The libvirtualkeyboard include directories
#  LIBVIRTUAL_KEYBOARD_LIBRARIES - The libraries needed to use libvirtualkeyboard
#
# Copyright (C) 2016 Metrological.
#
find_package(PkgConfig)
pkg_check_modules(LIBVIRTUAL_KEYBOARD REQUIRED virtualkeyboard)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBVIRTUAL_KEYBOARD DEFAULT_MSG
        LIBVIRTUAL_KEYBOARD_FOUND
        LIBVIRTUAL_KEYBOARD_INCLUDE_DIRS
        LIBVIRTUAL_KEYBOARD_LIBRARIES
        )

mark_as_advanced(LIBVIRTUAL_KEYBOARD_INCLUDE_DIRS LIBVIRTUAL_KEYBOARD_LIBRARIES)
