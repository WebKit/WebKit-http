#.rst:
# ECMPackageConfigHelpers
# -----------------------
#
# Helper macros for generating CMake package config files.
#
# ``write_basic_package_version_file()`` is the same as the one provided by the
# `CMakePackageConfigHelpers
# <http://www.cmake.org/cmake/help/v2.8.12/cmake.html#module:CMakePackageConfigHelpers>`_
# module in CMake; see that module's documentation for
# more information.
#
# ::
#
#   ecm_configure_package_config_file(<input> <output>
#       INSTALL_DESTINATION <path>
#       [PATH_VARS <var1> [<var2> [...]]
#       [NO_SET_AND_CHECK_MACRO]
#       [NO_CHECK_REQUIRED_COMPONENTS_MACRO])
#
#
# This behaves in the same way as configure_package_config_file() from CMake
# 2.8.12, except that it adds an extra helper macro: find_dependency(). It is
# highly recommended that you read the `documentation for
# CMakePackageConfigHelpers
# <http://www.cmake.org/cmake/help/v2.8.12/cmake.html#module:CMakePackageConfigHelpers>`_
# for more information, particularly with regard to the PATH_VARS argument.
#
# Note that there is no argument that will disable the find_dependency() macro;
# if you do not require this macro, you should use
# ``configure_package_config_file`` from the CMakePackageConfigHelpers module.
#
# CMake 3.0 includes a CMakeFindDependencyMacro module that provides the
# find_dependency() macro (which you can ``include()`` in your package config
# file), so this file is only useful for projects wishing to provide config
# files that will work with CMake 2.8.12.
#
# Additional Config File Macros
# =============================
#
# ::
#
#   find_dependency(<dep> [<version> [EXACT]])
#
# find_dependency() should be used instead of find_package() to find package
# dependencies.  It forwards the correct parameters for EXACT, QUIET and
# REQUIRED which were passed to the original find_package() call.  It also sets
# an informative diagnostic message if the dependency could not be found.
#
# Since pre-1.0.0.

#=============================================================================
# Copyright 2014 Alex Merry <alex.merry@kdemail.net>
# Copyright 2013 Stephen Kelly <steveire@gmail.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file COPYING-CMAKE-SCRIPTS for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of extra-cmake-modules, substitute the full
#  License text for the above reference.)

include(${CMAKE_ROOT}/Modules/CMakePackageConfigHelpers.cmake)

set(_ecm_package_config_helpers_included TRUE)

if(NOT CMAKE_MINIMUM_REQUIRED_VERSION VERSION_LESS 2.8.13)
    message(AUTHOR_WARNING "Your project already requires a version of CMake that includes the find_dependency macro via the CMakeFindDependencyMacro module. You should use CMakePackageConfigHelpers instead of ECMPackageConfigHelpers.")
endif()

