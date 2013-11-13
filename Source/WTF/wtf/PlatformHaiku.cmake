LIST(APPEND WTF_SOURCES
    haiku/MainThreadHaiku.cpp

    OSAllocatorPosix.cpp
    ThreadIdentifierDataPthreads.cpp
    ThreadingPthreads.cpp

    unicode/icu/CollatorICU.cpp
)

LIST(APPEND WTF_LIBRARIES
    ${ZLIB_LIBRARIES}
    be
)

LIST(APPEND WTF_INCLUDE_DIRECTORIES
    ${ICU_INCLUDE_DIRS}
    ${JAVASCRIPTCORE_DIR}/wtf/unicode/
)
