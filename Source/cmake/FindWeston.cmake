# - Try to find Weston.
# Once done, this will define
#
#  WESTON_FOUND - system has Wayland.
#  WESTON_INCLUDE_DIRS - the Wayland include directories
#  WESTON_LIBRARIES - link these to use Wayland.
#
# Copyright (C) 2014 Igalia S.L.
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
pkg_check_modules(PC_WESTON QUIET weston)

find_path(WESTON_INCLUDE_DIRS
    NAMES weston/compositor.h
    HINTS ${PC_WESTON_INCLUDEDIR}
          ${PC_WESTON_INCLUDE_DIRS}
)

set(WESTON_INCLUDE_DIRS ${WESTON_INCLUDE_DIRS} ${PC_WESTON_INCLUDE_DIRS})

# Weston does not provide any usable libraries.

if ("${Weston_FIND_VERSION}" VERSION_GREATER "${PC_WESTON_VERSION}")
    message(FATAL_ERROR "Required version (" ${Weston_FIND_VERSION} ") is higher than found version (" ${PC_WESTON_VERSION} ")")
endif ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(WESTON REQUIRED_VARS WESTON_INCLUDE_DIRS VERSION_VAR PC_WESTON_VERSION)

mark_as_advanced(WESTON_INCLUDE_DIRS)
