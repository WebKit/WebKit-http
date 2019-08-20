set(WebKit_WebProcess_OUTPUT_NAME QtWebProcess)
set(WebKit_NetworkProcess_OUTPUT_NAME QtWebNetworkProcess)
set(WebKit_PluginProcess_OUTPUT_NAME QtWebPluginProcess)
set(WebKit_DatabaseProcess_OUTPUT_NAME QtWebStorageProcess)

file(MAKE_DIRECTORY ${DERIVED_SOURCES_WEBKIT_DIR})

if (SHARED_CORE)
    set(WebKit_LIBRARY_TYPE SHARED)
else ()
    set(WebKit_LIBRARY_TYPE STATIC)
endif ()

add_definitions(-DBUILDING_WEBKIT)

if (${JavaScriptCore_LIBRARY_TYPE} MATCHES STATIC)
    add_definitions(-DSTATICALLY_LINKED_WITH_WTF -DSTATICALLY_LINKED_WITH_JavaScriptCore)
endif ()

QTWEBKIT_SKIP_AUTOMOC(WebKit)

#set(WebKit_USE_PREFIX_HEADER ON)

list(APPEND WebKit_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}"
    "${FORWARDING_HEADERS_DIR}/QtWebKit"

    "${WEBCORE_DIR}/platform/graphics/opentype"
    "${WEBCORE_DIR}/platform/graphics/qt"
    "${WEBCORE_DIR}/platform/network/qt"
    "${WEBCORE_DIR}/platform/qt"

    # The WebKit Qt APIs depend on qwebkitglobal.h, which lives in WebKitLegacy
    "${WEBKITLEGACY_DIR}/qt/Api"
    "${WEBKITLEGACY_DIR}/qt/Plugins"

    "${WEBKIT_DIR}/NetworkProcess/CustomProtocols/qt"
    "${WEBKIT_DIR}/NetworkProcess/qt"

    "${WEBKIT_DIR}/Shared/CoordinatedGraphics"
    "${WEBKIT_DIR}/Shared/Plugins/unix"
    "${WEBKIT_DIR}/Shared/qt"
    "${WEBKIT_DIR}/Shared/unix"

    "${WEBKIT_DIR}/UIProcess/API/C/qt"
    "${WEBKIT_DIR}/UIProcess/API/qt"
    "${WEBKIT_DIR}/UIProcess/API/cpp/qt"
    "${WEBKIT_DIR}/UIProcess/CoordinatedGraphics"
    "${WEBKIT_DIR}/UIProcess/InspectorServer/qt"
    "${WEBKIT_DIR}/UIProcess/gstreamer"
    "${WEBKIT_DIR}/UIProcess/qt"

    "${WEBKIT_DIR}/WebProcess/Plugins/Netscape/unix"
    "${WEBKIT_DIR}/WebProcess/Plugins/Netscape/x11"
    "${WEBKIT_DIR}/WebProcess/WebCoreSupport/qt"
    "${WEBKIT_DIR}/WebProcess/WebPage/CoordinatedGraphics"
    "${WEBKIT_DIR}/WebProcess/qt"
)

list(APPEND WebKit_SOURCES
    DatabaseProcess/qt/DatabaseProcessMainQt.cpp

    NetworkProcess/CustomProtocols/qt/CustomProtocolManagerQt.cpp

    NetworkProcess/Downloads/qt/DownloadQt.cpp
    NetworkProcess/Downloads/qt/QtFileDownloader.cpp

    NetworkProcess/qt/NetworkProcessMainQt.cpp
    NetworkProcess/qt/NetworkProcessQt.cpp
    NetworkProcess/qt/QtNetworkAccessManager.cpp
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

    Shared/Plugins/Netscape/unix/NetscapePluginModuleUnix.cpp

    Shared/Plugins/unix/PluginSearchPath.cpp

    Shared/qt/ArgumentCodersQt.cpp
    Shared/qt/ChildProcessMainQt.cpp
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
    Shared/qt/WebGestureEvent.cpp

    UIProcess/BackingStore.cpp
    UIProcess/DefaultUndoController.cpp
    UIProcess/LegacySessionStateCodingNone.cpp

    UIProcess/API/C/qt/WKIconDatabaseQt.cpp

    UIProcess/API/cpp/qt/WKStringQt.cpp
    UIProcess/API/cpp/qt/WKURLQt.cpp

    UIProcess/API/qt/APIWebsiteDataStoreQt.cpp
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

    UIProcess/Plugins/unix/PluginInfoStoreUnix.cpp

    UIProcess/Storage/StorageManager.cpp

    UIProcess/WebsiteData/unix/WebsiteDataStoreUnix.cpp

    UIProcess/gstreamer/InstallMissingMediaPluginsPermissionRequest.cpp
    UIProcess/gstreamer/WebPageProxyGStreamer.cpp

    UIProcess/qt/BackingStoreQt.cpp
    UIProcess/qt/ColorChooserContextObject.h
    UIProcess/qt/DialogContextObjects.h
    UIProcess/qt/ItemSelectorContextObject.cpp
    UIProcess/qt/PageViewportControllerClientQt.cpp
    UIProcess/qt/QrcSchemeHandler.cpp
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
    UIProcess/qt/WebGeolocationProviderQt.cpp
    UIProcess/qt/WebInspectorProxyQt.cpp
    UIProcess/qt/WebPageProxyQt.cpp
    UIProcess/qt/WebPopupMenuProxyQt.cpp
    UIProcess/qt/WebPreferencesQt.cpp
    UIProcess/qt/WebProcessPoolQt.cpp

    WebProcess/Cookies/qt/WebCookieManagerQt.cpp

    WebProcess/InjectedBundle/qt/InjectedBundleQt.cpp

    WebProcess/Plugins/Netscape/qt/PluginProxyQt.cpp

    WebProcess/Plugins/Netscape/unix/NetscapePluginUnix.cpp

    WebProcess/Plugins/Netscape/x11/NetscapePluginX11.cpp

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
    WebProcess/qt/SeccompFiltersWebProcessQt.cpp
    WebProcess/qt/WebProcessMainQt.cpp
    WebProcess/qt/WebProcessQt.cpp
)

