find_package(Wayland REQUIRED)
find_package(WaylandEGL REQUIRED)

add_definitions(-DBACKEND_WAYLAND_EGL=1)

list(APPEND WPE_PLATFORM_INCLUDE_DIRECTORIES
    "${WPE_PLATFORM_SOURCE_DIR}/src/wayland"
    "${WPE_PLATFORM_SOURCE_DIR}/src/wayland/protocols"
    ${WAYLAND_INCLUDE_DIRS}
    ${WAYLAND_EGL_INCLUDE_DIRS}
)

list(APPEND WPE_PLATFORM_LIBRARIES
    ${WAYLAND_LIBRARIES}
    ${WAYLAND_EGL_LIBRARIES}
)

list(APPEND WPE_PLATFORM_SOURCES
    src/wayland-egl/renderer-backend.cpp
    src/wayland-egl/view-backend.cpp
    src/wayland/protocols/xdg-shell-protocol.c
    src/wayland/display.cpp
)
