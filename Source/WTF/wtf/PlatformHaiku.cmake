LIST(APPEND WTF_SOURCES
    generic/WorkQueueGeneric.cpp
	haiku/RunLoopHaiku.cpp
    generic/MainThreadGeneric.cpp
    haiku/CurrentProcessMemoryStatus.cpp
	haiku/MemoryFootprintHaiku.cpp
    linux/MemoryPressureHandlerLinux.cpp

    posix/FileSystemPOSIX.cpp
    haiku/OSAllocatorHaiku.cpp
    posix/ThreadingPOSIX.cpp

    unicode/icu/CollatorICU.cpp

    unix/CPUTimeUnix.cpp

    PlatformUserPreferredLanguagesHaiku.cpp

    text/haiku/TextBreakIteratorInternalICUHaiku.cpp

)

LIST(APPEND WTF_LIBRARIES
    ${ZLIB_LIBRARIES}
    be execinfo
)

list(APPEND WTF_INCLUDE_DIRECTORIES
	/system/develop/headers/private/system/arch/$ENV{BE_HOST_CPU}/
	/system/develop/headers/private/system
)

add_definitions(-D_BSD_SOURCE=1)
