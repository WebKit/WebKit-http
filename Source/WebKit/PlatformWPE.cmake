include(InspectorGResources.cmake)

set(WebKit_OUTPUT_NAME WPEWebKit-${WPE_API_VERSION})
set(WebKit_WebProcess_OUTPUT_NAME WPEWebProcess)
set(WebKit_NetworkProcess_OUTPUT_NAME WPENetworkProcess)
set(WebKit_StorageProcess_OUTPUT_NAME WPEStorageProcess)

file(MAKE_DIRECTORY ${DERIVED_SOURCES_WPE_API_DIR})
file(MAKE_DIRECTORY ${FORWARDING_HEADERS_WPE_DIR})
file(MAKE_DIRECTORY ${FORWARDING_HEADERS_WPE_EXTENSION_DIR})
file(MAKE_DIRECTORY ${FORWARDING_HEADERS_WPE_DOM_DIR})

configure_file(wpe/wpe-webkit.pc.in ${CMAKE_BINARY_DIR}/wpe-webkit-${WPE_API_VERSION}.pc @ONLY)
configure_file(wpe/wpe-web-extension.pc.in ${CMAKE_BINARY_DIR}/wpe-web-extension-${WPE_API_VERSION}.pc @ONLY)
configure_file(wpe/wpe-webkit-deprecated.pc.in ${CMAKE_BINARY_DIR}/wpe-webkit-deprecated-${WPE_API_VERSION}.pc @ONLY)

add_definitions(-DWEBKIT2_COMPILATION)

add_definitions(-DPKGLIBDIR="${LIB_INSTALL_DIR}/wpe-webkit-${WPE_API_VERSION}")
add_definitions(-DPKGLIBEXECDIR="${LIBEXEC_INSTALL_DIR}")
add_definitions(-DLOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}")

if (NOT DEVELOPER_MODE AND NOT CMAKE_SYSTEM_NAME MATCHES "Darwin")
    WEBKIT_ADD_TARGET_PROPERTIES(WebKit LINK_FLAGS "-Wl,--version-script,${CMAKE_CURRENT_SOURCE_DIR}/webkitglib-symbols.map")
endif ()

# Temporary workaround to allow the build to succeed.
file(REMOVE "${FORWARDING_HEADERS_DIR}/WebCore/Settings.h")

set(WebKit_USE_PREFIX_HEADER ON)

add_custom_target(webkitwpe-forwarding-headers
    DEPENDS ${WebKit_DERIVED_SOURCES}
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT_DIR}/Scripts/generate-forwarding-headers.pl --include-path ${WEBKIT_DIR} --include-path ${DERIVED_SOURCES_WEBKIT_DIR} --output ${FORWARDING_HEADERS_DIR} --platform wpe --platform soup
)

 # These symbolic link allows includes like #include <wpe/WebkitWebView.h> which simulates installed headers.
add_custom_command(
    OUTPUT ${FORWARDING_HEADERS_WPE_DIR}/wpe
    DEPENDS ${WEBKIT_DIR}/UIProcess/API/wpe
    COMMAND ln -n -s -f ${WEBKIT_DIR}/UIProcess/API/wpe ${FORWARDING_HEADERS_WPE_DIR}/wpe
)

add_custom_command(
    OUTPUT ${FORWARDING_HEADERS_WPE_EXTENSION_DIR}/wpe
    DEPENDS ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe
    COMMAND ln -n -s -f ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe ${FORWARDING_HEADERS_WPE_EXTENSION_DIR}/wpe
)

add_custom_command(
    OUTPUT ${FORWARDING_HEADERS_WPE_DOM_DIR}/wpe
    DEPENDS ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/DOM
    COMMAND ln -n -s -f ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/DOM ${FORWARDING_HEADERS_WPE_DOM_DIR}/wpe
    VERBATIM
)

add_custom_target(webkitwpe-fake-api-headers
    DEPENDS ${FORWARDING_HEADERS_WPE_DIR}/wpe
            ${FORWARDING_HEADERS_WPE_EXTENSION_DIR}/wpe
            ${FORWARDING_HEADERS_WPE_DOM_DIR}/wpe
)

set(WEBKIT_EXTRA_DEPENDENCIES
    webkitwpe-fake-api-headers
    webkitwpe-forwarding-headers
)

