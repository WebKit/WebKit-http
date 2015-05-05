file(MAKE_DIRECTORY ${DERIVED_SOURCES_WEBKIT2_DIR})

add_definitions(-DWEBKIT2_COMPILATION)

set(WebKit2_USE_PREFIX_HEADER ON)

add_custom_target(webkit2wpe-forwarding-headers
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl --include-path ${WEBKIT2_DIR} --output ${FORWARDING_HEADERS_DIR} --platform wpe --platform soup
)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/bin/WPELauncher
    DEPENDS ${WEBKIT2_DIR}/UIProcess/wpe/WPELauncher.in
    COMMAND cp ${WEBKIT2_DIR}/UIProcess/wpe/WPELauncher.in ${CMAKE_BINARY_DIR}/bin/WPELauncher
    COMMAND chmod +x ${CMAKE_BINARY_DIR}/bin/WPELauncher
)
add_custom_target(webkit2wpe-launcher-script
    DEPENDS ${CMAKE_BINARY_DIR}/bin/WPELauncher
)

set(WEBKIT2_EXTRA_DEPENDENCIES
    webkit2wpe-forwarding-headers
    webkit2wpe-launcher-script
)

list(APPEND WebProcess_SOURCES
    WebProcess/EntryPoint/unix/WebProcessMain.cpp
)

list(APPEND NetworkProcess_SOURCES
    NetworkProcess/EntryPoint/unix/NetworkProcessMain.cpp
)

