list(APPEND WTF_SOURCES
    gobject/GMainLoopSource.cpp
    gobject/GRefPtr.cpp
    gobject/GSourceWrap.cpp
    gobject/GThreadSafeMainLoopSource.cpp
    gobject/GlibUtilities.cpp

    glib/MainThreadGLib.cpp
    glib/RunLoopGLib.cpp
    wpe/WorkQueueWPE.cpp
)

list(APPEND WTF_LIBRARIES
    ${GLIB_GIO_LIBRARIES}
    ${GLIB_GOBJECT_LIBRARIES}
    ${GLIB_LIBRARIES}
    pthread
    ${ZLIB_LIBRARIES}
)

list(APPEND WTF_INCLUDE_DIRECTORIES
    ${GLIB_INCLUDE_DIRS}
)
