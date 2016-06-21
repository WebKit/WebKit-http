add_definitions(-DBACKEND_WESTEROS=1)

list(APPEND WPE_PLATFORM_SOURCES
    src/westeros/renderer-backend.cpp
    src/westeros/view-backend.cpp
)
