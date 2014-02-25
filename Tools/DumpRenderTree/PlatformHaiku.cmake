list(APPEND DumpRenderTree_SOURCES
    haiku/DumpRenderTree.cpp
    haiku/EditingCallbacks.cpp
    haiku/EventSender.cpp
    haiku/GCControllerHaiku.cpp
    haiku/JSStringUtils.cpp
    haiku/PixelDumpSupportHaiku.cpp
    haiku/TestRunnerHaiku.cpp
    haiku/WorkQueueItemHaiku.cpp
)

list(APPEND DumpRenderTree_LIBRARIES
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${SQLITE_LIBRARIES}
)

list(APPEND DumpRenderTree_INCLUDE_DIRECTORIES
    ${WEBKIT_DIR}/haiku/API
    ${WEBKIT_DIR}/haiku
    ${WEBKIT_DIR}/haiku/WebCoreSupport
    ${WEBCORE_DIR}/platform/graphics/haiku
    ${WEBCORE_DIR}/platform/network/haiku
    ${TOOLS_DIR}/DumpRenderTree/haiku
)

if (ENABLE_ACCESSIBILITY)
    list(APPEND DumpRenderTree_INCLUDE_DIRECTORIES
        ${TOOLS_DIR}/DumpRenderTree/haiku
    )
endif ()

# FIXME: DOWNLOADED_FONTS_DIR should not hardcode the directory
# structure. See <https://bugs.webkit.org/show_bug.cgi?id=81475>.
add_definitions(-DFONTS_CONF_DIR="${TOOLS_DIR}/DumpRenderTree/gtk/fonts"
                -DDOWNLOADED_FONTS_DIR="${CMAKE_SOURCE_DIR}/WebKitBuild/Dependencies/Source/webkitgtk-test-fonts-0.0.3"
)
