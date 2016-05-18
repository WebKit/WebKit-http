add_definitions(-DBACKEND_INTELCE=1)

list(APPEND WPE_METROLOGICAL_SOURCES
    src/intelce/renderer-backend.cpp
    src/intelce/view-backend.cpp
)
