set(WebKit2_WebProcess_OUTPUT_NAME QtWebProcess)
set(WebKit2_NetworkProcess_OUTPUT_NAME QtWebNetworkProcess)
set(WebKit2_PluginProcess_OUTPUT_NAME QtWebPluginProcess)
set(WebKit2_DatabaseProcess_OUTPUT_NAME QtWebDatabaseProcess)

#set(WebKit2_USE_PREFIX_HEADER ON)

# FIXME: It should be in WebKitFS actually
set(FORWARDING_HEADERS_DIR "${DERIVED_SOURCES_DIR}/ForwardingHeaders")

list(APPEND WebKit2_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}"

    "${WEBCORE_DIR}/platform/graphics/opentype"
    "${WEBCORE_DIR}/platform/graphics/qt"
    "${WEBCORE_DIR}/platform/network/qt"
    "${WEBCORE_DIR}/platform/qt"

    # The WebKit2 Qt APIs depend on qwebkitglobal.h, which lives in WebKit
    "${WEBKIT_DIR}/qt/Api"

    "${WEBKIT2_DIR}/NetworkProcess/CustomProtocols/qt"

    "${WEBKIT2_DIR}/Shared/CoordinatedGraphics"
    "${WEBKIT2_DIR}/Shared/qt"
    "${WEBKIT2_DIR}/Shared/unix"

    "${WEBKIT2_DIR}/UIProcess/API/C/qt"
    "${WEBKIT2_DIR}/UIProcess/API/qt"
    "${WEBKIT2_DIR}/UIProcess/API/cpp/qt"
    "${WEBKIT2_DIR}/UIProcess/CoordinatedGraphics"
    "${WEBKIT2_DIR}/UIProcess/InspectorServer/qt"
    "${WEBKIT2_DIR}/UIProcess/gstreamer"
    "${WEBKIT2_DIR}/UIProcess/qt"
    "${WEBKIT2_DIR}/WebProcess/WebCoreSupport/qt"
    "${WEBKIT2_DIR}/WebProcess/WebPage/CoordinatedGraphics"
    "${WEBKIT2_DIR}/WebProcess/qt"
)

