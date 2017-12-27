#.rst:
# ECMGeneratePkgConfigFile
# ------------------------
#
# Generate a `pkg-config <http://www.freedesktop.org/wiki/Software/pkg-config/>`_
# file for the benefit of
# `autotools <http://www.gnu.org/software/automake/manual/html_node/Autotools-Introduction.html>`_-based
# projects.
#
# ::
#
#   ecm_generate_pkgconfig_file(BASE_NAME <baseName>
#                         [LIB_NAME <libName>]
#                         [DEPS "<dep> [<dep> [...]]"]
#                         [FILENAME_VAR <filename_variable>]
#                         [INCLUDE_INSTALL_DIR <dir>]
#                         [LIB_INSTALL_DIR <dir>]
#                         [DEFINES -D<variable=value>...]
#                         [DESCRIPTION <library description>]
#                         [INSTALL])
#
# ``BASE_NAME`` is the name of the module. It's the name projects will use to
# find the module.
#
# ``LIB_NAME`` is the name of the library that is being exported. If undefined,
# it will default to the ``BASE_NAME``. That means the ``LIB_NAME`` will be set
# as the name field as well as the library to link to.
#
# ``FILENAME_VAR`` is specified with a variable name. This variable will
# receive the location of the generated file will be set, within the build
# directory. This way it can be used in case some processing is required. See
# also ``INSTALL``.
#
# ``INCLUDE_INSTALL_DIR`` specifies where the includes will be installed. If
# it's not specified, it will default to ``INSTALL_INCLUDEDIR``,
# ``CMAKE_INSTALL_INCLUDEDIR`` or just "include/" in case they are specified,
# with the BASE_NAME postfixed.
#
# ``LIB_INSTALL_DIR`` specifies where the library is being installed. If it's
# not specified, it will default to ``LIB_INSTALL_DIR``,
# ``CMAKE_INSTALL_LIBDIR`` or just "lib/" in case they are specified.
#
# ``DEFINES`` is a list of preprocessor defines that it is recommended users of
# the library pass to the compiler when using it.
#
# ``DESCRIPTION`` describes what this library is. If it's not specified, CMake
# will first try to get the description from the metainfo.yaml file or will
# create one based on ``LIB_NAME``.
#
# ``INSTALL`` will cause the module to be installed to the ``pkgconfig``
# subdirectory of ``LIB_INSTALL_DIR``, unless the ``ECM_PKGCONFIG_INSTALL_DIR``
# cache variable is set to something different. Note that the first call to
# ecm_generate_pkgconfig_file with the ``INSTALL`` argument will cause
# ``ECM_PKGCONFIG_INSTALL_DIR`` to be set to the cache, and will be used in any
# subsequent calls.
#
# To properly use this macro a version needs to be set. To retrieve it,
# ``ECM_PKGCONFIG_INSTALL_DIR`` uses ``PROJECT_VERSION``. To set it, use the
# project() command (only available since CMake 3.0) or the ecm_setup_version()
# macro.
#
# Example usage:
#
# .. code-block:: cmake
#
#   ecm_generate_pkgconfig_file(
#       BASE_NAME KF5Archive
#       DEPS Qt5Core
#       FILENAME_VAR pkgconfig_filename
#       INSTALL
#   )
#
# Since 1.3.0.
# ``DESCRIPTION`` available since 5.1.41
#

#=============================================================================
# Copyright 2014 Aleix Pol Gonzalez <aleixpol@kde.org>
# Copyright 2014 David Faure <faure@kde.org>
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

