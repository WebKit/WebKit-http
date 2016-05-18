find_package(BCMHost REQUIRED)

add_definitions(-DBACKEND_BCM_RPI=1)

list(APPEND WPE_METROLOGICAL_INCLUDE_DIRECTORIES
    ${BCM_HOST_INCLUDE_DIRS}
)

list(APPEND WPE_METROLOGICAL_LIBRARIES
    ${BCM_HOST_LIBRARIES}
)

list(APPEND WPE_METROLOGICAL_SOURCES
    src/rpi/renderer-backend.cpp
    src/rpi/view-backend.cpp
)
