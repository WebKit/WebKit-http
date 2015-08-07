set(WPELAUNCHER_DIR "${TOOLS_DIR}/WPELauncher")

set(WPELauncher2_SOURCES
    ${WPELAUNCHER_DIR}/main.cpp
)

set(WPELauncher2_INCLUDE_DIRECTORIES
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/Source
    ${FORWARDING_HEADERS_DIR}
    ${GLIB_INCLUDE_DIRS}
)

set(WPELauncher2_SYSTEM_INCLUDE_DIRECTORIES
    ${GLIB_INCLUDE_DIRS}
)

set(WPELauncher2_LIBRARIES
    WebKit2
    ${GLIB_LIBRARIES}
)

include_directories(SYSTEM ${WPELauncher2_SYSTEM_INCLUDE_DIRECTORIES})
add_executable(WPELauncher2 ${WPELauncher2_SOURCES})
target_include_directories(WPELauncher2 PUBLIC ${WPELauncher2_INCLUDE_DIRECTORIES})
target_link_libraries(WPELauncher2 ${WPELauncher2_LIBRARIES})
