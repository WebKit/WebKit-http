list(APPEND WTF_SOURCES
    glib/GMainLoopSource.cpp
    glib/GRefPtr.cpp
    glib/GSourceWrap.cpp
    glib/GThreadSafeMainLoopSource.cpp
    glib/GLibUtilities.cpp
    glib/MainThreadGLib.cpp
    glib/RunLoopGLib.cpp
    glib/WorkQueueGLib.cpp
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