list(APPEND WebKit2_SOURCES
    NetworkProcess/soup/NetworkProcessSoup.cpp
    NetworkProcess/soup/RemoteNetworkingContextSoup.cpp
    NetworkProcess/wpe/NetworkProcessMainWPE.cpp
    Platform/IPC/unix/AttachmentUnix.cpp
    Platform/IPC/unix/ConnectionUnix.cpp
    Platform/unix/SharedMemoryUnix.cpp
    Platform/wpe/LoggingWPE.cpp
    Platform/wpe/ModuleWPE.cpp
    PluginProcess/unix/PluginControllerProxyUnix.cpp
    PluginProcess/unix/PluginProcessMainUnix.cpp
    PluginProcess/unix/PluginProcessUnix.cpp
    Shared/API/c/cairo/WKImageCairo.cpp
    Shared/Downloads/soup/DownloadSoup.cpp
    Shared/Downloads/wpe/DownloadSoupErrorsWPE.cpp
    Shared/Network/CustomProtocols/soup/CustomProtocolManagerImpl.cpp
    Shared/Network/CustomProtocols/soup/CustomProtocolManagerSoup.cpp
    Shared/Plugins/Netscape/x11/NetscapePluginModuleX11.cpp
    Shared/cairo/ShareableBitmapCairo.cpp
    Shared/linux/WebMemorySamplerLinux.cpp
    Shared/soup/WebCoreArgumentCodersSoup.cpp
    Shared/unix/ChildProcessMain.cpp
    Shared/wpe/NativeContextMenuItemWPE.cpp
    Shared/wpe/NativeWebKeyboardEventWPE.cpp
    Shared/wpe/NativeWebMouseEventWPE.cpp
    Shared/wpe/NativeWebTouchEventWPE.cpp
    Shared/wpe/NativeWebWheelEventWPE.cpp
    Shared/wpe/ProcessExecutablePathWPE.cpp
    Shared/wpe/WebEventFactory.cpp
    UIProcess/API/C/cairo/WKIconDatabaseCairo.cpp
    UIProcess/API/C/soup/WKCookieManagerSoup.cpp
    UIProcess/API/C/soup/WKSoupCustomProtocolRequestManager.cpp
    UIProcess/API/C/wpe/WKView.cpp
    UIProcess/API/wpe/PageClientImpl.cpp
    UIProcess/API/wpe/WPEView.cpp
    UIProcess/BackingStore.cpp
    UIProcess/DefaultUndoController.cpp
    UIProcess/DrawingAreaProxyImpl.cpp
    UIProcess/InspectorServer/wpe/WebInspectorServerWPE.cpp
    UIProcess/InspectorServer/soup/WebSocketServerSoup.cpp
    UIProcess/Launcher/wpe/ProcessLauncherWPE.cpp
    UIProcess/Network/CustomProtocols/soup/CustomProtocolManagerProxySoup.cpp
    UIProcess/Network/CustomProtocols/soup/WebSoupCustomProtocolRequestManagerClient.cpp
    UIProcess/Network/CustomProtocols/soup/WebSoupCustomProtocolRequestManager.cpp
    UIProcess/Network/soup/NetworkProcessProxySoup.cpp
    UIProcess/Plugins/unix/PluginInfoStoreUnix.cpp
    UIProcess/Plugins/unix/PluginProcessProxyUnix.cpp
    UIProcess/Storage/StorageManager.cpp
    UIProcess/cairo/BackingStoreCairo.cpp
    UIProcess/soup/WebCookieManagerProxySoup.cpp
    UIProcess/soup/WebProcessPoolSoup.cpp
    UIProcess/wpe/TextCheckerWPE.cpp
    UIProcess/wpe/WebInspectorProxyWPE.cpp
    UIProcess/wpe/WebPageProxyWPE.cpp
    UIProcess/wpe/WebPreferencesWPE.cpp
    UIProcess/wpe/WebProcessPoolWPE.cpp
    UIProcess/wpe/WebProcessProxyWPE.cpp
    WebProcess/Cookies/soup/WebCookieManagerSoup.cpp
    WebProcess/Cookies/soup/WebKitSoupCookieJarSqlite.cpp
    WebProcess/InjectedBundle/wpe/InjectedBundleWPE.cpp
    WebProcess/MediaCache/WebMediaKeyStorageManager.cpp
    WebProcess/WebCoreSupport/soup/WebFrameNetworkingContext.cpp
    WebProcess/WebCoreSupport/wpe/WebContextMenuClientWPE.cpp
    WebProcess/WebCoreSupport/wpe/WebEditorClientWPE.cpp
    WebProcess/WebCoreSupport/wpe/WebErrorsWPE.cpp
    WebProcess/WebCoreSupport/wpe/WebPopupMenuWPE.cpp
    WebProcess/WebPage/DrawingAreaImpl.cpp
    WebProcess/WebPage/wpe/LayerTreeHostWPE.cpp
    WebProcess/WebPage/wpe/WebInspectorUIWPE.cpp
    WebProcess/WebPage/wpe/WebPageWPE.cpp
    WebProcess/soup/WebKitSoupRequestGeneric.cpp
    WebProcess/soup/WebKitSoupRequestInputStream.cpp
    WebProcess/soup/WebProcessSoup.cpp
    WebProcess/wpe/WebProcessMainWPE.cpp
)

list(APPEND WebKit2_DERIVED_SOURCES
    ${DERIVED_SOURCES_WEBKIT2_DIR}/WebKit2ResourcesGResourceBundle.c
)

set(WebKit2Resources
)

if (ENABLE_WEB_AUDIO)
    list(APPEND WebKit2Resources
        "        <file alias=\"audio/Composite\">Composite.wav</file>\n"
    )
endif ()

file(WRITE ${DERIVED_SOURCES_WEBKIT2_DIR}/WebKit2ResourcesGResourceBundle.xml
    "<?xml version=1.0 encoding=UTF-8?>\n"
    "<gresources>\n"
    "    <gresource prefix=\"/org/webkitwpe/resources\">\n"
    ${WebKit2Resources}
    "    </gresource>\n"
    "</gresources>\n"
)

