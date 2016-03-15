set(WPE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Source/ThirdParty/WPE")

find_package(Libxkbcommon 0.4.0 REQUIRED)

set(WPE_INCLUDE_DIRECTORIES
    "${WPE_SOURCE_DIR}/Headers"
    "${WPE_SOURCE_DIR}/Source/wpe"
    "${WPE_SOURCE_DIR}/Source/wpe/external"
    ${LIBXKBCOMMON_INCLUDE_DIRS}
)

set(WPE_LIBRARIES
    ${LIBXKBCOMMON_LIBRARIES}
)

set(WPE_SOURCES
    Source/wpe/pasteboard.c
    Source/wpe/pasteboard-generic.cpp
    Source/wpe/pasteboard-noop.cpp

    Source/wpe/input.c
    Source/wpe/loader.c
    Source/wpe/renderer.c
    Source/wpe/view-backend.c
)

add_library(WPE SHARED ${WPE_SOURCES})
target_include_directories(WPE PRIVATE ${WPE_INCLUDE_DIRECTORIES})
target_link_libraries(WPE ${WPE_LIBRARIES})

POPULATE_LIBRARY_VERSION(WPE)
set_target_properties(WPE PROPERTIES VERSION ${WPE_VERSION} SOVERSION ${WPE_VERSION_MAJOR})

install(TARGETS WPE DESTINATION "${LIB_INSTALL_DIR}")
