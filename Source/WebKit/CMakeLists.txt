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

WEBKIT_FRAMEWORK(WebKit)

set_target_properties(WebKit PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
install(TARGETS WebKit DESTINATION "${LIB_INSTALL_DIR}")

add_dependencies(WebKit WebCore)
if (WIN32)
    add_dependencies(WebKit WebKitGUID)
endif ()
