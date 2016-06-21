add_definitions(-DBACKEND_BCM_NEXUS=1)

list(APPEND WPE_PLATFORM_SOURCES
    src/bcm-nexus/renderer-backend.cpp
    src/bcm-nexus/view-backend.cpp
)
