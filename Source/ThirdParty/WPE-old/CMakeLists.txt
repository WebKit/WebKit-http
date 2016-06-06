find_package(GLIB 2.40.0 REQUIRED COMPONENTS gio gobject gthread gmodule)

set(USE_KEY_INPUT_HANDLING_LINUX_INPUT OFF CACHE BOOL "Whether to use <linux/input.h> event codes for keyboard event handling")

if (USE_KEY_INPUT_HANDLING_LINUX_INPUT)
    add_definitions(-DKEY_INPUT_HANDLING_LINUX_INPUT=1)
else ()
    find_package(Libxkbcommon 0.4.0 REQUIRED)
    add_definitions(-DKEY_INPUT_HANDLING_XKB=1)
endif ()

if (USE_WPE_BACKEND_DRM)
    find_package(LibDRM REQUIRED)
    add_definitions(-DWPE_BACKEND_DRM=1)

    if (USE_WPE_BACKEND_DRM_TEGRA)
        add_definitions(-DWPE_BACKEND_DRM_TEGRA=1)
    endif ()
endif ()

if (USE_WPE_BACKEND_WAYLAND)
    find_package(Wayland 1.6.0 REQUIRED)
    add_definitions(-DWPE_BACKEND_WAYLAND=1)
endif ()

if (USE_WPE_BUFFER_MANAGEMENT_GBM)
    find_package(LibGBM REQUIRED)
    add_definitions(-DWPE_BUFFER_MANAGEMENT_GBM=1)
endif ()

set(WPE_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}/Source/WPE/Headers"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Graphics"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Input"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Pasteboard"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Pasteboard"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Pasteboard/Generic"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Pasteboard/Wayland"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/ViewBackend"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/ViewBackend/Wayland"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/ViewBackend/Wayland/Protocols"
    ${GLIB_INCLUDE_DIRS}
    ${LIBXKBCOMMON_INCLUDE_DIRS}
    ${WAYLAND_INCLUDE_DIRS}
)

set(WPE_LIBRARIES
    ${GLIB_LIBRARIES}
    ${LIBXKBCOMMON_LIBRARIES}
    ${WAYLAND_LIBRARIES}
)

set(WPE_SOURCES
    Source/Graphics/RenderingBackend.cpp

    Source/Input/Handling.cpp
    Source/Input/KeyboardEventRepeating.cpp

    Source/Input/LinuxInput/KeyMappingLinuxInput.cpp
    Source/Input/LinuxInput/KeyboardEventHandlerLinuxInput.cpp

    Source/Input/XKB/KeyMappingXKB.cpp
    Source/Input/XKB/KeyboardEventHandlerXKB.cpp

    Source/Pasteboard/Pasteboard.cpp

    Source/Pasteboard/Generic/PasteboardGeneric.cpp

    Source/ViewBackend/ViewBackend.cpp
)

if (USE_WPE_BUFFER_MANAGEMENT_GBM)
    list(APPEND WPE_INCLUDE_DIRECTORIES
        "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Graphics/GBM"
        ${LIBGBM_INCLUDE_DIRS}
    )
    list(APPEND WPE_LIBRARIES
        ${LIBGBM_LIBRARIES}
    )
    list(APPEND WPE_SOURCES
        Source/Graphics/GBM/BufferDataGBM.cpp
        Source/Graphics/GBM/RenderingBackendGBM.cpp
    )
endif ()

if (USE_WPE_BACKEND_DRM)
    list(APPEND WPE_INCLUDE_DIRECTORIES
        "${CMAKE_SOURCE_DIR}/Source/WPE/Source/ViewBackend/DRM"
        ${LIBDRM_INCLUDE_DIRS}
    )
    list(APPEND WPE_LIBRARIES
        ${LIBDRM_LIBRARIES}
    )
    list(APPEND WPE_SOURCES
        Source/ViewBackend/DRM/ViewBackendDRM.cpp
    )
endif ()

if (USE_WPE_BACKEND_WAYLAND)
    list(APPEND WPE_SOURCES
        Source/Pasteboard/Wayland/PasteboardWayland.cpp

        Source/ViewBackend/Wayland/ViewBackendWayland.cpp
        Source/ViewBackend/Wayland/WaylandDisplay.cpp

        Source/ViewBackend/Wayland/Protocols/ivi-application-protocol.c
        Source/ViewBackend/Wayland/Protocols/wayland-drm-protocol.c
        Source/ViewBackend/Wayland/Protocols/xdg-shell-protocol.c
    )
endif ()

include_directories(${WPE_INCLUDE_DIRECTORIES})
add_library(WPE SHARED ${WPE_SOURCES})
target_link_libraries(WPE ${WPE_LIBRARIES})

POPULATE_LIBRARY_VERSION(WPE)
set_target_properties(WPE PROPERTIES VERSION ${WPE_VERSION} SOVERSION ${WPE_VERSION_MAJOR})

install(TARGETS WPE DESTINATION "${LIB_INSTALL_DIR}")