list(APPEND WebKit2_SOURCES
    NetworkProcess/CustomProtocols/qt/CustomProtocolManagerImpl.cpp
    NetworkProcess/CustomProtocols/qt/CustomProtocolManagerQt.cpp

    NetworkProcess/Downloads/qt/DownloadQt.cpp
    NetworkProcess/Downloads/qt/QtFileDownloader.cpp

    NetworkProcess/qt/NetworkProcessMainQt.cpp
    NetworkProcess/qt/NetworkProcessQt.cpp
    NetworkProcess/qt/RemoteNetworkingContextQt.cpp

    Platform/qt/LoggingQt.cpp
    Platform/qt/ModuleQt.cpp

    PluginProcess/qt/PluginControllerProxyQt.cpp
    PluginProcess/qt/PluginProcessMainQt.cpp
    PluginProcess/qt/PluginProcessQt.cpp

    Shared/API/c/qt/WKImageQt.cpp

    Shared/Authentication/qt/AuthenticationManagerQt.cpp

    Shared/CoordinatedGraphics/CoordinatedBackingStore.cpp
    Shared/CoordinatedGraphics/CoordinatedGraphicsArgumentCoders.cpp
    Shared/CoordinatedGraphics/CoordinatedGraphicsScene.cpp
    Shared/CoordinatedGraphics/WebCoordinatedSurface.cpp

    Shared/qt/ArgumentCodersQt.cpp
    Shared/qt/NativeWebKeyboardEventQt.cpp
    Shared/qt/NativeWebMouseEventQt.cpp
    Shared/qt/NativeWebTouchEventQt.cpp
    Shared/qt/NativeWebWheelEventQt.cpp
    Shared/qt/ProcessExecutablePathQt.cpp
    Shared/qt/QtNetworkReplyData.cpp
    Shared/qt/QtNetworkRequestData.cpp
    Shared/qt/ShareableBitmapQt.cpp
    Shared/qt/WebCoreArgumentCodersQt.cpp
    Shared/qt/WebEventFactoryQt.cpp

    Shared/unix/ChildProcessMain.cpp

    UIProcess/BackingStore.cpp
    UIProcess/DefaultUndoController.cpp
    UIProcess/DrawingAreaProxyImpl.cpp
    UIProcess/LegacySessionStateCodingNone.cpp

    UIProcess/API/C/qt/WKIconDatabaseQt.cpp

    UIProcess/API/cpp/qt/WKStringQt.cpp
    UIProcess/API/cpp/qt/WKURLQt.cpp

    UIProcess/API/qt/qquicknetworkreply.cpp
    UIProcess/API/qt/qquicknetworkrequest.cpp
    UIProcess/API/qt/qquickurlschemedelegate.cpp
    UIProcess/API/qt/qquickwebpage.cpp
    UIProcess/API/qt/qquickwebview.cpp
    UIProcess/API/qt/qtwebsecurityorigin.cpp
    UIProcess/API/qt/qwebchannelwebkittransport.cpp
    UIProcess/API/qt/qwebdownloaditem.cpp
    UIProcess/API/qt/qwebdownloaditem_p.h
    UIProcess/API/qt/qwebdownloaditem_p_p.h
    UIProcess/API/qt/qwebiconimageprovider.cpp
    UIProcess/API/qt/qwebkittest.cpp
    UIProcess/API/qt/qwebloadrequest.cpp
    UIProcess/API/qt/qwebnavigationhistory.cpp
    UIProcess/API/qt/qwebnavigationrequest.cpp
    UIProcess/API/qt/qwebpermissionrequest.cpp
    UIProcess/API/qt/qwebpreferences.cpp

    UIProcess/CoordinatedGraphics/CoordinatedDrawingAreaProxy.cpp
    UIProcess/CoordinatedGraphics/CoordinatedLayerTreeHostProxy.cpp
    UIProcess/CoordinatedGraphics/PageViewportController.cpp
    UIProcess/CoordinatedGraphics/WebPageProxyCoordinatedGraphics.cpp

    UIProcess/InspectorServer/qt/WebInspectorServerQt.cpp
    UIProcess/InspectorServer/qt/WebSocketServerQt.cpp

    UIProcess/Launcher/qt/ProcessLauncherQt.cpp

    UIProcess/Network/CustomProtocols/qt/CustomProtocolManagerProxyQt.cpp

    UIProcess/Plugins/qt/PluginProcessProxyQt.cpp

    UIProcess/Storage/StorageManager.cpp

    UIProcess/WebsiteData/unix/WebsiteDataStoreUnix.cpp

    UIProcess/gstreamer/InstallMissingMediaPluginsPermissionRequest.cpp
    UIProcess/gstreamer/WebPageProxyGStreamer.cpp

    UIProcess/qt/BackingStoreQt.cpp
    UIProcess/qt/PageViewportControllerClientQt.cpp
    UIProcess/qt/QtDialogRunner.cpp
    UIProcess/qt/QtDownloadManager.cpp
    UIProcess/qt/QtGestureRecognizer.cpp
    UIProcess/qt/QtPageClient.cpp
    UIProcess/qt/QtPanGestureRecognizer.cpp
    UIProcess/qt/QtPinchGestureRecognizer.cpp
    UIProcess/qt/QtTapGestureRecognizer.cpp
    UIProcess/qt/QtWebContext.cpp
    UIProcess/qt/QtWebError.cpp
    UIProcess/qt/QtWebIconDatabaseClient.cpp
    UIProcess/qt/QtWebPageEventHandler.cpp
    UIProcess/qt/QtWebPagePolicyClient.cpp
    UIProcess/qt/QtWebPageSGNode.cpp
    UIProcess/qt/QtWebPageUIClient.cpp
    UIProcess/qt/TextCheckerQt.cpp
    UIProcess/qt/WebColorPickerQt.cpp
    UIProcess/qt/WebContextMenuProxyQt.cpp
    UIProcess/qt/WebContextQt.cpp
    UIProcess/qt/WebFullScreenManagerProxyQt.cpp
    UIProcess/qt/WebGeolocationProviderQt.cpp
    UIProcess/qt/WebInspectorProxyQt.cpp
    UIProcess/qt/WebPageProxyQt.cpp
    UIProcess/qt/WebPopupMenuProxyQt.cpp
    UIProcess/qt/WebPreferencesQt.cpp

    WebProcess/Cookies/qt/WebCookieManagerQt.cpp

    WebProcess/InjectedBundle/qt/InjectedBundleQt.cpp

    WebProcess/Plugins/Netscape/qt/PluginProxyQt.cpp

    WebProcess/WebCoreSupport/qt/WebContextMenuClientQt.cpp
    WebProcess/WebCoreSupport/qt/WebDragClientQt.cpp
    WebProcess/WebCoreSupport/qt/WebErrorsQt.cpp
    WebProcess/WebCoreSupport/qt/WebFrameNetworkingContext.cpp
    WebProcess/WebCoreSupport/qt/WebPopupMenuQt.cpp

    WebProcess/WebPage/CoordinatedGraphics/CoordinatedDrawingArea.cpp
    WebProcess/WebPage/CoordinatedGraphics/CoordinatedLayerTreeHost.cpp
    WebProcess/WebPage/CoordinatedGraphics/WebPageCoordinatedGraphics.cpp

    WebProcess/WebPage/gstreamer/WebPageGStreamer.cpp

    WebProcess/WebPage/qt/WebInspectorUIQt.cpp
    WebProcess/WebPage/qt/WebPageQt.cpp

    WebProcess/qt/QtBuiltinBundle.cpp
    WebProcess/qt/QtBuiltinBundlePage.cpp
    WebProcess/qt/QtNetworkAccessManager.cpp
    WebProcess/qt/QtNetworkReply.cpp
    WebProcess/qt/SeccompFiltersWebProcessQt.cpp
    WebProcess/qt/WebProcessMainQt.cpp
    WebProcess/qt/WebProcessQt.cpp
)

