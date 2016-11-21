# Install script for directory: /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-9d4c9c472778d89386fa3533f95f6b8e8c203894/Tools/WPELauncher

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  if(EXISTS "$ENV{DESTDIR}/usr/bin/WPELauncher" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/bin/WPELauncher")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/usr/bin/WPELauncher"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/bin/WPELauncher")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/bin" TYPE EXECUTABLE FILES "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-9d4c9c472778d89386fa3533f95f6b8e8c203894/build-Release/bin/WPELauncher")
  if(EXISTS "$ENV{DESTDIR}/usr/bin/WPELauncher" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/bin/WPELauncher")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}/usr/bin/WPELauncher"
         OLD_RPATH "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-9d4c9c472778d89386fa3533f95f6b8e8c203894/build-Release/lib:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/home/naveen/workspace/ML_03/buildroot-wpe/output/host/usr/bin/arm-buildroot-linux-gnueabihf-strip" "$ENV{DESTDIR}/usr/bin/WPELauncher")
    endif()
  endif()
endif()