list(APPEND WebProcess_SOURCES
    WebProcess/EntryPoint/unix/WebProcessMain.cpp
)

list(APPEND NetworkProcess_SOURCES
    NetworkProcess/EntryPoint/unix/NetworkProcessMain.cpp
)

list(APPEND StorageProcess_SOURCES
    StorageProcess/EntryPoint/unix/StorageProcessMain.cpp
)

list(APPEND WebKit_UNIFIED_SOURCE_LIST_FILES
    "SourcesWPE.txt"
)

list(APPEND WebKit_MESSAGES_IN_FILES
    NetworkProcess/CustomProtocols/LegacyCustomProtocolManager.messages.in

    UIProcess/API/wpe/CompositingManagerProxy.messages.in

    UIProcess/Network/CustomProtocols/LegacyCustomProtocolManagerProxy.messages.in
)

list(APPEND WebKit_DERIVED_SOURCES
    ${DERIVED_SOURCES_WEBKIT_DIR}/WebKitResourcesGResourceBundle.c

    ${DERIVED_SOURCES_WPE_API_DIR}/WebKitEnumTypes.cpp
    ${DERIVED_SOURCES_WPE_API_DIR}/WebKitWebProcessEnumTypes.cpp
)

set(WPE_API_INSTALLED_HEADERS
    ${DERIVED_SOURCES_WPE_API_DIR}/WebKitEnumTypes.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitApplicationInfo.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitAuthenticationRequest.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitAutomationSession.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitBackForwardList.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitBackForwardListItem.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitCredential.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitContextMenu.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitContextMenuActions.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitContextMenuItem.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitCookieManager.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitDefines.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitDownload.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitEditingCommands.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitEditorState.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitError.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitFaviconDatabase.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitFileChooserRequest.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitFindController.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitFormSubmissionRequest.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitGeolocationPermissionRequest.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitHitTestResult.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitInstallMissingMediaPluginsPermissionRequest.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitJavascriptResult.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitMimeInfo.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitNavigationAction.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitNavigationPolicyDecision.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitNetworkProxySettings.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitNotificationPermissionRequest.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitNotification.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitPermissionRequest.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitPlugin.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitPolicyDecision.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitResponsePolicyDecision.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitScriptDialog.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitSecurityManager.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitSecurityOrigin.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitSettings.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitURIRequest.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitURIResponse.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitURISchemeRequest.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitUserContent.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitUserContentManager.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitUserMediaPermissionRequest.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitWebContext.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitWebResource.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitWebView.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitWebViewBackend.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitWebViewSessionState.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitWebsiteData.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitWebsiteDataManager.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitWindowProperties.h
    ${WEBKIT_DIR}/UIProcess/API/wpe/webkit.h
)

set(WPE_WEB_EXTENSION_API_INSTALLED_HEADERS
    ${DERIVED_SOURCES_WPE_API_DIR}/WebKitWebProcessEnumTypes.h
    ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/WebKitConsoleMessage.h
    ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/WebKitFrame.h
    ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/WebKitScriptWorld.h
    ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/WebKitWebEditor.h
    ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/WebKitWebExtension.h
    ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/WebKitWebHitTestResult.h
    ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/WebKitWebPage.h
    ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/webkit-web-extension.h
    ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/DOM/webkitdom.h
    ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/DOM/WebKitDOMDefines.h
    ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/DOM/WebKitDOMDocument.h
    ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/DOM/WebKitDOMElement.h
    ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/DOM/WebKitDOMNode.h
    ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/DOM/WebKitDOMObject.h
)

# To generate WebKitEnumTypes.h we want to use all installed headers, except WebKitEnumTypes.h itself.
set(WPE_ENUM_GENERATION_HEADERS ${WPE_API_INSTALLED_HEADERS})
list(REMOVE_ITEM WPE_ENUM_GENERATION_HEADERS ${DERIVED_SOURCES_WPE_API_DIR}/WebKitEnumTypes.h)
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WPE_API_DIR}/WebKitEnumTypes.h
           ${DERIVED_SOURCES_WPE_API_DIR}/WebKitEnumTypes.cpp
    DEPENDS ${WPE_ENUM_GENERATION_HEADERS}

    COMMAND glib-mkenums --template ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitEnumTypes.h.template ${WPE_ENUM_GENERATION_HEADERS} | sed s/web_kit/webkit/ | sed s/WEBKIT_TYPE_KIT/WEBKIT_TYPE/ > ${DERIVED_SOURCES_WPE_API_DIR}/WebKitEnumTypes.h

    COMMAND glib-mkenums --template ${WEBKIT_DIR}/UIProcess/API/wpe/WebKitEnumTypes.cpp.template ${WPE_ENUM_GENERATION_HEADERS} | sed s/web_kit/webkit/ > ${DERIVED_SOURCES_WPE_API_DIR}/WebKitEnumTypes.cpp
    VERBATIM
)

