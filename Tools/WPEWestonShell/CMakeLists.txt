set(WPEWestonShell_SOURCES
    Environment.cpp
    Module.cpp
    Shell.cpp
)

set(WPEWestonShell_INCLUDE_DIRECTORIES
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/Source
    ${FORWARDING_HEADERS_DIR}
    ${GLIB_INCLUDE_DIRS}
    ${WESTON_INCLUDE_DIRS}
    ${WPE_DIR}
)

set(WPEWestonShell_LIBRARIES
    WPE
    WebKit2
    ${GLIB_LIBRARIES}
    ${WESTON_LIBRARIES}
)

add_library(WPEWestonShell SHARED ${WPEWestonShell_SOURCES})
target_link_libraries(WPEWestonShell ${WPEWestonShell_LIBRARIES})
target_include_directories(WPEWestonShell PUBLIC ${WPEWestonShell_INCLUDE_DIRECTORIES})
install(TARGETS WPEWestonShell DESTINATION "${LIB_INSTALL_DIR}")
