add_definitions(-DBACKEND_BCM_NEXUS_WAYLAND=1)

list(APPEND WPE_PLATFORM_INCLUDE_DIRECTORIES
    "${WPE_PLATFORM_SOURCE_DIR}/src/bcm-nexus-wayland"
    "${WPE_PLATFORM_SOURCE_DIR}/src/wayland"
    "${WPE_PLATFORM_SOURCE_DIR}/src/wayland/protocols"
    ${WAYLAND_INCLUDE_DIRS}
)

list(APPEND WPE_PLATFORM_LIBRARIES
    ${WAYLAND_LIBRARIES}
)

list(APPEND WPE_PLATFORM_SOURCES
    src/bcm-nexus-wayland/renderer-backend.cpp
    src/bcm-nexus-wayland/view-backend.cpp
    src/wayland/protocols/nsc-protocol.c
    src/wayland/protocols/xdg-shell-protocol.c
    src/wayland/display.cpp
)