qt5_add_resources(WebKit_SOURCES
    WebKit2.qrc
)

if (USE_MACH_PORTS)
    list(APPEND WebKit_INCLUDE_DIRECTORIES
        "${WEBKIT_DIR}/Platform/IPC/mac"
        "${WEBKIT_DIR}/Platform/mac"
    )
    list(APPEND WebKit_SOURCES
        Platform/IPC/mac/ConnectionMac.mm

        Platform/cocoa/SharedMemoryCocoa.cpp

        Platform/mac/MachUtilities.cpp
    )
    list(APPEND WebKit_LIBRARIES
        objc
    )
elseif (WIN32)
    list(APPEND WebKit_SOURCES
        Platform/IPC/win/AttachmentWin.cpp
        Platform/IPC/win/ConnectionWin.cpp

        Platform/win/SharedMemoryWin.cpp
    )
else ()
    list(APPEND WebKit_SOURCES
        Platform/IPC/unix/AttachmentUnix.cpp
        Platform/IPC/unix/ConnectionUnix.cpp

        Platform/unix/SharedMemoryUnix.cpp
    )
endif ()

if (ENABLE_NETSCAPE_PLUGIN_API)
    # We don't build PluginProcess on Win and Mac because we don't
    # support WK2 NPAPI on these platforms, however NPAPI works in WK1.
    # Some WK2 code is guarded with ENABLE(NETSCAPE_PLUGIN_API) now
    # so it should be compiled even when we don't want PluginProcess
    # Enabling PLUGIN_PROCESS without building PluginProcess executable
    # fixes things
    add_definitions(-DENABLE_PLUGIN_PROCESS=1)
endif ()

list(APPEND WebKit_SYSTEM_INCLUDE_DIRECTORIES
    ${GLIB_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${Qt5Quick_INCLUDE_DIRS}
    ${Qt5Quick_PRIVATE_INCLUDE_DIRS}
    ${SQLITE_INCLUDE_DIR}
)

list(APPEND WebKit_LIBRARIES
    ${Qt5Positioning_LIBRARIES}
    ${Qt5Quick_LIBRARIES}
    ${Qt5WebChannel_LIBRARIES}
    ${X11_X11_LIB}
)

# list(APPEND WebKit_MESSAGES_IN_FILES
#     UIProcess/CoordinatedGraphics/CoordinatedLayerTreeHostProxy.messages.in

#     WebProcess/WebPage/CoordinatedGraphics/CoordinatedLayerTreeHost.messages.in
# )

list(APPEND WebProcess_SOURCES
    qt/MainQt.cpp
)

if (NOT SHARED_CORE)
    set(WebProcess_LIBRARIES
        WebKitLegacy
    )
    set(NetworkProcess_LIBRARIES
        WebKitLegacy
    )
    set(DatabaseProcess_LIBRARIES
        WebKitLegacy
    )
    set(PluginProcess_LIBRARIES
        WebKitLegacy
    )
endif ()

# FIXME: Allow building without widgets
list(APPEND WebProcess_LIBRARIES
    Qt5::Widgets
    WebKitWidgets
)

list(APPEND NetworkProcess_SOURCES
    NetworkProcess/EntryPoint/qt/NetworkProcessMain.cpp
)

list(APPEND DatabaseProcess_SOURCES
    DatabaseProcess/EntryPoint/qt/DatabaseProcessMain.cpp
)

list(APPEND PluginProcess_SOURCES
    qt/PluginMainQt.cpp
)

add_custom_target(WebKit-forwarding-headers
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT_DIR}/Scripts/generate-forwarding-headers.pl --include-path ${WEBKIT_DIR} --output ${FORWARDING_HEADERS_DIR} --platform qt
)

set(WEBKIT_EXTRA_DEPENDENCIES
     WebKit-forwarding-headers
)

WEBKIT_CREATE_FORWARDING_HEADERS(QtWebKit/private DIRECTORIES UIProcess/API/qt)

if (ENABLE_API_TESTS)
    add_subdirectory(UIProcess/API/qt/tests)
endif ()

file(GLOB WebKit_PRIVATE_HEADERS UIProcess/API/qt/*_p.h)
install(
    FILES
       ${WebKit_PRIVATE_HEADERS}
    DESTINATION
        ${KDE_INSTALL_INCLUDEDIR}/QtWebKit/${PROJECT_VERSION}/QtWebKit/private
    COMPONENT Data
)
