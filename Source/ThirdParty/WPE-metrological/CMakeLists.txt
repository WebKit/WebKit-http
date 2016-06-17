set(WPE_METROLOGICAL_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Source/ThirdParty/WPE-metrological")

# Converted
option(USE_WPE_BACKEND_BCM_NEXUS "Whether to enable support for the BCM_NEXUS WPE backend" OFF)
option(USE_WPE_BACKEND_BCM_RPI "Whether to enable support for the BCM_RPi WPE backend" OFF)
option(USE_WPE_BACKEND_INTEL_CE "Whether to enable support for the Intel CE WPE backend" OFF)
option(USE_WPE_BACKEND_STM "Whether to enable support for the STM WPE backend" OFF)
option(USE_WPE_BACKEND_WESTEROS "Whether to enable support for the Westeros WPE backend" OFF)

# TODO
option(USE_WPE_BACKEND_DRM "Whether to enable support for the DRM WPE backend" OFF)
option(USE_WPE_BACKEND_DRM_TEGRA "Whether to enable support for the Tegra-specific quirks in the DRM WPE backend" OFF)
option(USE_WPE_BACKEND_WAYLAND "Whether to enable support for the Wayland WPE backend" OFF)

set(USE_KEY_INPUT_HANDLING_LINUX_INPUT OFF CACHE BOOL "Whether to use <linux/input.h> event codes for keyboard event handling")

find_package(GLIB 2.38.0 REQUIRED COMPONENTS gio gio-unix)
find_package(Libinput REQUIRED)
find_package(Libudev REQUIRED)

if (USE_WPE_VIRTUAL_KEYBOARD)
    find_package(LibVirtualKeyboard REQUIRED)
    add_definitions(-DKEY_INPUT_HANDLING_VIRTUAL=1)
endif()

if (USE_KEY_INPUT_HANDLING_LINUX_INPUT)
    add_definitions(-DKEY_INPUT_HANDLING_LINUX_INPUT=1)
else ()
    find_package(Libxkbcommon 0.4.0 REQUIRED)
    add_definitions(-DKEY_INPUT_HANDLING_XKB=1)
endif ()

set(WPE_METROLOGICAL_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}/Source/ThirdParty/WPE/include"
    "${WPE_METROLOGICAL_SOURCE_DIR}/src/external"
    "${WPE_METROLOGICAL_SOURCE_DIR}/src/input"
    "${WPE_METROLOGICAL_SOURCE_DIR}/src/util"
    ${GIO_UNIX_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${LIBINPUT_INCLUDE_DIRS}
    ${LIBUDEV_INCLUDE_DIRS}
)

set(WPE_METROLOGICAL_LIBRARIES
    ${GLIB_GIO_LIBRARIES}
    ${LIBINPUT_LIBRARIES}
    ${LIBUDEV_LIBRARIES}
)

if (NOT USE_KEY_INPUT_HANDLING_LINUX_INPUT)
    list(APPEND WPE_METROLOGICAL_INCLUDE_DIRECTORIES
        ${LIBXKBCOMMON_INCLUDE_DIRS}
    )
    list(APPEND WPE_METROLOGICAL_LIBRARIES
        ${LIBXKBCOMMON_LIBRARIES}
    )
endif ()

set(WPE_METROLOGICAL_SOURCES
    src/metrological.cpp

    src/input/KeyboardEventRepeating.cpp
    src/input/LibinputServer.cpp

    src/input/LinuxInput/KeyboardEventHandlerLinuxInput.cpp
    src/input/LinuxInput/input-linuxinput.cpp

    src/input/XKB/KeyboardEventHandlerXKB.cpp
    src/input/XKB/input-libxkbcommon.cpp

    src/util/ipc.cpp
)

if (USE_WPE_BACKEND_BCM_NEXUS)
    include(src/bcm-nexus/CMakeLists.txt)
endif ()

if (USE_WPE_BACKEND_BCM_RPI)
    include(src/bcm-rpi/CMakeLists.txt)
endif ()

if (USE_WPE_BACKEND_INTEL_CE)
    include(src/intelce/CMakeLists.txt)
endif ()

if (USE_WPE_BACKEND_STM)
    include(src/stm/CMakeLists.txt)
endif ()

if (USE_WPE_BACKEND_WESTEROS)
    include(src/westeros/CMakeLists.txt)
endif ()

add_library(WPE-metrological SHARED ${WPE_METROLOGICAL_SOURCES})
target_include_directories(WPE-metrological PRIVATE ${WPE_METROLOGICAL_INCLUDE_DIRECTORIES})
target_link_libraries(WPE-metrological ${WPE_METROLOGICAL_LIBRARIES})
install(TARGETS WPE-metrological DESTINATION "${LIB_INSTALL_DIR}")
