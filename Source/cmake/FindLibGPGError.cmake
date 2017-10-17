#.rst
# FindLibGPGError
# -------------
#
# Finds the libgpg-error library.
#
# This will define the following variables:
#
# ``LIBGPGERROR_FOUND``
#     True if the requested version of libgpg-error was found
# ``LIBGPGERROR_VERSION``
#     The version of libgpg-error that was found
# ``LIBGPGERROR_INCLUDE_DIRS``
#     The libgpg-error include directories
# ``LIBGPGERROR_LIBRARIES``
#     The linker libraries needed to use the libgpg-error library

# Copyright 2014 Nicolás Alvarez <nicolas.alvarez@gmail.com>
# Copyright 2016 Igalia S.L
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

find_program(LIBGPGERRORCONFIG_SCRIPT NAMES gpg-error-config)
if (LIBGPGERRORCONFIG_SCRIPT)
    execute_process(
        COMMAND "${LIBGPGERRORCONFIG_SCRIPT}" --prefix
        RESULT_VARIABLE CONFIGSCRIPT_RESULT
        OUTPUT_VARIABLE PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (CONFIGSCRIPT_RESULT EQUAL 0)
        set(LIBGPGERROR_LIB_HINT "${PREFIX}/lib")
        set(LIBGPGERROR_INCLUDE_HINT "${PREFIX}/include")
    endif ()
endif ()

find_library(LIBGPGERROR_LIBRARY
    NAMES gpg-error
    HINTS ${LIBGPGERROR_LIB_HINT}
)
find_path(LIBGPGERROR_INCLUDE_DIR
    NAMES gpg-error.h
    HINTS ${LIBGPGERROR_INCLUDE_HINT}
)

if (LIBGPGERROR_INCLUDE_DIR)
    file(STRINGS ${LIBGPGERROR_INCLUDE_DIR}/error.h GPG_ERROR_H REGEX "^#define GPG_ERROR_VERSION ")
    string(REGEX REPLACE "^#define GPG_ERROR_VERSION \"(.*)\".*$" "\\1" LIBGPGERROR_VERSION "${GPG_ERROR_H}")
endif ()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LIBGPGERROR
    FOUND_VAR LIBGPGERROR_FOUND
    REQUIRED_VARS LIBGPGERROR_LIBRARY LIBGPGERROR_INCLUDE_DIR
    VERSION_VAR LIBGPGERROR_VERSION
)
if (LIBGPGERROR_FOUND)
    set(LIBGPGERROR_LIBRARIES ${LIBGPGERROR_LIBRARY})
    set(LIBGPGERROR_INCLUDE_DIRS ${LIBGPGERROR_INCLUDE_DIR})
endif ()

mark_as_advanced(LIBGPGERROR_LIBRARY LIBGPGERROR_INCLUDE_DIR)
