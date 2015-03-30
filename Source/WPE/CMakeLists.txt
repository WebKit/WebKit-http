set(USE_KEY_INPUT_HANDLING_LINUX_INPUT OFF CACHE BOOL "Whether to use <linux/input.h> event codes for keyboard event handling")

if (USE_KEY_INPUT_HANDLING_LINUX_INPUT)
    add_definitions(-DKEY_INPUT_HANDLING_LINUX_INPUT=1)
else ()
    find_package(Libxkbcommon 0.4.0 REQUIRED)
    add_definitions(-DKEY_INPUT_HANDLING_XKB=1)
endif ()

set(WPE_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}/Source/WPE/Headers"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Input"
    ${LIBXKBCOMMON_INCLUDE_DIRS}
)

set(WPE_LIBRARIES
    ${LIBXKBCOMMON_LIBRARIES}
)

set(WPE_SOURCES
    Source/Input/Handling.cpp

    Source/Input/LinuxInput/KeyMappingLinuxInput.cpp
    Source/Input/LinuxInput/KeyboardEventHandlerLinuxInput.cpp

    Source/Input/XKB/KeyMappingXKB.cpp
    Source/Input/XKB/KeyboardEventHandlerXKB.cpp
)

include_directories(${WPE_INCLUDE_DIRECTORIES})
add_library(WPE STATIC ${WPE_SOURCES})
target_link_libraries(WPE ${WPE_LIBRARIES})
