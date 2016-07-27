set(WPELAUNCHER_DIR "${TOOLS_DIR}/wpe/examples/launcher")

set(WPELauncher_SOURCES
    ${WPELAUNCHER_DIR}/main.cpp
)

set(WPELauncher_INCLUDE_DIRECTORIES
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/Source
    ${FORWARDING_HEADERS_DIR}
    ${WTF_DIR}
)

set(WPELauncher_SYSTEM_INCLUDE_DIRECTORIES
    ${GLIB_INCLUDE_DIRS}
)

set(WPELauncher_LIBRARIES
    WebKit2
    ${GLIB_LIBRARIES}
)

include_directories(SYSTEM ${WPELauncher_SYSTEM_INCLUDE_DIRECTORIES})
add_executable(WPELauncher ${WPELauncher_SOURCES})
target_include_directories(WPELauncher PUBLIC ${WPELauncher_INCLUDE_DIRECTORIES})
target_link_libraries(WPELauncher ${WPELauncher_LIBRARIES})
install(TARGETS WPELauncher DESTINATION "${EXEC_INSTALL_DIR}")
