cmake_minimum_required(VERSION 3.10)
include(WebKitCommon)
set_property(DIRECTORY . PROPERTY FOLDER "WebKitLegacy")

set(WebKitLegacy_SOURCES
    Storage/StorageAreaImpl.cpp
    Storage/StorageAreaSync.cpp
    Storage/StorageNamespaceImpl.cpp
    Storage/StorageSyncManager.cpp
    Storage/StorageThread.cpp
    Storage/StorageTracker.cpp
    Storage/WebDatabaseProvider.cpp
    Storage/WebStorageNamespaceProvider.cpp

    WebCoreSupport/NetworkStorageSessionMap.cpp
    WebCoreSupport/WebResourceLoadScheduler.cpp
    WebCoreSupport/WebViewGroup.cpp
)

set(WebKitLegacy_PRIVATE_INCLUDE_DIRECTORIES
    "${CMAKE_BINARY_DIR}"
    "${WEBKITLEGACY_DIR}/Storage"
    "${WEBKITLEGACY_DIR}/WebCoreSupport"
    "${WebCore_PRIVATE_FRAMEWORK_HEADERS_DIR}"
    "${WebKitLegacy_DERIVED_SOURCES_DIR}"
)

set(WebKitLegacy_LIBRARIES
    PRIVATE WebKit::JavaScriptCore
    PRIVATE WebCore${DEBUG_SUFFIX}
    PRIVATE PAL${DEBUG_SUFFIX}
)

WEBKIT_FRAMEWORK_DECLARE(WebKitLegacy)
WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

if (MSVC)
    if (NOT ${PORT} STREQUAL "Qt")
        WEBKIT_ADD_PRECOMPILED_HEADER("WebKitPrefix.h" "win/WebKitPrefix.cpp" WebKitLegacy_SOURCES)
        list(APPEND WebKitLegacy_SOURCES win/WebKit.resources/WebKit.rc)
        add_definitions(/DWEBKIT_EXPORTS /DFRAMEWORK_NAME=WebKitLegacy)
    else ()
        WEBKIT_ADD_PRECOMPILED_HEADER("WebKitPrefix.h" "qt/WebKitPrefix.cpp" WebKitLegacy_SOURCES)
    endif ()
endif ()

set(WebKit_PRIVATE_HEADERS_LOCATION Headers/${PROJECT_VERSION}/QtWebKit/private)

WEBKIT_FRAMEWORK(WebKitLegacy)

if (${PORT} STREQUAL "Qt")
    set_target_properties(WebKitLegacy PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
    install(TARGETS WebKitLegacy EXPORT WebKitTargets
            DESTINATION "${LIB_INSTALL_DIR}"
            RUNTIME DESTINATION "${BIN_INSTALL_DIR}"
    )
    if (MSVC AND NOT QT_STATIC_BUILD)
        install(FILES $<TARGET_PDB_FILE:WebKitLegacy> DESTINATION "${BIN_INSTALL_DIR}" OPTIONAL)
    endif ()

    if (NOT MSVC)
        ADD_PREFIX_HEADER(WebKitLegacy "${WEBKITLEGACY_DIR}/qt/WebKitPrefix.h")
    endif ()

    if (MACOS_BUILD_FRAMEWORKS)
        set_target_properties(WebKitLegacy PROPERTIES
            FRAMEWORK_VERSION ${PROJECT_VERSION_MAJOR}
            SOVERSION ${MACOS_COMPATIBILITY_VERSION}
            MACOSX_FRAMEWORK_IDENTIFIER org.qt-project.QtWebKit
        )
    endif ()

    if (USE_LINKER_VERSION_SCRIPT)
        set(VERSION_SCRIPT "${CMAKE_BINARY_DIR}/QtWebKit.version")
        add_custom_command(TARGET WebKitLegacy PRE_LINK
            COMMAND ${PERL_EXECUTABLE} ${TOOLS_DIR}/qt/generate-version-script.pl ${Qt5_VERSION} > ${VERSION_SCRIPT}
            VERBATIM
        )
    set_target_properties(WebKitLegacy PROPERTIES LINK_FLAGS -Wl,--version-script,${VERSION_SCRIPT})
    endif ()

    if (ENABLE_WEBKIT)
        add_dependencies(WebKitLegacy WebKit)
    endif ()
endif ()

add_dependencies(WebKitLegacy WebCore WebCorePrivateFrameworkHeaders)
if (TARGET WebKitLegacyGUID)
    add_dependencies(WebKitLegacy WebKitLegacyGUID)
endif ()

if (APPLE)
    set_target_properties(WebKitLegacy PROPERTIES LINK_FLAGS "-umbrella WebKit")
endif ()

if (MSVC)
    add_custom_command(
        TARGET WebKitLegacy
        POST_BUILD
        COMMAND ${PERL_EXECUTABLE} ${WEBKIT_LIBRARIES_DIR}/tools/scripts/version-stamp.pl ${WebKitLegacy_DERIVED_SOURCES_DIR} $<TARGET_FILE:WebKitLegacy>
        VERBATIM)

    if (NOT WTF_PLATFORM_WIN_CAIRO AND NOT ${PORT} STREQUAL "Qt")
         add_subdirectory(win/WebKitQuartzCoreAdditions)
     endif ()
endif ()
