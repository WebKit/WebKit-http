set(WPE_MESA_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Source/ThirdParty/WPE-mesa")

option(WPE_MESA_DRM_TEGRA_SUPPORT "Whether to enable support for the Tegra-specific quirks in the DRM WPE backend" OFF)

find_package(GLIB 2.40.0 REQUIRED COMPONENTS gio gobject gthread gmodule)
find_package(LibDRM REQUIRED)
find_package(LibGBM REQUIRED)
find_package(Wayland 1.6.0 REQUIRED)
add_definitions(-DWPE_BACKEND_DRM=1)
add_definitions(-DWPE_BACKEND_WAYLAND=1)
add_definitions(-DWPE_BUFFER_MANAGEMENT_GBM=1)

if (WPE_MESA_DRM_TEGRA_SUPPORT)
    add_definitions(-DWPE_BACKEND_DRM_TEGRA=1)
endif ()

set(WPE_MESA_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}/Source/ThirdParty/WPE/Headers"
    "${WPE_MESA_SOURCE_DIR}/Source/wpe"
    "${WPE_MESA_SOURCE_DIR}/Source/wpe/drm"
    "${WPE_MESA_SOURCE_DIR}/Source/wpe/gbm"
    "${WPE_MESA_SOURCE_DIR}/Source/wpe/wayland"
    "${WPE_MESA_SOURCE_DIR}/Source/wpe/wayland/protocols"
    ${GIO_UNIX_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${LIBDRM_INCLUDE_DIRS}
    ${LIBGBM_INCLUDE_DIRS}
    ${WAYLAND_INCLUDE_DIRS}
)

set(WPE_MESA_LIBRARIES
    ${GIO_UNIX_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${LIBDRM_LIBRARIES}
    ${LIBGBM_LIBRARIES}
    ${WAYLAND_LIBRARIES}
)

set(WPE_MESA_SOURCES
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

add_library(WPE-mesa SHARED ${WPE_MESA_SOURCES})
target_include_directories(WPE-mesa PRIVATE ${WPE_MESA_INCLUDE_DIRECTORIES})
target_link_libraries(WPE-mesa ${WPE_MESA_LIBRARIES})
install(TARGETS WPE-mesa DESTINATION "${LIB_INSTALL_DIR}")