function(ECM_CONFIGURE_PACKAGE_CONFIG_FILE _inputFile _outputFile)
  set(options NO_SET_AND_CHECK_MACRO NO_CHECK_REQUIRED_COMPONENTS_MACRO)
  set(oneValueArgs INSTALL_DESTINATION )
  set(multiValueArgs PATH_VARS )

  cmake_parse_arguments(CCF "${options}" "${oneValueArgs}" "${multiValueArgs}"  ${ARGN})

  if(CCF_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unknown keywords given to CONFIGURE_PACKAGE_CONFIG_FILE(): \"${CCF_UNPARSED_ARGUMENTS}\"")
  endif()

  if(NOT CCF_INSTALL_DESTINATION)
    message(FATAL_ERROR "No INSTALL_DESTINATION given to CONFIGURE_PACKAGE_CONFIG_FILE()")
  endif()

  if(IS_ABSOLUTE "${CCF_INSTALL_DESTINATION}")
    set(absInstallDir "${CCF_INSTALL_DESTINATION}")
  else()
    set(absInstallDir "${CMAKE_INSTALL_PREFIX}/${CCF_INSTALL_DESTINATION}")
  endif()

  file(RELATIVE_PATH PACKAGE_RELATIVE_PATH "${absInstallDir}" "${CMAKE_INSTALL_PREFIX}" )

  foreach(var ${CCF_PATH_VARS})
    if(NOT DEFINED ${var})
      message(FATAL_ERROR "Variable ${var} does not exist")
    else()
      if(IS_ABSOLUTE "${${var}}")
        string(REPLACE "${CMAKE_INSTALL_PREFIX}" "\${PACKAGE_PREFIX_DIR}"
                        PACKAGE_${var} "${${var}}")
      else()
        set(PACKAGE_${var} "\${PACKAGE_PREFIX_DIR}/${${var}}")
      endif()
    endif()
  endforeach()

  get_filename_component(inputFileName "${_inputFile}" NAME)

  set(PACKAGE_INIT "
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() (ECM variant) #######
####### Any changes to this file will be overwritten by the next CMake run            #######
####### The input file was ${inputFileName}                                           #######

get_filename_component(PACKAGE_PREFIX_DIR \"\${CMAKE_CURRENT_LIST_DIR}/${PACKAGE_RELATIVE_PATH}\" ABSOLUTE)
")

  if("${absInstallDir}" MATCHES "^(/usr)?/lib(64)?/.+")
    # Handle "/usr move" symlinks created by some Linux distros.
    set(PACKAGE_INIT "${PACKAGE_INIT}
# Use original install prefix when loaded through a \"/usr move\"
# cross-prefix symbolic link such as /lib -> /usr/lib.
get_filename_component(_realCurr \"\${CMAKE_CURRENT_LIST_DIR}\" REALPATH)
get_filename_component(_realOrig \"${absInstallDir}\" REALPATH)
if(_realCurr STREQUAL _realOrig)
  set(PACKAGE_PREFIX_DIR \"${CMAKE_INSTALL_PREFIX}\")
endif()
unset(_realOrig)
unset(_realCurr)
")
  endif()

  if(NOT CCF_NO_SET_AND_CHECK_MACRO)
    set(PACKAGE_INIT "${PACKAGE_INIT}
macro(set_and_check _var _file)
  set(\${_var} \"\${_file}\")
  if(NOT EXISTS \"\${_file}\")
    message(FATAL_ERROR \"File or directory \${_file} referenced by variable \${_var} does not exist !\")
  endif()
endmacro()

include(CMakeFindDependencyMacro OPTIONAL RESULT_VARIABLE _CMakeFindDependencyMacro_FOUND)

if (NOT _CMakeFindDependencyMacro_FOUND)
  macro(find_dependency dep)
    if (NOT \${dep}_FOUND)

      set(ecm_fd_version)
      if (\${ARGC} GREATER 1)
        set(ecm_fd_version \${ARGV1})
      endif()
      set(ecm_fd_exact_arg)
      if(\${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION_EXACT)
        set(ecm_fd_exact_arg EXACT)
      endif()
      set(ecm_fd_quiet_arg)
      if(\${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
        set(ecm_fd_quiet_arg QUIET)
      endif()
      set(ecm_fd_required_arg)
      if(\${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED)
        set(ecm_fd_required_arg REQUIRED)
      endif()

      find_package(\${dep} \${ecm_fd_version}
          \${ecm_fd_exact_arg}
          \${ecm_fd_quiet_arg}
          \${ecm_fd_required_arg}
      )

      if (NOT \${dep}_FOUND)
        set(\${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE \"\${CMAKE_FIND_PACKAGE_NAME} could not be found because dependency \${dep} could not be found.\")
        set(\${CMAKE_FIND_PACKAGE_NAME}_FOUND False)
        return()
      endif()

      set(ecm_fd_version)
      set(ecm_fd_required_arg)
      set(ecm_fd_quiet_arg)
      set(ecm_fd_exact_arg)
    endif()
  endmacro()
endif()

")
  endif()


  if(NOT CCF_NO_CHECK_REQUIRED_COMPONENTS_MACRO)
    set(PACKAGE_INIT "${PACKAGE_INIT}
macro(check_required_components _NAME)
  foreach(comp \${\${_NAME}_FIND_COMPONENTS})
    if(NOT \${_NAME}_\${comp}_FOUND)
      if(\${_NAME}_FIND_REQUIRED_\${comp})
        set(\${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()
")
  endif()

  set(PACKAGE_INIT "${PACKAGE_INIT}
####################################################################################")

  configure_file("${_inputFile}" "${_outputFile}" @ONLY)

endfunction()
