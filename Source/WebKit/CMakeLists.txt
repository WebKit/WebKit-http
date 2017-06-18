cmake_minimum_required(VERSION 2.8.12)
include(WebKitCommon)

set(WebKit_SOURCES
    Storage/StorageAreaImpl.cpp
    Storage/StorageAreaSync.cpp
    Storage/StorageNamespaceImpl.cpp
    Storage/StorageSyncManager.cpp
    Storage/StorageThread.cpp
    Storage/StorageTracker.cpp
    Storage/WebDatabaseProvider.cpp
    Storage/WebStorageNamespaceProvider.cpp

    WebCoreSupport/WebResourceLoadScheduler.cpp
    WebCoreSupport/WebViewGroup.cpp
)

set(WebKit_INCLUDE_DIRECTORIES
    Storage
    WebCoreSupport

    "${CMAKE_BINARY_DIR}"
    "${CMAKE_SOURCE_DIR}/Source"
    "${DERIVED_SOURCES_DIR}"
    "${DERIVED_SOURCES_DIR}/ForwardingHeaders"
    "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore"
    "${DERIVED_SOURCES_DIR}/ForwardingHeaders/WebCore"
    "${DERIVED_SOURCES_DIR}/ForwardingHeaders/WebKitLegacy"
)

set(WebKit_SYSTEM_INCLUDE_DIRECTORIES
    "${ICU_INCLUDE_DIRS}"
)

set(WebKit_LIBRARIES
    PRIVATE JavaScriptCore${DEBUG_SUFFIX}
    PRIVATE WTF${DEBUG_SUFFIX}
    PRIVATE WebCore${DEBUG_SUFFIX}
)

WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

if (MSVC)
    if (NOT ${PORT} STREQUAL "Qt")
        ADD_PRECOMPILED_HEADER("WebKitPrefix.h" "win/WebKitPrefix.cpp" WebKit_SOURCES)
        list(APPEND WebKit_SOURCES win/WebKit.resources/WebKit.rc)
        add_definitions(/DWEBKIT_EXPORTS /DFRAMEWORK_NAME=WebKit)
    else ()
        ADD_PRECOMPILED_HEADER("WebKitPrefix.h" "qt/WebKitPrefix.cpp" WebKit_SOURCES)
    endif ()
endif ()

set(WebKit_PRIVATE_HEADERS_LOCATION Headers/${PROJECT_VERSION}/QtWebKit/private)

WEBKIT_FRAMEWORK(WebKit)

if (${PORT} STREQUAL "Qt")
    set_target_properties(WebKit PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
    install(TARGETS WebKit EXPORT WebKitTargets
            DESTINATION "${LIB_INSTALL_DIR}"
            RUNTIME DESTINATION "${BIN_INSTALL_DIR}"
    )
    if (MSVC AND NOT QT_STATIC_BUILD)
        install(FILES $<TARGET_PDB_FILE:WebKit> DESTINATION "${BIN_INSTALL_DIR}" OPTIONAL)
    endif ()

    if (NOT MSVC)
        ADD_PREFIX_HEADER(WebKit "${WEBKIT_DIR}/qt/WebKitPrefix.h")
    endif ()

    if (MACOS_BUILD_FRAMEWORKS)
        set_target_properties(WebKit PROPERTIES
            FRAMEWORK_VERSION 5
            MACOSX_FRAMEWORK_IDENTIFIER org.qt-project.QtWebKit
        )
    endif ()

    if (ENABLE_WEBKIT2)
        add_dependencies(WebKit WebKit2)
    endif ()
endif ()

add_dependencies(WebKit WebCore)
if (TARGET WebKitGUID)
    add_dependencies(WebKit WebKitGUID)
endif ()
