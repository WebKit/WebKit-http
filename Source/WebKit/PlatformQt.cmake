include(ECMGenerateHeaders)

if (NOT MSVC)
    add_definitions("-include WebKitPrefix.h")
endif ()

list(APPEND WebKit_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}"
    "${DERIVED_SOURCES_DIR}"
    "${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}"
    "${DERIVED_SOURCES_WEBCORE_DIR}"
    "${JAVASCRIPTCORE_DIR}"

    # Copied from WebCore/CMakeLists.txt
    "${WEBCORE_DIR}/Modules/airplay"
    "${WEBCORE_DIR}/Modules/battery"
    "${WEBCORE_DIR}/Modules/encryptedmedia"
    "${WEBCORE_DIR}/Modules/fetch"
    "${WEBCORE_DIR}/Modules/geolocation"
    "${WEBCORE_DIR}/Modules/indexeddb"
    "${WEBCORE_DIR}/Modules/indexeddb/client"
    "${WEBCORE_DIR}/Modules/indexeddb/legacy"
    "${WEBCORE_DIR}/Modules/indexeddb/server"
    "${WEBCORE_DIR}/Modules/indexeddb/shared"
    "${WEBCORE_DIR}/Modules/indieui"
    "${WEBCORE_DIR}/Modules/mediacontrols/"
    "${WEBCORE_DIR}/Modules/mediasession"
    "${WEBCORE_DIR}/Modules/mediasource"
    "${WEBCORE_DIR}/Modules/mediastream"
    "${WEBCORE_DIR}/Modules/navigatorcontentutils"
    "${WEBCORE_DIR}/Modules/notifications"
    "${WEBCORE_DIR}/Modules/plugins"
    "${WEBCORE_DIR}/Modules/proximity"
    "${WEBCORE_DIR}/Modules/quota"
    "${WEBCORE_DIR}/Modules/speech"
    "${WEBCORE_DIR}/Modules/streams"
    "${WEBCORE_DIR}/Modules/vibration"
    "${WEBCORE_DIR}/Modules/webaudio"
    "${WEBCORE_DIR}/Modules/webdatabase"
    "${WEBCORE_DIR}/Modules/websockets"
    "${WEBCORE_DIR}/accessibility"
    "${WEBCORE_DIR}/bindings"
    "${WEBCORE_DIR}/bindings/generic"
    "${WEBCORE_DIR}/bindings/js"
    "${WEBCORE_DIR}/bridge"
    "${WEBCORE_DIR}/bridge/c"
    "${WEBCORE_DIR}/bridge/jsc"
    "${WEBCORE_DIR}/contentextensions"
    "${WEBCORE_DIR}/crypto"
    "${WEBCORE_DIR}/crypto/algorithms"
    "${WEBCORE_DIR}/crypto/keys"
    "${WEBCORE_DIR}/crypto/parameters"
    "${WEBCORE_DIR}/css"
    "${WEBCORE_DIR}/cssjit"
    "${WEBCORE_DIR}/dom"
    "${WEBCORE_DIR}/dom/default"
    "${WEBCORE_DIR}/editing"
    "${WEBCORE_DIR}/fileapi"
    "${WEBCORE_DIR}/history"
    "${WEBCORE_DIR}/html"
    "${WEBCORE_DIR}/html/canvas"
    "${WEBCORE_DIR}/html/forms"
    "${WEBCORE_DIR}/html/parser"
    "${WEBCORE_DIR}/html/shadow"
    "${WEBCORE_DIR}/html/track"
    "${WEBCORE_DIR}/inspector"
    "${WEBCORE_DIR}/loader"
    "${WEBCORE_DIR}/loader/appcache"
    "${WEBCORE_DIR}/loader/archive"
    "${WEBCORE_DIR}/loader/archive/mhtml"
    "${WEBCORE_DIR}/loader/cache"
    "${WEBCORE_DIR}/loader/icon"
    "${WEBCORE_DIR}/mathml"
    "${WEBCORE_DIR}/page"
    "${WEBCORE_DIR}/page/animation"
    "${WEBCORE_DIR}/page/csp"
    "${WEBCORE_DIR}/page/scrolling"
    "${WEBCORE_DIR}/platform"
    "${WEBCORE_DIR}/platform/animation"
    "${WEBCORE_DIR}/platform/audio"
    "${WEBCORE_DIR}/platform/graphics"
    "${WEBCORE_DIR}/platform/graphics/cpu/arm"
    "${WEBCORE_DIR}/platform/graphics/cpu/arm/filters"
    "${WEBCORE_DIR}/platform/graphics/displaylists"
    "${WEBCORE_DIR}/platform/graphics/filters"
    "${WEBCORE_DIR}/platform/graphics/filters/texmap"
    "${WEBCORE_DIR}/platform/graphics/harfbuzz"
    "${WEBCORE_DIR}/platform/graphics/harfbuzz/ng"
    "${WEBCORE_DIR}/platform/graphics/opentype"
    "${WEBCORE_DIR}/platform/graphics/texmap"
    "${WEBCORE_DIR}/platform/graphics/transforms"
    "${WEBCORE_DIR}/platform/image-decoders"
    "${WEBCORE_DIR}/platform/image-decoders/bmp"
    "${WEBCORE_DIR}/platform/image-decoders/gif"
    "${WEBCORE_DIR}/platform/image-decoders/ico"
    "${WEBCORE_DIR}/platform/image-decoders/jpeg"
    "${WEBCORE_DIR}/platform/image-decoders/png"
    "${WEBCORE_DIR}/platform/image-decoders/webp"
    "${WEBCORE_DIR}/platform/mediastream"
    "${WEBCORE_DIR}/platform/mock"
    "${WEBCORE_DIR}/platform/mock/mediasource"
    "${WEBCORE_DIR}/platform/network"
    "${WEBCORE_DIR}/platform/sql"
    "${WEBCORE_DIR}/platform/text"
    "${WEBCORE_DIR}/platform/text/icu"
    "${WEBCORE_DIR}/plugins"
    "${WEBCORE_DIR}/rendering"
    "${WEBCORE_DIR}/rendering/line"
    "${WEBCORE_DIR}/rendering/mathml"
    "${WEBCORE_DIR}/rendering/shapes"
    "${WEBCORE_DIR}/rendering/style"
    "${WEBCORE_DIR}/rendering/svg"
    "${WEBCORE_DIR}/replay"
    "${WEBCORE_DIR}/storage"
    "${WEBCORE_DIR}/style"
    "${WEBCORE_DIR}/svg"
    "${WEBCORE_DIR}/svg/animation"
    "${WEBCORE_DIR}/svg/graphics"
    "${WEBCORE_DIR}/svg/graphics/filters"
    "${WEBCORE_DIR}/svg/properties"
    "${WEBCORE_DIR}/testing/js"
    "${WEBCORE_DIR}/websockets"
    "${WEBCORE_DIR}/workers"
    "${WEBCORE_DIR}/xml"
    "${WEBCORE_DIR}/xml/parser"

    "${WEBCORE_DIR}/bridge/qt"
    "${WEBCORE_DIR}/history/qt"
    "${WEBCORE_DIR}/platform"
    "${WEBCORE_DIR}/platform/animation"
    "${WEBCORE_DIR}/platform/qt"
    "${WEBCORE_DIR}/platform/audio/qt"
    "${WEBCORE_DIR}/platform/graphics"
    "${WEBCORE_DIR}/platform/graphics/qt"
    "${WEBCORE_DIR}/platform/graphics/gpu/qt"
    "${WEBCORE_DIR}/platform/graphics/surfaces/qt"
    "${WEBCORE_DIR}/platform/network"
    "${WEBCORE_DIR}/platform/network/qt"
    "${WEBCORE_DIR}/platform/text/qt"
    "${WEBCORE_DIR}/rendering"
    "${WEBCORE_DIR}/rendering/style"

    "${WEBKIT_DIR}/.."
    "${WEBKIT_DIR}/Storage"
    "${WEBKIT_DIR}/qt"
    "${WEBKIT_DIR}/qt/Api"
    "${WEBKIT_DIR}/qt/WebCoreSupport"

    "${WTF_DIR}"
)

