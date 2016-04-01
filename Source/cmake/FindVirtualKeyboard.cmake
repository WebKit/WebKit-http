# - Try to find LibGeneric
# Once done this will define
#  LIBVIRTUAL_KEYBOARD_FOUND - System has LibGeneric
#  LIBVIRTUAL_KEYBOARD_INCLUDE_DIRS - The LibGeneric include directories
#  LIBVIRTUAL_KEYBOARD_LIBRARIES - The libraries needed to use LibGeneric
#
# Copyright (C) 2015 Metrological.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND ITS CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ITS
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

find_package(PkgConfig)
pkg_check_modules(PC_VK QUIET virtualkeyboard)

find_path(LIBVIRTUAL_KEYBOARD_INCLUDE_DIR virtualkeyboard/VirtualKeyboard.h
          HINTS ${PC_VK_INCLUDEDIR} ${PC_VK_INCLUDE_DIRS}
          PATH_SUFFIXES gluelogic)

find_library(LIBVIRTUAL_KEYBOARD_LIBRARY NAMES virtualkeyboard 
             HINTS ${PC_VK_LIBDIR} ${PC_VK_LIBRARY_DIRS})

set(LIBVIRTUAL_KEYBOARD_LIBRARIES ${LIBVIRTUAL_KEYBOARD_LIBRARY})
set(LIBVIRTUAL_KEYBOARD_INCLUDE_DIRS ${LIBVIRTUAL_KEYBOARD_INCLUDE_DIR})
set(LIBVIRTUAL_KEYBOARD_DEFINES ${PC_GLUELOGIC_CFLAGS})

add_definitions ( ${LIBVIRTUAL_KEYBOARD_DEFINES} )

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LIBVIRTUAL_KEYBOARD DEFAULT_MSG LIBVIRTUAL_KEYBOARD_LIBRARIES LIBVIRTUAL_KEYBOARD_INCLUDE_DIRS)

mark_as_advanced(LIBVIRTUAL_KEYBOARD_LIBRARIES LIBVIRTUAL_KEYBOARD_INCLUDE_DIRS )
