find_package(GLIB 2.38.0 REQUIRED COMPONENTS gio gobject gthread gmodule)
find_package(Libinput REQUIRED)
find_package(Libudev REQUIRED)

set(USE_KEY_INPUT_HANDLING_LINUX_INPUT OFF CACHE BOOL "Whether to use <linux/input.h> event codes for keyboard event handling")

if (USE_WPE_VIRTUAL_KEYBOARD)
    find_package(LibVirtualKeyboard REQUIRED)
    add_definitions(-DKEY_INPUT_HANDLING_VIRTUAL=1)
endif()

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

if (USE_WPE_BACKEND_BCM_NEXUS)
    add_definitions(-DWPE_BACKEND_BCM_NEXUS=1)
endif ()

if (USE_WPE_BACKEND_BCM_RPI)
    find_package(BCMHost REQUIRED)
    add_definitions(-DWPE_BACKEND_BCM_RPI=1)
endif ()

if (USE_WPE_BACKEND_INTEL_CE)
    find_package(GDL REQUIRED)
    add_definitions(-DWPE_BACKEND_INTEL_CE=1)
endif ()

if (USE_WPE_BACKEND_STM)
    add_definitions(-DWPE_BACKEND_STM=1)
endif ()

if (USE_WPE_BACKEND_WESTEROS)
    find_package(westeros REQUIRED)
    add_definitions(-DWPE_BACKEND_WESTEROS=1)
endif ()

if (USE_WPE_BUFFER_MANAGEMENT_GBM)
    find_package(LibGBM REQUIRED)
    add_definitions(-DWPE_BUFFER_MANAGEMENT_GBM=1)
endif ()

if (USE_WPE_BUFFER_MANAGEMENT_BCM_RPI)
    add_definitions(-DWPE_BUFFER_MANAGEMENT_BCM_RPI=1)
endif ()

if (USE_WPE_BUFFER_MANAGEMENT_BCM_NEXUS)
    add_definitions(-DWPE_BUFFER_MANAGEMENT_BCM_NEXUS=1)
endif ()

set(WPE_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}/Source/WPE/Headers"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Graphics"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Graphics/BCMNexus"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Graphics/BCMRPi"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Graphics/GBM"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Graphics/IntelCE"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Graphics/Westeros"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Graphics/STM"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Input"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Pasteboard"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Pasteboard"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Pasteboard/Generic"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Pasteboard/Wayland"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/ViewBackend"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/ViewBackend/BCMNexus"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/ViewBackend/BCMRPi"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/ViewBackend/DRM"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/ViewBackend/IntelCE"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/ViewBackend/Wayland"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/ViewBackend/Wayland/Protocols"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/ViewBackend/Westeros"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/ViewBackend/STM"
    ${BCM_HOST_INCLUDE_DIRS}
    ${GDL_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${LIBINPUT_INCLUDE_DIRS}
    ${LIBUDEV_INCLUDE_DIRS}
    ${LIBXKBCOMMON_INCLUDE_DIRS}
    ${WAYLAND_INCLUDE_DIRS}
    ${WESTEROS_INCLUDE_DIRS}
)

