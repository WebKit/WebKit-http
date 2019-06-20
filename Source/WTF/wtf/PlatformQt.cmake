list(APPEND WTF_SOURCES
    qt/FileSystemQt.cpp
    qt/LanguageQt.cpp
    qt/MainThreadQt.cpp
    qt/RunLoopQt.cpp

    text/qt/StringQt.cpp
    text/qt/TextBreakIteratorInternalICUQt.cpp
)
QTWEBKIT_GENERATE_MOC_FILES_CPP(WTF qt/MainThreadQt.cpp qt/RunLoopQt.cpp)

if (USE_MACH_PORTS)
    list(APPEND WTF_FORWARDING_HEADERS_FILES
        cocoa/MachSendRight.h
    )
    list(APPEND WTF_SOURCES
        cocoa/MachSendRight.cpp
    )
endif()

list(APPEND WTF_SYSTEM_INCLUDE_DIRECTORIES
    ${Qt5Core_INCLUDE_DIRS}
)

list(APPEND WTF_LIBRARIES
    ${Qt5Core_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

if (SHARED_CORE)
    set(WTF_LIBRARY_TYPE SHARED)
else ()
    set(WTF_LIBRARY_TYPE STATIC)
endif ()

if (QT_STATIC_BUILD)
    list(APPEND WTF_LIBRARIES
        ${STATIC_LIB_DEPENDENCIES}
    )
endif ()

if (USE_MACH_PORTS)
    list(APPEND WTF_SOURCES
        cocoa/WorkQueueCocoa.cpp
    )
endif ()

if (USE_UNIX_DOMAIN_SOCKETS)
    list(APPEND WTF_SOURCES
        qt/WorkQueueQt.cpp

        unix/UniStdExtrasUnix.cpp
    )
    QTWEBKIT_GENERATE_MOC_FILES_CPP(WTF qt/WorkQueueQt.cpp)
endif ()

if (USE_GLIB)
    list(APPEND WTF_SOURCES
        glib/GRefPtr.cpp
    )
    list(APPEND WTF_SYSTEM_INCLUDE_DIRECTORIES
        ${GLIB_INCLUDE_DIRS}
    )
    list(APPEND WTF_LIBRARIES
        ${GLIB_GOBJECT_LIBRARIES}
        ${GLIB_LIBRARIES}
    )
endif ()

if (WIN32)
    list(REMOVE_ITEM WTF_SOURCES
        threads/BinarySemaphore.cpp
    )
    list(APPEND WTF_SOURCES
        threads/win/BinarySemaphoreWin.cpp

        win/CPUTimeWin.cpp
        win/WorkItemWin.cpp
        win/WorkQueueWin.cpp
    )
    list(APPEND WTF_LIBRARIES
        winmm
    )
endif ()

if (APPLE)
    list(APPEND WTF_SOURCES
        cocoa/CPUTimeCocoa.cpp
        cocoa/WorkQueueCocoa.cpp

        text/cf/AtomicStringImplCF.cpp
        text/cf/StringCF.cpp
        text/cf/StringImplCF.cpp
        text/cf/StringViewCF.cpp
    )
    list(APPEND WTF_LIBRARIES
        ${COREFOUNDATION_LIBRARY}
    )
    list(APPEND WTF_PUBLIC_HEADERS
        cf/TypeCastsCF.h
    )
endif ()

if (UNIX AND NOT APPLE)
    list(APPEND WTF_SOURCES
        unix/CPUTimeUnix.cpp
    )

    check_function_exists(clock_gettime CLOCK_GETTIME_EXISTS)
    if (NOT CLOCK_GETTIME_EXISTS)
        set(CMAKE_REQUIRED_LIBRARIES rt)
        check_function_exists(clock_gettime CLOCK_GETTIME_REQUIRES_LIBRT)
        if (CLOCK_GETTIME_REQUIRES_LIBRT)
            list(APPEND WTF_LIBRARIES rt)
        endif ()
    endif ()
endif ()
