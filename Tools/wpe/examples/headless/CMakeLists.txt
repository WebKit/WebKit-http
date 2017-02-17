find_package(WPE-mesa REQUIRED)

set(WPEHEADLESS_DIR "${TOOLS_DIR}/wpe/examples/headless")

set(WPEHeadless_SOURCES
    ${WPEHEADLESS_DIR}/main.cpp
)

set(WPEHeadless_INCLUDE_DIRECTORIES
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/Source
    ${FORWARDING_HEADERS_DIR}
    ${WTF_DIR}
)

set(WPEHeadless_SYSTEM_INCLUDE_DIRECTORIES
    ${GLIB_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${WPE_MESA_INCLUDE_DIRS}
)

set(WPEHeadless_LIBRARIES
    WebKit2
    ${GLIB_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${WPE_MESA_LIBRARIES}
)

include_directories(SYSTEM ${WPEHeadless_SYSTEM_INCLUDE_DIRECTORIES})
add_executable(WPEHeadless ${WPEHeadless_SOURCES})
target_include_directories(WPEHeadless PUBLIC ${WPEHeadless_INCLUDE_DIRECTORIES})
target_link_libraries(WPEHeadless ${WPEHeadless_LIBRARIES})
install(TARGETS WPEHeadless DESTINATION "${EXEC_INSTALL_DIR}")
