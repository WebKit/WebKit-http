# - Try to find Ecore
# Once done, this will define
#
#  ECORE_FOUND - system has Ecore installed.
#  ECORE_INCLUDE_DIRS - directories which contain the Ecore headers.
#  ECORE_LIBRARIES - libraries required to link against Ecore.
#
# Optionally, the COMPONENTS keyword can be passed to FIND_PACKAGE()
# and additional Ecore libraries can be looked for. Currently, the
# following libraries can be searched, and they define the following
# variables if found:
#
#  EVAS  - ECORE_EVAS_INCLUDE_DIRS and ECORE_EVAS_LIBRARIES
#  FILE  - ECORE_FILE_INCLUDE_DIRS and ECORE_FILE_LIBRARIES
#  INPUT - ECORE_INPUT_INCLUDE_DIRS and ECORE_INPUT_LIBRARIES
#  X     - ECORE_X_INCLUDE_DIRS and ECORE_X_LIBRARIES
#
# Copyright (C) 2012 Intel Corporation. All rights reserved.
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

INCLUDE(EFLHelpers)

FIND_EFL_LIBRARY(ECORE
    HEADERS Ecore.h
    HEADER_PREFIXES ecore-1
    LIBRARY ecore
)

# Components.
FIND_EFL_LIBRARY(ECORE_EVAS
    HEADERS Ecore_Evas.h
    HEADER_PREFIXES ecore-1
    LIBRARY ecore_evas
)
FIND_EFL_LIBRARY(ECORE_FILE
    HEADERS Ecore_File.h
    HEADER_PREFIXES ecore-1
    LIBRARY ecore_file
)
FIND_EFL_LIBRARY(ECORE_INPUT
    HEADERS Ecore_Input.h
    HEADER_PREFIXES ecore-1
    LIBRARY ecore_input
)
FIND_EFL_LIBRARY(ECORE_X
    HEADERS Ecore_X.h
    HEADER_PREFIXES ecore-1
    LIBRARY ecore_x
)

FOREACH(_component ${Ecore_FIND_COMPONENTS})
    SET(_ecore_component "ECORE_${_component}")
    STRING(TOUPPER ${_ecore_component} _UPPER_NAME)

    LIST(APPEND _ECORE_REQUIRED_COMPONENT_VARS ${_UPPER_NAME}_INCLUDE_DIRS ${_UPPER_NAME}_LIBRARIES)
ENDFOREACH()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Ecore REQUIRED_VARS ECORE_INCLUDE_DIRS ECORE_LIBRARIES ${_ECORE_REQUIRED_COMPONENT_VARS}
                                        VERSION_VAR   ECORE_VERSION)
