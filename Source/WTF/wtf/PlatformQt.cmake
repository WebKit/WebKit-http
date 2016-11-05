list(APPEND WTF_SOURCES
    qt/MainThreadQt.cpp
    qt/RunLoopQt.cpp
    qt/WorkQueueQt.cpp

    text/qt/StringQt.cpp
)

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

if (UNIX AND NOT APPLE)
    list(APPEND WTF_SOURCES
        UniStdExtras.cpp
    )
endif ()

if (USE_GLIB)
    list(APPEND WTF_SOURCES
        glib/GRefPtr.cpp
    )
    list(APPEND WTF_SYSTEM_INCLUDE_DIRECTORIES
        ${GLIB_INCLUDE_DIRS}
    )
endif ()

if (WIN32)
    list(APPEND WTF_LIBRARIES
        winmm
    )

    set(WTF_POST_BUILD_COMMAND "${CMAKE_BINARY_DIR}/DerivedSources/WTF/postBuild.cmd")
    file(WRITE "${WTF_POST_BUILD_COMMAND}" "@xcopy /y /s /d /f \"${WTF_DIR}/wtf/*.h\" \"${DERIVED_SOURCES_DIR}/ForwardingHeaders/WTF\" >nul 2>nul\n@xcopy /y /s /d /f \"${DERIVED_SOURCES_DIR}/WTF/*.h\" \"${DERIVED_SOURCES_DIR}/ForwardingHeaders/WTF\" >nul 2>nul\n")
    file(MAKE_DIRECTORY ${DERIVED_SOURCES_DIR}/ForwardingHeaders/WTF)
endif ()

if (APPLE)
    list(APPEND WTF_LIBRARIES
        ${COREFOUNDATION_LIBRARY}
    )
endif ()
