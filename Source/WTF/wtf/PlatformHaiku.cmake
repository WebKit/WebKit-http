LIST(APPEND WTF_SOURCES
    PlatformUserPreferredLanguagesHaiku.cpp

    text/haiku/TextBreakIteratorInternalICUHaiku.cpp

    haiku/MainThreadHaiku.cpp

    generic/RunLoopGeneric.cpp
    generic/WorkQueueGeneric.cpp

    OSAllocatorPosix.cpp
    ThreadIdentifierDataPthreads.cpp
    ThreadingPthreads.cpp

    unicode/icu/CollatorICU.cpp
)

LIST(APPEND WTF_LIBRARIES
    ${ZLIB_LIBRARIES}
    be execinfo
)

add_definitions(-D_BSD_SOURCE=1)