if (APPLE)
    list(APPEND WebKit2_INCLUDE_DIRECTORIES
        "${WEBKIT2_DIR}/Platform/IPC/mac"
        "${WEBKIT2_DIR}/Platform/mac"
    )
    list(APPEND WebKit2_SOURCES
        Platform/IPC/mac/ConnectionMac.mm

        Platform/mac/MachUtilities.cpp
        Platform/mac/SharedMemoryMac.cpp
    )
elseif (WIN32)
    list(APPEND WebKit2_SOURCES
        Platform/IPC/win/ConnectionWin.cpp

        Platform/win/SharedMemoryWin.cpp
    )
else ()
    list(APPEND WebKit2_SOURCES
        Platform/IPC/unix/AttachmentUnix.cpp
        Platform/IPC/unix/ConnectionUnix.cpp

        Platform/unix/SharedMemoryUnix.cpp
    )
endif ()

list(APPEND WebKit2_SYSTEM_INCLUDE_DIRECTORIES
    ${GSTREAMER_INCLUDE_DIRS}
    ${Qt5Quick_INCLUDE_DIRS}
    ${Qt5Quick_PRIVATE_INCLUDE_DIRS}
)

list(APPEND WebKit2_LIBRARIES
    ${Qt5Positioning_LIBRARIES}
    ${Qt5Quick_LIBRARIES}
)

list(APPEND WebKit2_MESSAGES_IN_FILES
    UIProcess/CoordinatedGraphics/CoordinatedLayerTreeHostProxy.messages.in

    WebProcess/WebPage/CoordinatedGraphics/CoordinatedLayerTreeHost.messages.in
)

list(APPEND WebProcess_SOURCES
    qt/MainQt.cpp
)

# FIXME: Allow building without widgets
list(APPEND WebProcess_LIBRARIES
    Qt5::Widgets
    WebKitWidgets
)

list(APPEND NetworkProcess_SOURCES
    NetworkProcess/EntryPoint/qt/NetworkProcessMain.cpp
)

# FIXME
list(APPEND DatabaseProcess_SOURCES
    DatabaseProcess/EntryPoint/unix/DatabaseProcessMain.cpp
)

list(APPEND PluginProcess_SOURCES
    qt/PluginMainQt.cpp
)

add_custom_target(WebKit2-forwarding-headers
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl --include-path ${WEBKIT2_DIR} --output ${FORWARDING_HEADERS_DIR} --platform qt
)

set(WEBKIT2_EXTRA_DEPENDENCIES
     WebKit2-forwarding-headers
)