# This files are not really port-independent
list(REMOVE_ITEM WebKit_SOURCES
    WebCoreSupport/WebViewGroup.cpp
)

list(APPEND WebKit_SOURCES
    Storage/StorageAreaImpl.cpp
    Storage/StorageAreaSync.cpp
    Storage/StorageNamespaceImpl.cpp
    Storage/StorageSyncManager.cpp
    Storage/StorageThread.cpp
    Storage/StorageTracker.cpp
    Storage/WebDatabaseProvider.cpp
    Storage/WebStorageNamespaceProvider.cpp

    qt/Api/qhttpheader.cpp
    qt/Api/qwebdatabase.cpp
    qt/Api/qwebelement.cpp
    qt/Api/qwebhistory.cpp
    qt/Api/qwebhistoryinterface.cpp
    qt/Api/qwebkitglobal.cpp
    qt/Api/qwebplugindatabase.cpp
    qt/Api/qwebpluginfactory.cpp
    qt/Api/qwebscriptworld.cpp
    qt/Api/qwebsecurityorigin.cpp
    qt/Api/qwebsettings.cpp

    qt/WebCoreSupport/ChromeClientQt.cpp
    qt/WebCoreSupport/ContextMenuClientQt.cpp
    qt/WebCoreSupport/DragClientQt.cpp
    qt/WebCoreSupport/EditorClientQt.cpp
    qt/WebCoreSupport/FrameLoaderClientQt.cpp
    qt/WebCoreSupport/FrameNetworkingContextQt.cpp
    qt/WebCoreSupport/IconDatabaseClientQt.cpp
    qt/WebCoreSupport/InitWebCoreQt.cpp
    qt/WebCoreSupport/InspectorClientQt.cpp
    qt/WebCoreSupport/InspectorServerQt.cpp
    qt/WebCoreSupport/NotificationPresenterClientQt.cpp
    qt/WebCoreSupport/PlatformStrategiesQt.cpp
    qt/WebCoreSupport/PopupMenuQt.cpp
    qt/WebCoreSupport/ProgressTrackerClientQt.cpp
    qt/WebCoreSupport/QWebFrameAdapter.cpp
    qt/WebCoreSupport/QWebPageAdapter.cpp
    qt/WebCoreSupport/QtPlatformPlugin.cpp
    qt/WebCoreSupport/QtPluginWidgetAdapter.cpp
    qt/WebCoreSupport/QtPrintContext.cpp
    qt/WebCoreSupport/SearchPopupMenuQt.cpp
    qt/WebCoreSupport/TextCheckerClientQt.cpp
    qt/WebCoreSupport/TextureMapperLayerClientQt.cpp
    qt/WebCoreSupport/UndoStepQt.cpp
    qt/WebCoreSupport/VisitedLinkStoreQt.cpp
    qt/WebCoreSupport/WebEventConversion.cpp
)