add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBKIT2_DIR}/WebKit2ResourcesGResourceBundle.c
    DEPENDS ${DERIVED_SOURCES_WEBKIT2_DIR}/WebKit2ResourcesGResourceBundle.xml
    COMMAND glib-compile-resources --generate --sourcedir=${CMAKE_SOURCE_DIR}/Source/WebCore/Resources --sourcedir=${CMAKE_SOURCE_DIR}/Source/WebCore/platform/audio/resources --target=${DERIVED_SOURCES_WEBKIT2_DIR}/WebKit2ResourcesGResourceBundle.c ${DERIVED_SOURCES_WEBKIT2_DIR}/WebKit2ResourcesGResourceBundle.xml
    VERBATIM
)

list(APPEND WebKit2_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}"
    "${DERIVED_SOURCES_DIR}"
    "${WEBCORE_DIR}/platform/graphics/cairo"
    "${WEBCORE_DIR}/platform/graphics/opentype"
    "${WEBCORE_DIR}/platform/network/soup"
    "${WEBKIT2_DIR}/NetworkProcess/unix"
    "${WEBKIT2_DIR}/Shared/API/c/wpe"
    "${WEBKIT2_DIR}/Shared/Network/CustomProtocols/soup"
    "${WEBKIT2_DIR}/Shared/Downloads/soup"
    "${WEBKIT2_DIR}/Shared/soup"
    "${WEBKIT2_DIR}/Shared/unix"
    "${WEBKIT2_DIR}/Shared/wpe"
    "${WEBKIT2_DIR}/UIProcess/API/C/cairo"
    "${WEBKIT2_DIR}/UIProcess/API/C/soup"
    "${WEBKIT2_DIR}/UIProcess/API/C/wpe"
    "${WEBKIT2_DIR}/UIProcess/API/wpe"
    "${WEBKIT2_DIR}/UIProcess/Network/CustomProtocols/soup"
    "${WEBKIT2_DIR}/UIProcess/soup"
    "${WEBKIT2_DIR}/UIProcess/wpe/WestonShell"
    "${WEBKIT2_DIR}/WebProcess/soup"
    "${WEBKIT2_DIR}/WebProcess/unix"
    "${WEBKIT2_DIR}/WebProcess/WebCoreSupport/soup"
    "${WEBKIT2_DIR}/WebProcess/WebPage/wpe"
    "${WTF_DIR}/wtf/gtk/"
    "${WTF_DIR}/wtf/gobject"
    "${WTF_DIR}"
    ${CAIRO_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${HARFBUZZ_INCLUDE_DIRS}
    ${LIBSOUP_INCLUDE_DIRS}
    ${WPE_DIR}
)

