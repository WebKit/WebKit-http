set(USE_KEY_INPUT_HANDLING_LINUX_INPUT OFF CACHE BOOL "Whether to use <linux/input.h> event codes for keyboard event handling")

if (USE_KEY_INPUT_HANDLING_LINUX_INPUT)
    add_definitions(-DKEY_INPUT_HANDLING_LINUX_INPUT=1)
else ()
    find_package(Libxkbcommon 0.4.0 REQUIRED)
    add_definitions(-DKEY_INPUT_HANDLING_XKB=1)
endif ()

add_definitions(-DWPE_BACKEND_DRM=1)
add_definitions(-DWPE_BACKEND_WAYLAND=1)

set(WPE_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}/Source/WPE/Headers"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Input"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/ViewBackend"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/ViewBackend/DRM"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/ViewBackend/Wayland"
    ${GLIB_INCLUDE_DIRS}
    ${LIBGBM_INCLUDE_DIRS}
    ${LIBDRM_INCLUDE_DIRS}
    ${LIBXKBCOMMON_INCLUDE_DIRS}
    ${WAYLAND_INCLUDE_DIRS}
)

set(WPE_LIBRARIES
    ${GLIB_LIBRARIES}
    ${LIBDRM_LIBRARIES}
    ${LIBXKBCOMMON_LIBRARIES}
    ${WAYLAND_LIBRARIES}
)

set(WPE_SOURCES
    Source/Input/Handling.cpp
    Source/Input/KeyboardEventRepeating.cpp

    Source/Input/LinuxInput/KeyMappingLinuxInput.cpp
    Source/Input/LinuxInput/KeyboardEventHandlerLinuxInput.cpp

    Source/Input/XKB/KeyMappingXKB.cpp
    Source/Input/XKB/KeyboardEventHandlerXKB.cpp

    Source/ViewBackend/ViewBackend.cpp

    Source/ViewBackend/DRM/ViewBackendDRM.cpp

    Source/ViewBackend/Wayland/ViewBackendWayland.cpp
    Source/ViewBackend/Wayland/WaylandDisplay.cpp
    Source/ViewBackend/Wayland/wayland-drm-protocol.c
    Source/ViewBackend/Wayland/xdg-shell-protocol.c
)

include_directories(${WPE_INCLUDE_DIRECTORIES})
add_library(WPE SHARED ${WPE_SOURCES})
target_link_libraries(WPE ${WPE_LIBRARIES})
install(TARGETS WPE DESTINATION "${LIB_INSTALL_DIR}")
