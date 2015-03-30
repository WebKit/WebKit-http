set(WPE_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}/Source/WPE/Headers"
    "${CMAKE_SOURCE_DIR}/Source/WPE/Source/Input"
)

set(WPE_SOURCES
    Source/Input/LinuxInput/KeyMappingLinuxInput.cpp

    Source/Input/XKB/KeyMappingXKB.cpp
)

add_definitions(-DKEY_INPUT_HANDLING_XKB=1)

include_directories(${WPE_INCLUDE_DIRECTORIES})
add_library(WPE STATIC ${WPE_SOURCES})
