find_package(BCMHost REQUIRED)
find_package(EGL REQUIRED)

add_definitions(-DBACKEND_BCM_RPI=1)

list(APPEND WPE_PLATFORM_INCLUDE_DIRECTORIES
    ${BCM_HOST_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
)

list(APPEND WPE_PLATFORM_LIBRARIES
    ${BCM_HOST_LIBRARIES}
    ${EGL_LIBRARIES}
)

list(APPEND WPE_PLATFORM_SOURCES
    src/bcm-rpi/cursor-data.cpp
    src/bcm-rpi/renderer-backend.cpp
    src/bcm-rpi/view-backend.cpp
)
