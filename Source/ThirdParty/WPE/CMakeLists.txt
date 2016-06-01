set(WPE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Source/ThirdParty/WPE")

find_package(Libxkbcommon 0.4.0 REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

set(WPE_INCLUDE_DIRECTORIES
    "${WPE_SOURCE_DIR}/include"
    "${WPE_SOURCE_DIR}/src"
    "${WPE_SOURCE_DIR}/src/external"
    ${LIBXKBCOMMON_INCLUDE_DIRS}
)

set(WPE_LIBRARIES
    ${LIBXKBCOMMON_LIBRARIES}
)

set(WPE_SOURCES
    src/input.c
    src/loader.c
    src/pasteboard.c
    src/pasteboard-generic.cpp
    src/pasteboard-noop.cpp
    src/renderer.c
    src/view-backend.c
)

add_library(WPE SHARED ${WPE_SOURCES})
target_include_directories(WPE PRIVATE ${WPE_INCLUDE_DIRECTORIES})
target_link_libraries(WPE ${WPE_LIBRARIES})

POPULATE_LIBRARY_VERSION(WPE)
set_target_properties(WPE PROPERTIES VERSION ${WPE_VERSION} SOVERSION ${WPE_VERSION_MAJOR})

install(TARGETS WPE DESTINATION "${LIB_INSTALL_DIR}")