function(ECM_GENERATE_PKGCONFIG_FILE)
  set(options INSTALL)
  set(oneValueArgs BASE_NAME LIB_NAME FILENAME_VAR INCLUDE_INSTALL_DIR LIB_INSTALL_DIR DESCRIPTION)
  set(multiValueArgs DEPS DEFINES)

  cmake_parse_arguments(EGPF "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(EGPF_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unknown keywords given to ECM_GENERATE_PKGCONFIG_FILE(): \"${EGPF_UNPARSED_ARGUMENTS}\"")
  endif()

  if(NOT EGPF_BASE_NAME)
    message(FATAL_ERROR "Required argument BASE_NAME missing in ECM_GENERATE_PKGCONFIG_FILE() call")
  endif()
  if(NOT PROJECT_VERSION)
    message(FATAL_ERROR "Required variable PROJECT_VERSION not set before ECM_GENERATE_PKGCONFIG_FILE() call. Did you call ecm_setup_version or project with the VERSION argument?")
  endif()
  if(NOT EGPF_LIB_NAME)
    set(EGPF_LIB_NAME ${EGPF_BASE_NAME})
  endif()
  if(NOT EGPF_INCLUDE_INSTALL_DIR)
      if(INCLUDE_INSTALL_DIR)
          set(EGPF_INCLUDE_INSTALL_DIR "${INCLUDE_INSTALL_DIR}/${EGPF_BASE_NAME}")
      elseif(CMAKE_INSTALL_INCLUDEDIR)
          set(EGPF_INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}/${EGPF_BASE_NAME}")
      else()
          set(EGPF_INCLUDE_INSTALL_DIR "include/${EGPF_BASE_NAME}")
      endif()
  endif()
  if(NOT EGPF_LIB_INSTALL_DIR)
      if(LIB_INSTALL_DIR)
          set(EGPF_LIB_INSTALL_DIR "${LIB_INSTALL_DIR}")
      elseif(CMAKE_INSTALL_LIBDIR)
          set(EGPF_LIB_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}")
      else()
          set(EGPF_LIB_INSTALL_DIR "lib")
      endif()
  endif()
  if(NOT EGPF_DESCRIPTION)
      if(EXISTS ${CMAKE_SOURCE_DIR}/metainfo.yaml)
          file(STRINGS "${CMAKE_SOURCE_DIR}/metainfo.yaml" _EGPF_METAINFO_DESCRIPTION_STRING REGEX "^description:.*$")
          if(_EGPF_METAINFO_DESCRIPTION_STRING)
              string(REGEX REPLACE "^description:[ ]*(.*)" "\\1" EGPF_DESCRIPTION ${_EGPF_METAINFO_DESCRIPTION_STRING})
          endif()
      endif()
      if("${EGPF_DESCRIPTION}" STREQUAL "")
          set(EGPF_DESCRIPTION "${EGPF_LIB_NAME} library.")
      endif()
  endif()

  set(PKGCONFIG_TARGET_BASENAME ${EGPF_BASE_NAME})
  set(PKGCONFIG_TARGET_LIBNAME ${EGPF_LIB_NAME})
  if (DEFINED EGPF_DEPS)
    string(REPLACE ";" " " PKGCONFIG_TARGET_DEPS "${EGPF_DEPS}")
  endif ()
  if(IS_ABSOLUTE "${EGPF_INCLUDE_INSTALL_DIR}")
      set(PKGCONFIG_TARGET_INCLUDES "-I${EGPF_INCLUDE_INSTALL_DIR}")
  else()
      set(PKGCONFIG_TARGET_INCLUDES "-I${CMAKE_INSTALL_PREFIX}/${EGPF_INCLUDE_INSTALL_DIR}")
  endif()
  if(IS_ABSOLUTE "${EGPF_LIB_INSTALL_DIR}")
      set(PKGCONFIG_TARGET_LIBS "${EGPF_LIB_INSTALL_DIR}")
  else()
      set(PKGCONFIG_TARGET_LIBS "${CMAKE_INSTALL_PREFIX}/${EGPF_LIB_INSTALL_DIR}")
  endif()
  set(PKGCONFIG_TARGET_DESCRIPTION "${EGPF_DESCRIPTION}")
  set(PKGCONFIG_TARGET_DEFINES "")
  if(EGPF_DEFINES)
    set(PKGCONFIG_TARGET_DEFINES "${EGPF_DEFINE}")
  endif()

  set(PKGCONFIG_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/${PKGCONFIG_TARGET_BASENAME}.pc)
  if (EGPF_FILENAME_VAR)
     set(${EGPF_FILENAME_VAR} ${PKGCONFIG_FILENAME} PARENT_SCOPE)
  endif()

  file(WRITE ${PKGCONFIG_FILENAME}
"
Name: ${PKGCONFIG_TARGET_LIBNAME}
Description: ${PKGCONFIG_TARGET_DESCRIPTION}
Version: ${PROJECT_VERSION}
Libs: -L${CMAKE_INSTALL_PREFIX}/${EGPF_LIB_INSTALL_DIR} -l${PKGCONFIG_TARGET_LIBNAME}
Cflags: ${PKGCONFIG_TARGET_INCLUDES} ${PKGCONFIG_TARGET_DEFINES}
Requires: ${PKGCONFIG_TARGET_DEPS}
"
  )

  if(EGPF_INSTALL)
    if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
      set(ECM_PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/libdata/pkgconfig" CACHE PATH "The directory where pkgconfig will be installed to.")
    else()
      set(ECM_PKGCONFIG_INSTALL_DIR "${EGPF_LIB_INSTALL_DIR}/pkgconfig" CACHE PATH "The directory where pkgconfig will be installed to.")
    endif()
    install(FILES ${PKGCONFIG_FILENAME} DESTINATION ${ECM_PKGCONFIG_INSTALL_DIR})
  endif()
endfunction()
