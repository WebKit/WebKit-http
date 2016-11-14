set(WPE_MESA_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Source/ThirdParty/WPE-mesa")

option(WPE_MESA_GBM "Whether to enable support for the libgbm-based buffer sharing" ON)
option(WPE_MESA_EXPORTABLE_DMA_BUF "Whether to enable support for the exportable dma-buf view backend" ON)

option(WPE_MESA_EXPERIMENTAL_WAYLAND_EGL "Whether to enable support for the wayland-egl nested compositor-based buffer sharing" OFF)

option(WPE_MESA_DRM_TEGRA_SUPPORT "Whether to enable support for the Tegra-specific quirks in the DRM WPE backend" OFF)

find_package(GLIB 2.40.0 REQUIRED COMPONENTS gio gobject gthread gmodule)
find_package(LibDRM REQUIRED)
find_package(Libxkbcommon 0.4.0 REQUIRED)
find_package(Wayland 1.6.0 REQUIRED)
find_package(WaylandCursor REQUIRED)
add_definitions(-DWPE_BACKEND_DRM=1)
add_definitions(-DWPE_BACKEND_WAYLAND=1)

set(WPE_MESA_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}/Source/ThirdParty/WPE/include"
    "${WPE_MESA_SOURCE_DIR}/src/drm"
    "${WPE_MESA_SOURCE_DIR}/src/external"
    "${WPE_MESA_SOURCE_DIR}/src/libxkbcommon"
    "${WPE_MESA_SOURCE_DIR}/src/util"
    "${WPE_MESA_SOURCE_DIR}/src/wayland"
    "${WPE_MESA_SOURCE_DIR}/src/wayland/protocols"
    ${GIO_UNIX_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${LIBXKBCOMMON_INCLUDE_DIRS}
    ${WAYLAND_INCLUDE_DIRS}
    ${WAYLAND_CURSOR_INCLUDE_DIRS}
)
set(WPE_MESA_LIBRARIES
    ${GIO_UNIX_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${LIBXKBCOMMON_LIBRARIES}
    ${WAYLAND_LIBRARIES}
    ${WAYLAND_CURSOR_LIBRARIES}
)
set(WPE_MESA_SOURCES
    src/mesa.cpp

    src/libxkbcommon/input-libxkbcommon.cpp

    src/util/ipc.cpp

    src/wayland/display.cpp
    src/wayland/pasteboard-wayland.cpp

    src/wayland/protocols/ivi-application-protocol.c
    src/wayland/protocols/wayland-drm-protocol.c
    src/wayland/protocols/xdg-shell-protocol.c
)

if (WPE_MESA_GBM)
    add_definitions(-DWPE_MESA_GBM=1)
    find_package(LibGBM REQUIRED)

    list(APPEND WPE_MESA_INCLUDE_DIRECTORIES
        "${WPE_MESA_SOURCE_DIR}/src/gbm"
        ${LIBDRM_INCLUDE_DIRS}
        ${LIBGBM_INCLUDE_DIRS}
    )

    list(APPEND WPE_MESA_LIBRARIES
        ${LIBDRM_LIBRARIES}
        ${LIBGBM_LIBRARIES}
    )

    list(APPEND WPE_MESA_SOURCES
        src/drm/view-backend-drm.cpp

        src/gbm/renderer-backend-egl-gbm.cpp
        src/gbm/renderer-host-gbm.cpp

        src/wayland/view-backend-wayland.cpp
    )

    if (WPE_MESA_EXPORTABLE_DMA_BUF)
        list(APPEND WPE_MESA_INCLUDE_DIRECTORIES
            "${WPE_MESA_SOURCE_DIR}/include"
        )
        list(APPEND WPE_MESA_SOURCES
            src/exportable/view-backend-exportable-dma-buf.cpp
        )
    endif ()
endif ()

if (WPE_MESA_EXPERIMENTAL_WAYLAND_EGL)
    add_definitions(-DWPE_MESA_EXPERIMENTAL_WAYLAND_EGL=1)
    find_package(WaylandEGL REQUIRED)

    list(APPEND WPE_MESA_INCLUDE_DIRECTORIES
        "${WPE_MESA_SOURCE_DIR}/src/nc"
        ${LIBDRM_INCLUDE_DIRS}
        ${WAYLAND_EGL_INCLUDE_DIRS}
    )

    list(APPEND WPE_MESA_LIBRARIES
        ${LIBDRM_LIBRARIES}
        ${WAYLAND_EGL_LIBRARIES}
    )

    list(APPEND WPE_MESA_SOURCES
        src/nc/renderer-backend-egl.cpp
        src/nc/renderer-host.cpp
        src/nc/view-backend-drm.cpp
        src/nc/view-backend-wayland.cpp
        src/nc/nc-view-display.cpp
    )
endif ()

if (WPE_MESA_DRM_TEGRA_SUPPORT)
    add_definitions(-DWPE_BACKEND_DRM_TEGRA=1)
endif ()

add_library(WPE-mesa SHARED ${WPE_MESA_SOURCES})
target_include_directories(WPE-mesa PRIVATE ${WPE_MESA_INCLUDE_DIRECTORIES})
target_link_libraries(WPE-mesa ${WPE_MESA_LIBRARIES})
install(TARGETS WPE-mesa DESTINATION "${LIB_INSTALL_DIR}")

# Create a libWPE-backend.so symlink to libWPE-mesa.so and install it.
add_custom_command(TARGET WPE-mesa
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink libWPE-mesa.so ${CMAKE_BINARY_DIR}/lib/libWPE-backend.so
                  )
install(FILES ${CMAKE_BINARY_DIR}/lib/libWPE-backend.so DESTINATION "${LIB_INSTALL_DIR}")