set(WPE_WEB_PROCESS_ENUM_GENERATION_HEADERS ${WPE_WEB_EXTENSION_API_INSTALLED_HEADERS})
list(REMOVE_ITEM WPE_WEB_PROCESS_ENUM_GENERATION_HEADERS ${DERIVED_SOURCES_WPE_API_DIR}/WebKitWebProcessEnumTypes.h)
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WPE_API_DIR}/WebKitWebProcessEnumTypes.h
           ${DERIVED_SOURCES_WPE_API_DIR}/WebKitWebProcessEnumTypes.cpp
    DEPENDS ${WPE_WEB_PROCESS_ENUM_GENERATION_HEADERS}

    COMMAND glib-mkenums --template ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/WebKitWebProcessEnumTypes.h.template ${WPE_WEB_PROCESS_ENUM_GENERATION_HEADERS} | sed s/web_kit/webkit/ | sed s/WEBKIT_TYPE_KIT/WEBKIT_TYPE/ > ${DERIVED_SOURCES_WPE_API_DIR}/WebKitWebProcessEnumTypes.h

    COMMAND glib-mkenums --template ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/WebKitWebProcessEnumTypes.cpp.template ${WPE_WEB_PROCESS_ENUM_GENERATION_HEADERS} | sed s/web_kit/webkit/ > ${DERIVED_SOURCES_WPE_API_DIR}/WebKitWebProcessEnumTypes.cpp
    VERBATIM
)

set(WebKitResources
)

if (ENABLE_WEB_AUDIO)
    list(APPEND WebKitResources
        "        <file alias=\"audio/Composite\">Composite.wav</file>\n"
    )
endif ()

file(WRITE ${DERIVED_SOURCES_WEBKIT_DIR}/WebKitResourcesGResourceBundle.xml
    "<?xml version=1.0 encoding=UTF-8?>\n"
    "<gresources>\n"
    "    <gresource prefix=\"/org/webkitwpe/resources\">\n"
    ${WebKitResources}
    "    </gresource>\n"
    "</gresources>\n"
)

add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBKIT_DIR}/WebKitResourcesGResourceBundle.c
    DEPENDS ${DERIVED_SOURCES_WEBKIT_DIR}/WebKitResourcesGResourceBundle.xml
    COMMAND glib-compile-resources --generate --sourcedir=${CMAKE_SOURCE_DIR}/Source/WebCore/Resources --sourcedir=${CMAKE_SOURCE_DIR}/Source/WebCore/platform/audio/resources --target=${DERIVED_SOURCES_WEBKIT_DIR}/WebKitResourcesGResourceBundle.c ${DERIVED_SOURCES_WEBKIT_DIR}/WebKitResourcesGResourceBundle.xml
    VERBATIM
)