# Note: Qt5Network_INCLUDE_DIRS includes Qt5Core_INCLUDE_DIRS
list(APPEND WebKit_SYSTEM_INCLUDE_DIRECTORIES
    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
)
# Build the include path with duplicates removed
list(REMOVE_DUPLICATES WebKit_SYSTEM_INCLUDE_DIRECTORIES)

list(APPEND WebKit_LIBRARIES
    PUBLIC
    ${ICU_LIBRARIES}
    ${Qt5Core_LIBRARIES}
    ${Qt5Gui_LIBRARIES}
    ${Qt5Network_LIBRARIES}
)

if (ENABLE_GEOLOCATION)
    list(APPEND WebKit_SOURCES
        qt/WebCoreSupport/GeolocationClientQt.cpp
        qt/WebCoreSupport/GeolocationPermissionClientQt.cpp
    )
endif ()

if (ENABLE_VIDEO)
    list(APPEND WebKit_SOURCES
        qt/WebCoreSupport/FullScreenVideoQt.cpp
    )
endif ()

WEBKIT_CREATE_FORWARDING_HEADERS(QtWebKit DIRECTORIES qt/Api)

ecm_generate_headers(
    QtWebKit_FORWARDING_HEADERS
    HEADER_NAMES
        QWebElement
        QWebSecurityOrigin
        QWebSettings
    COMMON_HEADER
        QtWebKit
    RELATIVE
        qt/Api
    OUTPUT_DIR
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/QtWebKit"
)

set(WebKit_LIBRARY_TYPE SHARED)
set(WebKit_OUTPUT_NAME Qt5WebKit)


############     WebKitWidgets     ############

set(WebKitWidgets_INCLUDE_DIRECTORIES
    ${WebKit_INCLUDE_DIRECTORIES}
    "${WEBKIT_DIR}/qt/WidgetApi"
    "${WEBKIT_DIR}/qt/WidgetSupport"
)

set(WebKitWidgets_SOURCES
    qt/WidgetApi/qgraphicswebview.cpp
    qt/WidgetApi/qwebframe.cpp
    qt/WidgetApi/qwebinspector.cpp
    qt/WidgetApi/qwebpage.cpp
    qt/WidgetApi/qwebview.cpp
    qt/WidgetApi/qwebviewaccessible.cpp

    qt/WidgetSupport/InitWebKitQt.cpp
    qt/WidgetSupport/InspectorClientWebPage.cpp
    qt/WidgetSupport/PageClientQt.cpp
    qt/WidgetSupport/QGraphicsWidgetPluginImpl.cpp
    qt/WidgetSupport/QStyleFacadeImp.cpp
    qt/WidgetSupport/QWebUndoCommand.cpp
    qt/WidgetSupport/QWidgetPluginImpl.cpp
    qt/WidgetSupport/QtFallbackWebPopup.cpp
    qt/WidgetSupport/QtWebComboBox.cpp
)

qt_wrap_cpp(WebKit WebKitWidgets_SOURCES
    qt/Api/qwebkitplatformplugin.h
)