list(APPEND WebKit2_LIBRARIES
    ${CAIRO_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${HARFBUZZ_LIBRARIES}
    ${LIBSOUP_LIBRARIES}
    WPE
)

if (ENABLE_WESTON_SHELL)
set(WPEWestonShell_SOURCES
    UIProcess/wpe/WestonShell/Environment.cpp
    UIProcess/wpe/WestonShell/Module.cpp
    UIProcess/wpe/WestonShell/Shell.cpp
)

set(WPEWestonShell_INCLUDE_DIRECTORIES
    "${WEBKIT2_DIR}/Shared/wpe"
    ${WESTON_INCLUDE_DIRS}
)

set(WPEWestonShell_LIBRARIES
    WebKit2
    ${WESTON_LIBRARIES}
)

add_library(WPEWestonShell SHARED ${WPEWestonShell_SOURCES})
target_link_libraries(WPEWestonShell ${WPEWestonShell_LIBRARIES})
target_include_directories(WPEWestonShell PUBLIC ${WPEWestonShell_INCLUDE_DIRECTORIES})
install(TARGETS WPEWestonShell DESTINATION "${LIB_INSTALL_DIR}")
endif () # ENABLE_WESTON_SHELL

if (ENABLE_ATHOL_SHELL)
set(WPEAtholShell_SOURCES
    UIProcess/wpe/AtholShell/AtholShell.cpp
    UIProcess/wpe/AtholShell/Module.cpp
)

set(WPEAtholShell_INCLUDE_DIRECTORIES
    "${WEBKIT2_DIR}/Shared/wpe"
    ${ATHOL_INCLUDE_DIRS}
)

set(WPEAtholShell_LIBRARIES
    WebKit2
    ${ATHOL_LIBRARIES}
)

add_library(WPEAtholShell SHARED ${WPEAtholShell_SOURCES})
target_link_libraries(WPEAtholShell ${WPEAtholShell_LIBRARIES})
target_include_directories(WPEAtholShell PUBLIC ${WPEAtholShell_INCLUDE_DIRECTORIES})
install(TARGETS WPEAtholShell DESTINATION "${LIB_INSTALL_DIR}")
endif () # ENABLE_ATHOL_SHELL

file(GLOB InspectorFiles
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/*.html
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Base/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Controllers/*.css
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Controllers/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/External/CodeMirror/*.css
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/External/CodeMirror/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/External/ESLint/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/External/Esprima/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Models/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Protocol/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Views/*.css
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Views/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Images/gtk/*.png
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Images/gtk/*.svg
)

# DerivedSources/JavaScriptCore/inspector/InspectorBackendCommands.js is
# expected in DerivedSources/WebInspectorUI/UserInterface/Protocol/.
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/UserInterface/Protocol/InspectorBackendCommands.js
    DEPENDS ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/inspector/InspectorBackendCommands.js
    COMMAND cp ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/inspector/InspectorBackendCommands.js ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/UserInterface/Protocol/InspectorBackendCommands.js
)

list(APPEND InspectorFiles
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/Localizations/en.lproj/localizedStrings.js
    ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/UserInterface/Protocol/InspectorBackendCommands.js
)

add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/InspectorGResourceBundle.xml
    DEPENDS ${InspectorFiles}
            ${TOOLS_DIR}/wpe/generate-inspector-gresource-manifest.py
    COMMAND ${TOOLS_DIR}/wpe/generate-inspector-gresource-manifest.py --output=${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/InspectorGResourceBundle.xml ${InspectorFiles}
    VERBATIM
)

add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/InspectorGResourceBundle.c
    DEPENDS ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/InspectorGResourceBundle.xml
    COMMAND glib-compile-resources --generate --sourcedir=${CMAKE_SOURCE_DIR}/Source/WebInspectorUI --sourcedir=${DERIVED_SOURCES_WEBINSPECTORUI_DIR} --target=${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/InspectorGResourceBundle.c ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/InspectorGResourceBundle.xml
    VERBATIM
)

add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/WebKit2InspectorGResourceBundle.c
    DEPENDS ${WEBKIT2_DIR}/UIProcess/API/wpe/WebKit2InspectorGResourceBundle.xml
            ${WEBKIT2_DIR}/UIProcess/InspectorServer/front-end/inspectorPageIndex.html
    COMMAND glib-compile-resources --generate --sourcedir=${WEBKIT2_DIR}/UIProcess/InspectorServer/front-end --target=${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/WebKit2InspectorGResourceBundle.c ${WEBKIT2_DIR}/UIProcess/API/wpe/WebKit2InspectorGResourceBundle.xml
    VERBATIM
)

list(APPEND WPEWebInspectorResources_DERIVED_SOURCES
    ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/InspectorGResourceBundle.c
    ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/WebKit2InspectorGResourceBundle.c
)

list(APPEND WPEWebInspectorResources_LIBRARIES
    ${GLIB_GIO_LIBRARIES}
)

add_library(WPEWebInspectorResources SHARED ${WPEWebInspectorResources_DERIVED_SOURCES})
add_dependencies(WPEWebInspectorResources WebKit2)
target_link_libraries(WPEWebInspectorResources ${WPEWebInspectorResources_LIBRARIES})
install(TARGETS WPEWebInspectorResources DESTINATION "${LIB_INSTALL_DIR}")