list(APPEND WebKit_INCLUDE_DIRECTORIES
    "${DERIVED_SOURCES_JAVASCRIPCOREWPE_DIR}"
    "${FORWARDING_HEADERS_DIR}"
    "${FORWARDING_HEADERS_DIR}/JavaScriptCore/"
    "${FORWARDING_HEADERS_DIR}/JavaScriptCore/glib"
    "${FORWARDING_HEADERS_WPE_DIR}"
    "${FORWARDING_HEADERS_WPE_EXTENSION_DIR}"
    "${FORWARDING_HEADERS_WPE_DOM_DIR}"
    "${DERIVED_SOURCES_DIR}"
    "${DERIVED_SOURCES_WPE_API_DIR}"
    "${WEBCORE_DIR}/platform/graphics/cairo"
    "${WEBCORE_DIR}/platform/graphics/freetype"
    "${WEBCORE_DIR}/platform/graphics/opentype"
    "${WEBCORE_DIR}/platform/graphics/texmap/coordinated"
    "${WEBCORE_DIR}/platform/network/soup"
    "${WEBKIT_DIR}/NetworkProcess/CustomProtocols/soup"
    "${WEBKIT_DIR}/NetworkProcess/soup"
    "${WEBKIT_DIR}/NetworkProcess/unix"
    "${WEBKIT_DIR}/Platform/IPC/glib"
    "${WEBKIT_DIR}/Platform/IPC/unix"
    "${WEBKIT_DIR}/Platform/classifier"
    "${WEBKIT_DIR}/Shared/API/c/wpe"
    "${WEBKIT_DIR}/Shared/API/glib"
    "${WEBKIT_DIR}/Shared/CoordinatedGraphics"
    "${WEBKIT_DIR}/Shared/CoordinatedGraphics/threadedcompositor"
    "${WEBKIT_DIR}/Shared/glib"
    "${WEBKIT_DIR}/Shared/soup"
    "${WEBKIT_DIR}/Shared/unix"
    "${WEBKIT_DIR}/Shared/wpe"
    "${WEBKIT_DIR}/StorageProcess/unix"
    "${WEBKIT_DIR}/UIProcess/API/C/cairo"
    "${WEBKIT_DIR}/UIProcess/API/C/soup"
    "${WEBKIT_DIR}/UIProcess/API/C/wpe"
    "${WEBKIT_DIR}/UIProcess/API/glib"
    "${WEBKIT_DIR}/UIProcess/API/wpe"
    "${WEBKIT_DIR}/UIProcess/InspectorServer"
    "${WEBKIT_DIR}/UIProcess/Network/CustomProtocols/soup"
    "${WEBKIT_DIR}/UIProcess/gstreamer"
    "${WEBKIT_DIR}/UIProcess/linux"
    "${WEBKIT_DIR}/UIProcess/soup"
    "${WEBKIT_DIR}/UIProcess/wpe"
    "${WEBKIT_DIR}/WebProcess/InjectedBundle/API/glib"
    "${WEBKIT_DIR}/WebProcess/InjectedBundle/API/glib/DOM"
    "${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe"
    "${WEBKIT_DIR}/WebProcess/InjectedBundle/API/wpe/DOM"
    "${WEBKIT_DIR}/WebProcess/soup"
    "${WEBKIT_DIR}/WebProcess/unix"
    "${WEBKIT_DIR}/WebProcess/WebCoreSupport/soup"
    "${WEBKIT_DIR}/WebProcess/WebPage/CoordinatedGraphics"
    "${WEBKIT_DIR}/WebProcess/WebPage/wpe"
    "${WTF_DIR}/wtf/gtk/"
    "${WTF_DIR}/wtf/gobject"
    "${WTF_DIR}"
)

