find_package(GDL REQUIRED)

add_definitions(-DBACKEND_INTELCE=1)

list(APPEND WPE_METROLOGICAL_INCLUDE_DIRECTORIES
    ${GDL_INCLUDE_DIRS}
)

list(APPEND WPE_METROLOGICAL_LIBRARIES
    ${GDL_LIBRARIES}
)

list(APPEND WPE_METROLOGICAL_SOURCES
    src/intelce/renderer-backend.cpp
    src/intelce/view-backend.cpp
)
