# - Try to find playready.
# Once done, this will define
#
#  PLAYREADY_INCLUDE_DIRS - the playready include directories
#  PLAYREADY_LIBRARIES - link these to use playready.
#
# Copyright (C) 2016 Igalia S.L.
# Copyright (C) 2016 Metrological
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
pkg_check_modules(PC_PLAYREADY playready)

find_path(PLAYREADY_INCLUDE_DIRS
    NAMES drmtypes.h
    HINTS ${PC_PLAYREADY_INCLUDEDIR}/playready
    ${PC_PLAYREADY_INCLUDE_DIRS}/playready
    )

find_library(PLAYREADY_LIBRARIES
    NAMES playready
    HINTS ${PC_PLAYREADY_LIBDIR}
    ${PC_PLAYREADY_LIBRARY_DIRS}
    )

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PLAYREADY DEFAULT_MSG PLAYREADY_LIBRARIES)

if (PLAYREADY_FOUND)
    set(PLAYREADY_LIBRARY ${PLAYREADY_LIBRARIES})
    set(PLAYREADY_INCLUDE_DIR ${PLAYREADY_INCLUDE_DIRS})
endif ()

mark_as_advanced(PLAYREADY_INCLUDE_DIRS PLAYREADY_LIBRARIES)