set(WebKitWidgets_SYSTEM_INCLUDE_DIRECTORIES
    ${WebKit_SYSTEM_INCLUDE_DIRECTORIES}
    ${Qt5Widgets_INCLUDE_DIRS}
)

set(WebKitWidgets_LIBRARIES
    ${WebKit_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    WebKit
)

if (USE_QT_MULTIMEDIA)
    list(APPEND WebKitWidgets_SOURCES
        qt/WidgetSupport/DefaultFullScreenVideoHandler.cpp
    )
    if (NOT USE_GSTREAMER)
        list(APPEND WebKitWidgets_SOURCES
            qt/WidgetSupport/DefaultFullScreenVideoHandler.cpp
        )
    endif ()
endif ()

WEBKIT_CREATE_FORWARDING_HEADERS(QtWebKitWidgets DIRECTORIES qt/WidgetApi)

ecm_generate_headers(
    QtWebKitWidgets_FORWARDING_HEADERS
    HEADER_NAMES
        QWebFrame,QWebHitTestResult
        QWebPage
        QWebView
    COMMON_HEADER
        QtWebKitWidgets
    RELATIVE
        qt/WidgetApi
    OUTPUT_DIR
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/QtWebKitWidgets"
)

if (WIN32)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        enable_language(ASM_MASM)
        list(APPEND WebKit_SOURCES
            win/plugins/PaintHooks.asm
        )
    endif ()

    # Make sure incremental linking is turned off, as it creates unacceptably long link times.
    string(REPLACE "INCREMENTAL:YES" "INCREMENTAL:NO" replace_CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS})
    set(CMAKE_SHARED_LINKER_FLAGS "${replace_CMAKE_SHARED_LINKER_FLAGS} /INCREMENTAL:NO")
    string(REPLACE "INCREMENTAL:YES" "INCREMENTAL:NO" replace_CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
    set(CMAKE_EXE_LINKER_FLAGS "${replace_CMAKE_EXE_LINKER_FLAGS} /INCREMENTAL:NO")

    string(REPLACE "INCREMENTAL:YES" "INCREMENTAL:NO" replace_CMAKE_SHARED_LINKER_FLAGS_DEBUG ${CMAKE_SHARED_LINKER_FLAGS_DEBUG})
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${replace_CMAKE_SHARED_LINKER_FLAGS_DEBUG} /INCREMENTAL:NO")
    string(REPLACE "INCREMENTAL:YES" "INCREMENTAL:NO" replace_CMAKE_EXE_LINKER_FLAGS_DEBUG ${CMAKE_EXE_LINKER_FLAGS_DEBUG})
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${replace_CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL:NO")

    string(REPLACE "INCREMENTAL:YES" "INCREMENTAL:NO" replace_CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO ${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO})
    set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${replace_CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO} /INCREMENTAL:NO")
    string(REPLACE "INCREMENTAL:YES" "INCREMENTAL:NO" replace_CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO})
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${replace_CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /INCREMENTAL:NO")

    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")

    list(APPEND WebKit_INCLUDE_DIRECTORIES
        ${DERIVED_SOURCES_WEBKIT_DIR}
    )

    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT /NODEFAULTLIB:MSVCRTD")

    set(WebKit_POST_BUILD_COMMAND "${DERIVED_SOURCES_WEBKIT_DIR}/postBuild.cmd")
    file(WRITE "${WebKit_POST_BUILD_COMMAND}" "@xcopy /y /d /i /f \"${CMAKE_CURRENT_SOURCE_DIR}/qt/Api/*.h\" \"${DERIVED_SOURCES_WEBKIT_DIR}/QtWebkit\" >nul 2>nul\n")
    file(APPEND "${WebKit_POST_BUILD_COMMAND}" "@xcopy /y /d /i /f \"${CMAKE_CURRENT_SOURCE_DIR}/qt/WidgetApi/*.h\" \"${DERIVED_SOURCES_WEBKIT_DIR}/QtWebkitWidgets\" >nul 2>nul\n")
    add_custom_command(TARGET WebKit POST_BUILD COMMAND ${WebKit_POST_BUILD_COMMAND} VERBATIM)

    ADD_PRECOMPILED_HEADER("WebKitWidgetsPrefix.h" "qt/WebKitWidgetsPrefix.cpp" WebKitWidgets_SOURCES)
endif ()

set(WebKitWidgets_LIBRARY_TYPE SHARED)
set(WebKitWidgets_OUTPUT_NAME Qt5WebKitWidgets)

WEBKIT_FRAMEWORK(WebKitWidgets)
add_dependencies(WebKitWidgets WebKit)

add_subdirectory(qt/tests)
