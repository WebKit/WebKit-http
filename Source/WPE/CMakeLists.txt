#--- WPE

find_package(Libxkbcommon 0.4.0 REQUIRED)

set(WPE_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}/Source/WPE/Headers"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/wpe"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/wpe/external"
    ${LIBXKBCOMMON_INCLUDE_DIRS}
)

set(WPE_LIBRARIES
    ${LIBXKBCOMMON_LIBRARIES}
)

set(WPE_SOURCES
    Source/wpe/pasteboard.c
    Source/wpe/pasteboard-generic.cpp

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

#--- WPE-platform

find_package(GLIB 2.40.0 REQUIRED COMPONENTS gio gobject gthread gmodule)
find_package(LibDRM REQUIRED)
find_package(LibGBM REQUIRED)
find_package(Wayland 1.6.0 REQUIRED)
add_definitions(-DWPE_BACKEND_DRM=1)
add_definitions(-DWPE_BACKEND_WAYLAND=1)
add_definitions(-DWPE_BUFFER_MANAGEMENT_GBM=1)

if (USE_WPE_BACKEND_DRM_TEGRA)
    add_definitions(-DWPE_BACKEND_DRM_TEGRA=1)
endif ()

set(WPE_PLATFORM_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}/Source/WPE/Headers"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/wpe"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/wpe/drm"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/wpe/gbm"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/wpe/wayland"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/wpe/wayland/protocols"
    ${GIO_UNIX_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${LIBDRM_INCLUDE_DIRS}
    ${LIBGBM_INCLUDE_DIRS}
    ${WAYLAND_INCLUDE_DIRS}
)

set(WPE_PLATFORM_LIBRARIES
    ${GIO_UNIX_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${LIBDRM_LIBRARIES}
    ${LIBGBM_LIBRARIES}
    ${WAYLAND_LIBRARIES}
)

set(WPE_PLATFORM_SOURCES
    Source/wpe/mesa.cpp

    Source/wpe/drm/view-backend-drm.cpp

    Source/wpe/gbm/renderer-gbm.cpp
    Source/wpe/gbm/gbm-connection.cpp

    Source/wpe/wayland/display.cpp
    Source/wpe/wayland/pasteboard-wayland.cpp
    Source/wpe/wayland/view-backend-wayland.cpp

    Source/wpe/wayland/protocols/ivi-application-protocol.c
    Source/wpe/wayland/protocols/wayland-drm-protocol.c
    Source/wpe/wayland/protocols/xdg-shell-protocol.c
)

add_library(WPE-platform SHARED ${WPE_PLATFORM_SOURCES})
target_include_directories(WPE-platform PRIVATE ${WPE_PLATFORM_INCLUDE_DIRECTORIES})
target_link_libraries(WPE-platform ${WPE_PLATFORM_LIBRARIES})
install(TARGETS WPE-platform DESTINATION "${LIB_INSTALL_DIR}")
