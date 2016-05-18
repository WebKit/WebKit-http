set(WPE_METROLOGICAL_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Source/ThirdParty/WPE-metrological")


# Converted
option(USE_WPE_BACKEND_BCM_NEXUS "Whether to enable support for the BCM_NEXUS WPE backend" OFF)
option(USE_WPE_BACKEND_BCM_RPI "Whether to enable support for the BCM_RPi WPE backend" OFF)
option(USE_WPE_BACKEND_INTEL_CE "Whether to enable support for the Intel CE WPE backend" OFF)

# TODO
option(USE_WPE_BACKEND_DRM "Whether to enable support for the DRM WPE backend" OFF)
option(USE_WPE_BACKEND_DRM_TEGRA "Whether to enable support for the Tegra-specific quirks in the DRM WPE backend" OFF)
option(USE_WPE_BACKEND_WAYLAND "Whether to enable support for the Wayland WPE backend" OFF)
option(USE_WPE_BACKEND_WESTEROS "Whether to enable support for the Westeros WPE backend" OFF)


set(WPE_METROLOGICAL_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}/Source/ThirdParty/WPE/Headers"
)

set(WPE_METROLOGICAL_LIBRARIES
)

set(WPE_METROLOGICAL_SOURCES
    src/metrological.cpp
)

if (USE_WPE_BACKEND_BCM_NEXUS)
    include(src/bcm-nexus/CMakeLists.txt)
endif ()

if (USE_WPE_BACKEND_BCM_RPI)
    include(src/bcm-rpi/CMakeLists.txt)
endif ()

if (USE_WPE_BACKEND_INTEL_CE)
    include(src/intelce/CMakeLists.txt)
endif ()

add_library(WPE-metrological SHARED ${WPE_METROLOGICAL_SOURCES})
target_include_directories(WPE-metrological PRIVATE ${WPE_METROLOGICAL_INCLUDE_DIRECTORIES})
target_link_libraries(WPE-metrological ${WPE_METROLOGICAL_LIBRARIES})
install(TARGETS WPE-metrological DESTINATION "${LIB_INSTALL_DIR}")
