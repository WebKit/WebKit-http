add_definitions(-DBACKEND_BCM_NEXUS_WAYLAND=1)

list(APPEND WPE_METROLOGICAL_SOURCES
    src/bcm-nexus-wayland/renderer-backend.cpp
    src/bcm-nexus-wayland/view-backend.cpp
)
