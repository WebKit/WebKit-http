add_definitions(-DBACKEND_STM=1)

list(APPEND WPE_PLATFORM_SOURCES
    src/stm/renderer-backend.cpp
    src/stm/view-backend.cpp
)
