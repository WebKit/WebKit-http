LIST(APPEND WTF_SOURCES
    generic/WorkQueueGeneric.cpp
    generic/RunLoopGeneric.cpp
    generic/MainThreadGeneric.cpp
    haiku/CurrentProcessMemoryStatus.cpp
	haiku/MemoryFootprintHaiku.cpp
    linux/MemoryPressureHandlerLinux.cpp

    OSAllocatorPosix.cpp
    ThreadingPthreads.cpp

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
