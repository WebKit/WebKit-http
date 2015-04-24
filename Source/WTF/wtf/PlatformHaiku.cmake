LIST(APPEND WTF_SOURCES
    haiku/MainThreadHaiku.cpp
    haiku/RunLoopHaiku.cpp

    OSAllocatorPosix.cpp
    ThreadIdentifierDataPthreads.cpp
    ThreadingPthreads.cpp

    unicode/icu/CollatorICU.cpp
)

LIST(APPEND WTF_LIBRARIES
    ${ZLIB_LIBRARIES}
    be execinfo
)

LIST(APPEND WTF_INCLUDE_DIRECTORIES
    ${ICU_INCLUDE_DIRS}
    ${JAVASCRIPTCORE_DIR}/wtf/unicode/
)

add_definitions(-D_BSD_SOURCE=1)
