list(APPEND WebKit2_SOURCES
    Platform/IPC/unix/AttachmentUnix.cpp
    Platform/IPC/unix/ConnectionUnix.cpp

    Platform/haiku/LoggingHaiku.cpp
    Platform/haiku/ModuleHaiku.cpp
    Platform/haiku/WorkQueueHaiku.cpp

    Platform/unix/SharedMemoryUnix.cpp

    PluginProcess/unix/PluginControllerProxyUnix.cpp
    PluginProcess/unix/PluginProcessMainUnix.cpp
    PluginProcess/unix/PluginProcessUnix.cpp

    Shared/CoordinatedGraphics/CoordinatedGraphicsArgumentCoders.cpp
    Shared/CoordinatedGraphics/WebCoordinatedSurface.cpp

    Shared/Downloads/haiku/DownloadHaiku.cpp

    Shared/WebCoreArgumentCoders.cpp

    Shared/haiku/ShareableBitmapHaiku.cpp

    Shared/haiku/ProcessExecutablePathHaiku.cpp

    Shared/haiku/WebCoreArgumentCodersHaiku.cpp
    Shared/haiku/WebMemorySamplerHaiku.cpp

    Shared/unix/ChildProcessMain.cpp

    UIProcess/DefaultUndoController.cpp

    UIProcess/API/C/haiku/WKView.cpp

    UIProcess/API/CoordinatedGraphics/WKCoordinatedScene.cpp

    UIProcess/API/haiku/WebView.cpp

    UIProcess/CoordinatedGraphics/CoordinatedDrawingAreaProxy.cpp
    UIProcess/CoordinatedGraphics/CoordinatedLayerTreeHostProxy.cpp
    UIProcess/CoordinatedGraphics/PageViewportController.cpp
    UIProcess/CoordinatedGraphics/WebPageProxyCoordinatedGraphics.cpp
    UIProcess/CoordinatedGraphics/WebView.cpp
    UIProcess/CoordinatedGraphics/WebViewClient.cpp

    UIProcess/Launcher/haiku/ProcessLauncherHaiku.cpp

    UIProcess/Plugins/unix/PluginInfoStoreUnix.cpp
    UIProcess/Plugins/unix/PluginProcessProxyUnix.cpp

    UIProcess/Storage/StorageManager.cpp

    UIProcess/haiku/BackingStoreHaiku.cpp
    UIProcess/haiku/TextCheckerHaiku.cpp
    UIProcess/haiku/WebContextHaiku.cpp
    UIProcess/haiku/WebInspectorProxyHaiku.cpp
    UIProcess/haiku/WebPageProxyHaiku.cpp
    UIProcess/haiku/WebPreferencesHaiku.cpp
    UIProcess/haiku/WebProcessProxyHaiku.cpp
    UIProcess/haiku/WebView.cpp

    WebProcess/Cookies/haiku/WebCookieManagerHaiku.cpp

    WebProcess/InjectedBundle/haiku/InjectedBundleHaiku.cpp

    WebProcess/Plugins/Netscape/unix/PluginProxyUnix.cpp

    WebProcess/WebCoreSupport/haiku/WebContextMenuClientHaiku.cpp
    WebProcess/WebCoreSupport/haiku/WebErrorsHaiku.cpp
    WebProcess/WebCoreSupport/haiku/WebFrameNetworkingContext.cpp
    WebProcess/WebCoreSupport/haiku/WebPopupMenuHaiku.cpp

    WebProcess/WebPage/DrawingAreaImpl.cpp

    WebProcess/WebPage/CoordinatedGraphics/CoordinatedDrawingArea.cpp
    WebProcess/WebPage/CoordinatedGraphics/CoordinatedLayerTreeHost.cpp
    WebProcess/WebPage/CoordinatedGraphics/WebPageCoordinatedGraphics.cpp

    WebProcess/WebPage/haiku/WebInspectorHaiku.cpp
    WebProcess/WebPage/haiku/WebPageHaiku.cpp

    WebProcess/haiku/WebProcessHaiku.cpp
    WebProcess/haiku/WebProcessMainHaiku.cpp
)

list(APPEND WebKit2_MESSAGES_IN_FILES
    UIProcess/CoordinatedGraphics/CoordinatedLayerTreeHostProxy.messages.in
    WebProcess/WebPage/CoordinatedGraphics/CoordinatedLayerTreeHost.messages.in
)

list(APPEND WebKit2_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/graphics/opentype"
    "${WEBCORE_DIR}/platform/network/haiku"
    "${WEBKIT2_DIR}/NetworkProcess/unix"
    "${WEBKIT2_DIR}/Platform"
    "${WEBKIT2_DIR}/Shared/API/c/haiku"
    "${WEBKIT2_DIR}/Shared/CoordinatedGraphics"
    "${WEBKIT2_DIR}/Shared/unix"
    "${WEBKIT2_DIR}/UIProcess/API/C/CoordinatedGraphics"
    "${WEBKIT2_DIR}/UIProcess/API/C/haiku"
    "${WEBKIT2_DIR}/UIProcess/API/haiku"
    "${WEBKIT2_DIR}/UIProcess/haiku"
    "${WEBKIT2_DIR}/UIProcess/CoordinatedGraphics"
    "${WEBKIT2_DIR}/WebProcess/unix"
    "${WEBKIT2_DIR}/WebProcess/WebCoreSupport/haiku"
    "${WEBKIT2_DIR}/WebProcess/WebPage/CoordinatedGraphics"
    ${LIBXML2_INCLUDE_DIR}
    ${LIBXSLT_INCLUDE_DIRS}
    ${SQLITE_INCLUDE_DIRS}
    ${WTF_DIR}
)

list(APPEND WebKit2_LIBRARIES
    ${CMAKE_DL_LIBS}
    ${FONTCONFIG_LIBRARIES}
    ${FREETYPE2_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${PNG_LIBRARIES}
    ${SQLITE_LIBRARIES}
)

list(APPEND WebProcess_SOURCES
    WebProcess/EntryPoint/unix/WebProcessMain.cpp
)

list(APPEND NetworkProcess_SOURCES
    NetworkProcess/EntryPoint/unix/NetworkProcessMain.cpp
)

list(APPEND WebProcess_LIBRARIES
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${SQLITE_LIBRARIES}
)

add_custom_target(forwarding-headerHaiku
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl ${WEBKIT2_DIR} ${DERIVED_SOURCES_WEBKIT2_DIR}/include haiku
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl ${WEBKIT2_DIR} ${DERIVED_SOURCES_WEBKIT2_DIR}/include CoordinatedGraphics
)

set(WEBKIT2_EXTRA_DEPENDENCIES
    forwarding-headerHaiku
)

add_definitions(
    -DLIBEXECDIR=\"${EXEC_INSTALL_DIR}\"
    -DWEBPROCESSNAME=\"WebProcess\"
    -DPLUGINPROCESSNAME=\"PluginProcess\"
    -DNETWORKPROCESSNAME=\"NetworkProcess\"
)

