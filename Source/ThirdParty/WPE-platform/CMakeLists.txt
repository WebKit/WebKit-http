set(WPE_PLATFORM_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Source/ThirdParty/WPE-platform")

# Converted
option(USE_WPEWEBKIT_BACKEND_BCM_NEXUS "Whether to enable support for the BCM_NEXUS WPE backend" OFF)
option(USE_WPEWEBKIT_BACKEND_BCM_NEXUS_WAYLAND "Whether to enable support for the BCM_NEXUS Wayland WPE backend" OFF)
option(USE_WPEWEBKIT_BACKEND_BCM_RPI "Whether to enable support for the BCM_RPi WPE backend" OFF)
option(USE_WPEWEBKIT_BACKEND_INTEL_CE "Whether to enable support for the Intel CE WPE backend" OFF)
option(USE_WPEWEBKIT_BACKEND_WAYLAND_EGL "Whether to enable support for the wayland-egl WPE backend" OFF)
option(USE_WPEWEBKIT_BACKEND_WESTEROS "Whether to enable support for the Westeros WPE backend" OFF)

option(USE_PLATFORM_BROADCOM "Whether the playback is based on Broadcom plugins" OFF)
option(USE_WPEWEBKIT_BACKEND_WESTEROS_MESA "Whether to enable support for the gbm based offscreen target for westeros Mesa only" OFF)

# TODO
option(USE_WPEWEBKIT_BACKEND_DRM "Whether to enable support for the DRM WPE backend" OFF)
option(USE_WPEWEBKIT_BACKEND_DRM_TEGRA "Whether to enable support for the Tegra-specific quirks in the DRM WPE backend" OFF)
option(USE_WPEWEBKIT_BACKEND_WAYLAND "Whether to enable support for the Wayland WPE backend" OFF)

option(USE_WPEWEBKIT_INPUT_LIBINPUT "Whether to enable support for the libinput input backend" ON)
option(USE_WPEWEBKIT_INPUT_UDEV "Whether to enable support for the libinput input udev lib" ON)
set(USE_KEY_INPUT_HANDLING_LINUX_INPUT OFF CACHE BOOL "Whether to use <linux/input.h> event codes for keyboard event handling")

find_package(GLIB 2.38.0 REQUIRED COMPONENTS gio gio-unix)

if (USE_WPEWEBKIT_VIRTUAL_KEYBOARD)
    find_package(LibVirtualKeyboard REQUIRED)
    add_definitions(-DKEY_INPUT_HANDLING_VIRTUAL=1)
endif()

if (USE_KEY_INPUT_HANDLING_LINUX_INPUT)
    add_definitions(-DKEY_INPUT_HANDLING_LINUX_INPUT=1)
else ()
    find_package(Libxkbcommon 0.4.0 REQUIRED)
    add_definitions(-DKEY_INPUT_HANDLING_XKB=1)
endif ()

if (USE_WPEWEBKIT_BACKEND_WESTEROS_MESA)
   add_definitions(-DWPE_BACKEND_MESA=1)
endif ()

set(WPE_PLATFORM_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}/Source/ThirdParty/WPE/include"
    "${WPE_PLATFORM_SOURCE_DIR}/src/external"
    "${WPE_PLATFORM_SOURCE_DIR}/src/input"
    "${WPE_PLATFORM_SOURCE_DIR}/src/util"
    ${GIO_UNIX_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
)

set(WPE_PLATFORM_LIBRARIES
    ${WPE_LIBRARIES}
    ${GLIB_GIO_LIBRARIES}
    ${GLIB_GOBJECT_LIBRARIES}
    ${GLIB_LIBRARIES}
)

if (NOT USE_KEY_INPUT_HANDLING_LINUX_INPUT)
    list(APPEND WPE_PLATFORM_INCLUDE_DIRECTORIES
        ${LIBXKBCOMMON_INCLUDE_DIRS}
    )
    list(APPEND WPE_PLATFORM_LIBRARIES
        ${LIBXKBCOMMON_LIBRARIES}
    )
endif ()

if (USE_WPEWEBKIT_VIRTUAL_KEYBOARD)
    list(APPEND WPE_PLATFORM_LIBRARIES
        ${LIBVIRTUAL_KEYBOARD_LIBRARIES}
    )
endif ()

set(WPE_PLATFORM_SOURCES
    src/loader-impl.cpp

    src/input/LinuxInput/KeyboardEventHandlerLinuxInput.cpp
    src/input/LinuxInput/input-linuxinput.cpp

    src/input/XKB/KeyboardEventHandlerXKB.cpp
    src/input/XKB/input-libxkbcommon.cpp

    src/util/ipc.cpp
)

if (USE_WPEWEBKIT_INPUT_LIBINPUT)
    find_package(Libinput REQUIRED)
    list(APPEND WPE_PLATFORM_INCLUDE_DIRECTORIES
        ${LIBINPUT_INCLUDE_DIRS}
    )
    list(APPEND WPE_PLATFORM_LIBRARIES
        ${LIBINPUT_LIBRARIES}
    )
    if (USE_WPEWEBKIT_INPUT_UDEV)
        find_package(Libudev REQUIRED)
        list(APPEND WPE_PLATFORM_INCLUDE_DIRECTORIES
            ${LIBUDEV_INCLUDE_DIRS}
        )
        list(APPEND WPE_PLATFORM_LIBRARIES
            ${LIBUDEV_LIBRARIES}
        )
        add_definitions(-DKEY_INPUT_UDEV=1)
    endif ()
    list(APPEND WPE_PLATFORM_SOURCES
        src/input/Libinput/KeyboardEventRepeating.cpp
        src/input/Libinput/LibinputServer.cpp
    )
endif()

if (USE_WPEWEBKIT_BACKEND_BCM_NEXUS)
    include(src/bcm-nexus/CMakeLists.txt)
endif ()

if (USE_WPEWEBKIT_BACKEND_BCM_NEXUS_WAYLAND)
    include(src/bcm-nexus-wayland/CMakeLists.txt)
endif ()

if (USE_WPEWEBKIT_BACKEND_BCM_RPI)
    include(src/bcm-rpi/CMakeLists.txt)
endif ()

if (USE_WPEWEBKIT_BACKEND_INTEL_CE)
    include(src/intelce/CMakeLists.txt)
endif ()

if (USE_WPEWEBKIT_BACKEND_WAYLAND_EGL)
    include(src/wayland-egl/CMakeLists.txt)
endif ()

if (USE_WPEWEBKIT_BACKEND_WESTEROS)
    include(src/westeros/CMakeLists.txt)
endif ()

add_library(WPEBackend-rdk SHARED ${WPE_PLATFORM_SOURCES})
target_include_directories(WPEBackend-rdk PRIVATE ${WPE_PLATFORM_INCLUDE_DIRECTORIES})
target_link_libraries(WPEBackend-rdk ${WPE_PLATFORM_LIBRARIES})
install(TARGETS WPEBackend-rdk DESTINATION "${LIB_INSTALL_DIR}")

# Create a libWPEBackend-default.so symlink to libWPEBackend-rdk.so and install it.
add_custom_command(TARGET WPEBackend-rdk
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E create_symlink libWPEBackend-rdk.so ${CMAKE_BINARY_DIR}/lib/libWPEBackend-default.so
  )
install(FILES ${CMAKE_BINARY_DIR}/lib/libWPEBackend-default.so DESTINATION lib)