set(WPE_LIBRARIES
    ${BCM_HOST_LIBRARIES}
    ${GDL_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${LIBINPUT_LIBRARIES}
    ${LIBUDEV_LIBRARIES}
    ${LIBXKBCOMMON_LIBRARIES}
    ${WAYLAND_LIBRARIES}
)

set(WPE_SOURCES
    Source/Graphics/RenderingBackend.cpp

    Source/Input/Handling.cpp
    Source/Input/KeyboardEventRepeating.cpp
    Source/Input/LibinputServer.cpp

    Source/Input/LinuxInput/KeyMappingLinuxInput.cpp
    Source/Input/LinuxInput/KeyboardEventHandlerLinuxInput.cpp

    Source/Input/XKB/KeyMappingXKB.cpp
    Source/Input/XKB/KeyboardEventHandlerXKB.cpp

    Source/Pasteboard/Pasteboard.cpp

    Source/Pasteboard/Generic/PasteboardGeneric.cpp

    Source/ViewBackend/ViewBackend.cpp
)

if (USE_WPE_BACKEND_BCM_NEXUS)
    list(APPEND WPE_SOURCES
        Source/Graphics/BCMNexus/BufferDataBCMNexus.cpp
        Source/Graphics/BCMNexus/RenderingBackendBCMNexus.cpp

        Source/ViewBackend/BCMNexus/ViewBackendBCMNexus.cpp
    )
    list(APPEND WPE_LIBRARIES nxclient)
endif ()

if (USE_WPE_BACKEND_BCM_RPI)
    list(APPEND WPE_SOURCES
        Source/Graphics/BCMRPi/BufferDataBCMRPi.cpp
        Source/Graphics/BCMRPi/RenderingBackendBCMRPi.cpp

        Source/ViewBackend/BCMRPi/ViewBackendBCMRPi.cpp
    )
    list(APPEND WPE_LIBRARIES nxclient)
endif ()

if (USE_WPE_BACKEND_INTEL_CE)
    list(APPEND WPE_SOURCES
        Source/Graphics/IntelCE/BufferDataIntelCE.cpp
        Source/Graphics/IntelCE/RenderingBackendIntelCE.cpp

        Source/ViewBackend/IntelCE/ViewBackendIntelCE.cpp
    )
endif ()

if (USE_WPE_VIRTUAL_KEYBOARD)
    list(APPEND WPE_LIBRARIES
        ${LIBVIRTUAL_KEYBOARD_LIBRARIES}
    )
endif()

if (USE_WPE_BACKEND_WESTEROS)
    list(APPEND WPE_INCLUDE_DIRECTORIES
        ${WESTEROS_INCLUDE_DIRS}
        ${WESTEROSEGL_INCLUDE_DIRS}
    )
    list(APPEND WPE_LIBRARIES
        ${WESTEROS_LIBRARIES}
        ${WAYLAND_EGL_LIBRARIES}
        ${WESTEROSEGL_LIBRARIES}
    )
    list(APPEND WPE_SOURCES
        Source/Graphics/Westeros/RenderingBackendWesteros.cpp

        Source/ViewBackend/Westeros/ViewBackendWesteros.cpp
        Source/ViewBackend/Westeros/WesterosViewbackendInput.cpp
        Source/ViewBackend/Westeros/WesterosViewbackendOutput.cpp
    )
endif ()

if (USE_WPE_BACKEND_STM)
     list(APPEND WPE_INCLUDE_DIRECTORIES
        ${STM_INCLUDE_DIRS}
        ${STMEGL_INCLUDE_DIRS}
     )
     list(APPEND WPE_LIBRARIES
        ${STM_LIBRARIES}
        ${STMEGL_LIBRARIES}
     )
     list(APPEND WPE_SOURCES
        Source/Graphics/STM/RenderingBackendSTM.cpp
        Source/Graphics/RenderingBackend.cpp
        Source/ViewBackend/STM/ViewBackendSTM.cpp
     )
endif ()

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

if (USE_WPE_BUFFER_MANAGEMENT_BCM_RPI)
    list(APPEND WPE_INCLUDE_DIRECTORIES
        "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Graphics/BCMRPiBM"
    )

    list(APPEND WPE_SOURCES
        Source/Graphics/BCMRPiBM/RenderingBackendBCMRPiBM.cpp
    )
endif ()

if (USE_WPE_BUFFER_MANAGEMENT_BCM_NEXUS)
    list(APPEND WPE_INCLUDE_DIRECTORIES
        "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Graphics/BCMNexusBM"
    )

    list(APPEND WPE_SOURCES
        Source/Graphics/BCMNexusBM/RenderingBackendBCMNexusBM.cpp
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
    list(APPEND WPE_INCLUDE_DIRECTORIES
        "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Graphics/Wayland"
    )
    list(APPEND WPE_SOURCES
        Source/Graphics/Wayland/BufferFactory.cpp

        Source/Pasteboard/Wayland/PasteboardWayland.cpp

        Source/ViewBackend/Wayland/ViewBackendWayland.cpp
        Source/ViewBackend/Wayland/WaylandDisplay.cpp

        Source/ViewBackend/Wayland/Protocols/ivi-application-protocol.c
        Source/ViewBackend/Wayland/Protocols/wayland-drm-protocol.c
        Source/ViewBackend/Wayland/Protocols/xdg-shell-protocol.c
        Source/ViewBackend/Wayland/Protocols/nsc-protocol.c
    )

    if (USE_WPE_BUFFER_MANAGEMENT_GBM)
        list(APPEND WPE_SOURCES
            Source/Graphics/Wayland/BufferFactoryWLDRM.cpp
        )
    endif ()

    if (USE_WPE_BUFFER_MANAGEMENT_BCM_RPI)
        list(APPEND WPE_SOURCES
            Source/Graphics/Wayland/BufferFactoryBCMRPi.cpp
        )
    endif ()

    if (USE_WPE_BUFFER_MANAGEMENT_BCM_NEXUS)
        list(APPEND WPE_SOURCES
            Source/Graphics/Wayland/BufferFactoryBCMNexus.cpp
        )
    endif ()
endif ()

include_directories(${WPE_INCLUDE_DIRECTORIES})
add_library(WPE SHARED ${WPE_SOURCES})
target_link_libraries(WPE ${WPE_LIBRARIES})

POPULATE_LIBRARY_VERSION(WPE)
set_target_properties(WPE PROPERTIES VERSION ${WPE_VERSION} SOVERSION ${WPE_VERSION_MAJOR})

install(TARGETS WPE DESTINATION "${LIB_INSTALL_DIR}")
