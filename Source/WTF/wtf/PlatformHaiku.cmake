LIST(APPEND WTF_SOURCES
    generic/WorkQueueGeneric.cpp
	haiku/RunLoopHaiku.cpp
    generic/MainThreadGeneric.cpp
    haiku/CurrentProcessMemoryStatus.cpp
	haiku/MemoryFootprintHaiku.cpp
    linux/MemoryPressureHandlerLinux.cpp

    OSAllocatorPosix.cpp
    ThreadingPthreads.cpp

    posix/FileSystemPOSIX.cpp

    unicode/icu/CollatorICU.cpp

    unix/CPUTimeUnix.cpp

    PlatformUserPreferredLanguagesHaiku.cpp

    text/haiku/TextBreakIteratorInternalICUHaiku.cpp

)

LIST(APPEND WTF_LIBRARIES
    ${ZLIB_LIBRARIES}
    be execinfo
)

add_definitions(-D_BSD_SOURCE=1)