list(APPEND WebKit_SYSTEM_INCLUDE_DIRECTORIES
    ${BCM_HOST_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${FREETYPE2_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${HARFBUZZ_INCLUDE_DIRS}
    ${LIBSOUP_INCLUDE_DIRS}
    ${WPE_INCLUDE_DIRS}
)

list(APPEND WebKit_LIBRARIES
    PRIVATE
        ${BCM_HOST_LIBRARIES}
        ${CAIRO_LIBRARIES}
        ${FREETYPE2_LIBRARIES}
        ${GLIB_LIBRARIES}
        ${GSTREAMER_LIBRARIES}
        ${HARFBUZZ_LIBRARIES}
        ${LIBSOUP_LIBRARIES}
        ${WPE_LIBRARIES}
)

WEBKIT_BUILD_INSPECTOR_GRESOURCES(${DERIVED_SOURCES_WEBINSPECTORUI_DIR})

if (ENABLE_BREAKPAD)
    list(APPEND WebProcess_INCLUDE_DIRECTORIES
        ${BREAKPAD_INCLUDE_DIRS}
    )

    list(APPEND WebProcess_LIBRARIES
        ${BREAKPAD_LIBRARIES}
    )

    list(APPEND NetworkProcess_INCLUDE_DIRECTORIES
        ${BREAKPAD_INCLUDE_DIRS}
    )

    list(APPEND NetworkProcess_LIBRARIES
        ${BREAKPAD_LIBRARIES}
    )
endif ()

add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/WebKit2InspectorGResourceBundle.c
    DEPENDS ${WEBKIT_DIR}/UIProcess/API/wpe/WebKit2InspectorGResourceBundle.xml
            ${WEBKIT_DIR}/UIProcess/InspectorServer/front-end/inspectorPageIndex.html
    COMMAND glib-compile-resources --generate --sourcedir=${WEBKIT_DIR}/UIProcess/InspectorServer/front-end --target=${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/WebKit2InspectorGResourceBundle.c ${WEBKIT_DIR}/UIProcess/API/wpe/WebKit2InspectorGResourceBundle.xml
    VERBATIM
)

list(APPEND WPEWebInspectorResources_DERIVED_SOURCES
    ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/InspectorGResourceBundle.c
    ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/WebKit2InspectorGResourceBundle.c
)

list(APPEND WPEWebInspectorResources_LIBRARIES
    ${GLIB_GIO_LIBRARIES}
)

list(APPEND WPEWebInspectorResources_SYSTEM_INCLUDE_DIRECTORIES
    ${GLIB_INCLUDE_DIRS}
)

add_library(WPEWebInspectorResources SHARED ${WPEWebInspectorResources_DERIVED_SOURCES})
add_dependencies(WPEWebInspectorResources WebKit)
target_link_libraries(WPEWebInspectorResources ${WPEWebInspectorResources_LIBRARIES})
target_include_directories(WPEWebInspectorResources SYSTEM PUBLIC ${WPEWebInspectorResources_SYSTEM_INCLUDE_DIRECTORIES})
install(TARGETS WPEWebInspectorResources DESTINATION "${LIB_INSTALL_DIR}/wpe-webkit-${WPE_API_VERSION}")

add_library(WPEInjectedBundle MODULE "${WEBKIT_DIR}/WebProcess/InjectedBundle/API/glib/WebKitInjectedBundleMain.cpp")
ADD_WEBKIT_PREFIX_HEADER(WPEInjectedBundle)
target_link_libraries(WPEInjectedBundle WebKit)

target_include_directories(WPEInjectedBundle PRIVATE ${WebKit_INCLUDE_DIRECTORIES})
target_include_directories(WPEInjectedBundle SYSTEM PRIVATE ${WebKit_SYSTEM_INCLUDE_DIRECTORIES})

if (EXPORT_DEPRECATED_WEBKIT2_C_API)
    set(WPE_INSTALLED_WEBKIT_HEADERS
        ${WEBKIT_DIR}/Shared/API/c/WKArray.h
        ${WEBKIT_DIR}/Shared/API/c/WKBase.h
        ${WEBKIT_DIR}/Shared/API/c/WKData.h
        ${WEBKIT_DIR}/Shared/API/c/WKDeclarationSpecifiers.h
        ${WEBKIT_DIR}/Shared/API/c/WKDiagnosticLoggingResultType.h
        ${WEBKIT_DIR}/Shared/API/c/WKDictionary.h
        ${WEBKIT_DIR}/Shared/API/c/WKErrorRef.h
        ${WEBKIT_DIR}/Shared/API/c/WKEvent.h
        ${WEBKIT_DIR}/Shared/API/c/WKFindOptions.h
        ${WEBKIT_DIR}/Shared/API/c/WKGeometry.h
        ${WEBKIT_DIR}/Shared/API/c/WKImage.h
        ${WEBKIT_DIR}/Shared/API/c/WKMutableArray.h
        ${WEBKIT_DIR}/Shared/API/c/WKMutableDictionary.h
        ${WEBKIT_DIR}/Shared/API/c/WKNumber.h
        ${WEBKIT_DIR}/Shared/API/c/WKPageLoadTypes.h
        ${WEBKIT_DIR}/Shared/API/c/WKPageVisibilityTypes.h
        ${WEBKIT_DIR}/Shared/API/c/WKSecurityOriginRef.h
        ${WEBKIT_DIR}/Shared/API/c/WKSerializedScriptValue.h
        ${WEBKIT_DIR}/Shared/API/c/WKString.h
        ${WEBKIT_DIR}/Shared/API/c/WKType.h
        ${WEBKIT_DIR}/Shared/API/c/WKURL.h
        ${WEBKIT_DIR}/Shared/API/c/WKURLRequest.h
        ${WEBKIT_DIR}/Shared/API/c/WKURLResponse.h
        ${WEBKIT_DIR}/Shared/API/c/WKUserContentInjectedFrames.h
        ${WEBKIT_DIR}/Shared/API/c/WKUserContentURLPattern.h
        ${WEBKIT_DIR}/Shared/API/c/WKUserScriptInjectionTime.h

        ${WEBKIT_DIR}/Shared/API/c/wpe/WKBaseWPE.h

        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundle.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundleBackForwardList.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundleBackForwardListItem.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundleDOMWindowExtension.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundleFileHandleRef.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundleFrame.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundleHitTestResult.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundleInitialize.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundleInspector.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundleNavigationAction.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundleNodeHandle.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundlePage.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundlePageBanner.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundlePageContextMenuClient.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundlePageEditorClient.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundlePageFormClient.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundlePageFullScreenClient.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundlePageGroup.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundlePageLoaderClient.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundlePageOverlay.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundlePagePolicyClient.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundlePageResourceLoadClient.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundlePageUIClient.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundleRangeHandle.h
        ${WEBKIT_DIR}/WebProcess/InjectedBundle/API/c/WKBundleScriptWorld.h

        ${WEBKIT_DIR}/UIProcess/API/C/WKBackForwardListItemRef.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKBackForwardListRef.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKContextAutomationClient.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKContextConfigurationRef.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKContextConnectionClient.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKContextDownloadClient.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKContextHistoryClient.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKContextInjectedBundleClient.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKContext.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKCookie.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKCookieManager.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKCredential.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKCredentialTypes.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKFrame.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKFrameInfoRef.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKFramePolicyListener.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKHTTPCookieStorageRef.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKHitTestResult.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKNativeEvent.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKNavigationActionRef.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKNavigationDataRef.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKNavigationRef.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKNavigationResponseRef.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKPage.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKPageConfigurationRef.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKPageContextMenuClient.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKPageDiagnosticLoggingClient.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKPageFindClient.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKPageFindMatchesClient.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKPageFormClient.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKPageGroup.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKPageInjectedBundleClient.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKPageLoaderClient.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKPageNavigationClient.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKPagePolicyClient.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKPageRenderingProgressEvents.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKPageUIClient.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKPluginLoadPolicy.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKPreferencesRef.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKProcessTerminationReason.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKSessionStateRef.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKUserContentControllerRef.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKUserScriptRef.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKViewportAttributes.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKWebAutomationSession.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKWebAutomationSessionClient.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKWindowFeaturesRef.h

        ${WEBKIT_DIR}/UIProcess/API/C/WKGeolocationManager.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKGeolocationPermissionRequest.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKGeolocationPosition.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKNotificationPermissionRequest.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKNotificationProvider.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKNotification.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKNotificationManager.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKUserMediaPermissionRequest.h
        ${WEBKIT_DIR}/UIProcess/API/C/WKProxy.h

        ${WEBKIT_DIR}/UIProcess/API/C/wpe/WKView.h

        ${WEBKIT_DIR}/UIProcess/API/C/soup/WKCookieManagerSoup.h
        ${WEBKIT_DIR}/UIProcess/API/C/soup/WKSoupSession.h
    )

    install(FILES ${WPE_INSTALLED_WEBKIT_HEADERS}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/wpe-webkit-deprecated-${WPE_API_VERSION}/WPE/WebKit"
        COMPONENT "Development"
    )

    set(WPE_INSTALLED_HEADERS
        ${WEBKIT_DIR}/Shared/API/c/wpe/WebKit.h
    )

    install(FILES ${WPE_INSTALLED_HEADERS}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/wpe-webkit-deprecated-${WPE_API_VERSION}/WPE"
        COMPONENT "Development"
    )
endif()

install(TARGETS WPEInjectedBundle
        DESTINATION "${LIB_INSTALL_DIR}/wpe-webkit-${WPE_API_VERSION}/injected-bundle"
)

install(FILES "${CMAKE_BINARY_DIR}/wpe-webkit-${WPE_API_VERSION}.pc"
              "${CMAKE_BINARY_DIR}/wpe-web-extension-${WPE_API_VERSION}.pc"
              "${CMAKE_BINARY_DIR}/wpe-webkit-deprecated-${WPE_API_VERSION}.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
        COMPONENT "Development"
)

install(FILES ${WPE_API_INSTALLED_HEADERS}
              ${WPE_WEB_EXTENSION_API_INSTALLED_HEADERS}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/wpe-webkit-${WPE_API_VERSION}/wpe"
        COMPONENT "Development"
)
