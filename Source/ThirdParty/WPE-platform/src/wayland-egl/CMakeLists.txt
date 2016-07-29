add_definitions(-DBACKEND_WAYLAND_EGL=1)

list(APPEND WPE_PLATFORM_SOURCES
    src/wayland-egl/renderer-backend.cpp
    src/wayland-egl/view-backend.cpp
)
