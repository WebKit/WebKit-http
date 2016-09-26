include(ECMGenerateHeaders)
include(ECMGeneratePkgConfigFile)
include(ECMGeneratePriFile)

macro(generate_header _file _var _content)
    file(GENERATE OUTPUT ${_file} CONTENT ${_content})
    list(APPEND ${_var} ${_file})
    set_source_files_properties(${_file} PROPERTIES GENERATED TRUE)
endmacro()

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
    "${WEBKIT_DIR}/win/Plugins"

    "${WTF_DIR}"
)

# This files are not really port-independent
list(REMOVE_ITEM WebKit_SOURCES
    WebCoreSupport/WebViewGroup.cpp
)

list(APPEND WebKit_SOURCES
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
    qt/WebCoreSupport/HistorySerialization.cpp
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
    qt/WebCoreSupport/WebDatabaseProviderQt.cpp
    qt/WebCoreSupport/WebEventConversion.cpp

    win/Plugins/PluginDatabase.cpp
    win/Plugins/PluginDebug.cpp
    win/Plugins/PluginPackage.cpp
    win/Plugins/PluginStream.cpp
    win/Plugins/PluginView.cpp
)

# Note: Qt5Network_INCLUDE_DIRS includes Qt5Core_INCLUDE_DIRS
list(APPEND WebKit_SYSTEM_INCLUDE_DIRECTORIES
    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
    ${Qt5Positioning_INCLUDE_DIRS}
)
# Build the include path with duplicates removed
list(REMOVE_DUPLICATES WebKit_SYSTEM_INCLUDE_DIRECTORIES)

list(APPEND WebKit_LIBRARIES
    PRIVATE
        ${ICU_LIBRARIES}
        ${Qt5Positioning_LIBRARIES}
        ${X11_X11_LIB}
        ${X11_Xcomposite_LIB}
        ${X11_Xrender_LIB}
    PUBLIC
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

if (USE_QT_MULTIMEDIA)
    qt_wrap_cpp(WebKit WebKit_SOURCES
        qt/Api/qwebfullscreenvideohandler.h
    )
    list(APPEND WebKit_SOURCES
        qt/WebCoreSupport/FullScreenVideoQt.cpp
    )
endif ()

if (ENABLE_TEST_SUPPORT)
    list(APPEND WebKit_SOURCES
        qt/WebCoreSupport/DumpRenderTreeSupportQt.cpp
        qt/WebCoreSupport/QtTestSupport.cpp
    )
    if (SHARED_CORE)
        list(APPEND WebKit_LIBRARIES PUBLIC WebCoreTestSupport)
        install(TARGETS WebCoreTestSupport EXPORT WebKitTargets
                DESTINATION "${LIB_INSTALL_DIR}")
    else ()
        list(APPEND WebKit_LIBRARIES PRIVATE WebCoreTestSupport)
    endif ()
endif ()

if (ENABLE_NETSCAPE_PLUGIN_API)
    list(APPEND WebKit_SOURCES
        win/Plugins/PluginMainThreadScheduler.cpp
        win/Plugins/npapi.cpp
    )

    if (UNIX AND NOT APPLE)
        list(APPEND WebKit_SOURCES
            qt/Plugins/PluginPackageQt.cpp
            qt/Plugins/PluginViewQt.cpp
        )
    endif ()

    if (PLUGIN_BACKEND_XLIB)
        list(APPEND WebKit_SOURCES
            qt/Plugins/QtX11ImageConversion.cpp
        )
    endif ()

    if (WIN32)
        list(APPEND WebKit_INCLUDE_DIRECTORIES
            ${WEBCORE_DIR}/platform/win
        )

        list(APPEND WebKit_SOURCES
            win/Plugins/PluginDatabaseWin.cpp
            win/Plugins/PluginMessageThrottlerWin.cpp
            win/Plugins/PluginPackageWin.cpp
            win/Plugins/PluginViewWin.cpp
        )
    endif ()
else ()
    list(APPEND WebKit_SOURCES
        qt/Plugins/PluginPackageNone.cpp
        qt/Plugins/PluginViewNone.cpp
    )
endif ()

# Resources have to be included directly in the final binary.
# The linker won't pick them from a static library since they aren't referenced.
if (NOT SHARED_CORE)
    qt5_add_resources(WebKit_SOURCES
        "${WEBCORE_DIR}/WebCore.qrc"
    )

    if (ENABLE_INSPECTOR_UI)
        include("${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/PlatformQt.cmake")
        list(APPEND WebKit_SOURCES
            "${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/qrc_WebInspector.cpp"
        )
    endif ()
endif ()

WEBKIT_CREATE_FORWARDING_HEADERS(QtWebKit DIRECTORIES qt/Api)

ecm_generate_headers(
    QtWebKit_FORWARDING_HEADERS
    HEADER_NAMES
        QWebDatabase
        QWebElement,QWebElementCollection
        QWebHistory,QWebHistoryItem
        QWebHistoryInterface
        QWebKitPlatformPlugin,QWebHapticFeedbackPlayer,QWebFullScreenVideoHandler,QWebNotificationData,QWebNotificationPresenter,QWebSelectData,QWebSelectMethod,QWebSpellChecker,QWebTouchModifier
        QWebPluginFactory
        QWebSecurityOrigin
        QWebSettings
    COMMON_HEADER
        QtWebKit
    RELATIVE
        qt/Api
    OUTPUT_DIR
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/QtWebKit"
    REQUIRED_HEADERS
        QtWebKit_HEADERS
)

set(WebKit_PUBLIC_HEADERS
    qt/Api/qwebkitglobal.h
    ${QtWebKit_HEADERS}
    ${QtWebKit_FORWARDING_HEADERS}
)

generate_header("${DERIVED_SOURCES_DIR}/ForwardingHeaders/QtWebKit/qtwebkitversion.h"
    WebKit_PUBLIC_HEADERS
    "#ifndef QT_QTWEBKIT_VERSION_H
#define QT_QTWEBKIT_VERSION_H

#define QTWEBKIT_VERSION_STR \"${PROJECT_VERSION_STRING}\"
#define QTWEBKIT_VERSION 0x05f00${PROJECT_VERSION_MICRO}

#endif
")

generate_header("${DERIVED_SOURCES_DIR}/ForwardingHeaders/QtWebKit/QtWebKitVersion"
    WebKit_PUBLIC_HEADERS
    "#include \"qtwebkitversion.h\"")

generate_header("${DERIVED_SOURCES_DIR}/ForwardingHeaders/QtWebKit/QtWebKitDepends"
    WebKit_PUBLIC_HEADERS
    "#ifdef __cplusplus /* create empty PCH in C mode */
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtNetwork/QtNetwork>
#endif
")

install(
    FILES
        ${WebKit_PUBLIC_HEADERS}
    DESTINATION
        ${KDE_INSTALL_INCLUDEDIR}/QtWebKit
)

file(GLOB WebKit_PRIVATE_HEADERS qt/Api/*_p.h)
install(
    FILES
        ${WebKit_PRIVATE_HEADERS}
    DESTINATION
        ${KDE_INSTALL_INCLUDEDIR}/QtWebKit/${PROJECT_VERSION}/QtWebKit/private
)

ecm_generate_pkgconfig_file(
    BASE_NAME Qt5WebKit
    DEPS "Qt5Core Qt5Gui Qt5Network"
    FILENAME_VAR WebKit_PKGCONFIG_FILENAME
    INSTALL
)

if (KDE_INSTALL_USE_QT_SYS_PATHS)
    set(WebKit_PRI_ARGUMENTS
        INCLUDE_INSTALL_DIR "$$QT_MODULE_INCLUDE_BASE"
        INCLUDE_INSTALL_DIR2 "$$QT_MODULE_INCLUDE_BASE/QtWebKit"
        LIB_INSTALL_DIR "$$QT_MODULE_LIB_BASE"
    )
else ()
    set(WebKit_PRI_ARGUMENTS
        INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR}
        INCLUDE_INSTALL_DIR2 "${KDE_INSTALL_INCLUDEDIR}/QtWebKit"
        SET_RPATH ON
    )
endif ()

ecm_generate_pri_file(
    BASE_NAME webkit
    LIB_NAME QtWebKit
    DEPS "core gui network"
    RUNTIME_DEPS "sensors positioning qml quick webchannel sql core_private gui_private"
    DEFINES QT_WEBKIT_LIB
    QT_MODULES webkit
    FILENAME_VAR WebKit_PRI_FILENAME
    ${WebKit_PRI_ARGUMENTS}
)
install(FILES ${WebKit_PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

if (QT_STATIC_BUILD)
    set(WebKit_LIBRARY_TYPE STATIC)
else ()
    set(WebKit_LIBRARY_TYPE SHARED)
endif ()

if (APPLE)
    set(WebKit_OUTPUT_NAME QtWebKit)
else ()
    set(WebKit_OUTPUT_NAME Qt5WebKit)
endif ()


############     WebKitWidgets     ############

set(WebKitWidgets_INCLUDE_DIRECTORIES
    "${WEBKIT_DIR}/qt/WidgetApi"
    "${WEBKIT_DIR}/qt/WidgetSupport"
)

set(WebKitWidgets_SOURCES
    qt/WidgetApi/qgraphicswebview.cpp
    qt/WidgetApi/qwebframe.cpp
    qt/WidgetApi/qwebinspector.cpp
    qt/WidgetApi/qwebpage.cpp
    qt/WidgetApi/qwebpage_p.cpp
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

set(WebKitWidgets_SYSTEM_INCLUDE_DIRECTORIES
    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
)

set(WebKitWidgets_LIBRARIES
    PRIVATE
        ${Qt5MultimediaWidgets_LIBRARIES}
        ${Qt5PrintSupport_LIBRARIES}
    PUBLIC
        ${Qt5Widgets_LIBRARIES}
        WebKit
)

if (USE_QT_MULTIMEDIA)
    list(APPEND WebKitWidgets_SOURCES
        qt/WidgetSupport/DefaultFullScreenVideoHandler.cpp
        qt/WidgetSupport/FullScreenVideoWidget.cpp
    )
    list(APPEND WebKitWidgets_SYSTEM_INCLUDE_DIRECTORIES
        ${Qt5MultimediaWidgets_INCLUDE_DIRS}
    )
endif ()

WEBKIT_CREATE_FORWARDING_HEADERS(QtWebKitWidgets DIRECTORIES qt/WidgetApi)

ecm_generate_headers(
    QtWebKitWidgets_FORWARDING_HEADERS
    HEADER_NAMES
        QGraphicsWebView
        QWebFrame,QWebHitTestResult
        QWebInspector
        QWebPage
        QWebView
    COMMON_HEADER
        QtWebKitWidgets
    RELATIVE
        qt/WidgetApi
    OUTPUT_DIR
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/QtWebKitWidgets"
    REQUIRED_HEADERS
        QtWebKitWidgets_HEADERS
)

set(WebKitWidgets_PUBLIC_HEADERS
    ${QtWebKitWidgets_HEADERS}
    ${QtWebKitWidgets_FORWARDING_HEADERS}
)

generate_header("${DERIVED_SOURCES_DIR}/ForwardingHeaders/QtWebKitWidgets/qtwebkitwidgetsversion.h"
    WebKitWidgets_PUBLIC_HEADERS
    "#ifndef QT_QTWEBKITWIDGETS_VERSION_H
#define QT_QTWEBKITWIDGETS_VERSION_H

#define QTWEBKITWIDGETS_VERSION_STR \"${PROJECT_VERSION_STRING}\"
#define QTWEBKITWIDGETS_VERSION 0x05f00${PROJECT_VERSION_MICRO}

#endif
")

generate_header("${DERIVED_SOURCES_DIR}/ForwardingHeaders/QtWebKitWidgets/QtWebKitWidgetsVersion"
    WebKitWidgets_PUBLIC_HEADERS
    "#include \"qtwebkitwidgetsversion.h\"")

generate_header("${DERIVED_SOURCES_DIR}/ForwardingHeaders/QtWebKitWidgets/QtWebKitWidgetsDepends"
    WebKitWidgets_PUBLIC_HEADERS
    "#ifdef __cplusplus /* create empty PCH in C mode */
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtNetwork/QtNetwork>
#include <QtWebKit/QtWebKit>
#endif
")

install(
    FILES
        ${WebKitWidgets_PUBLIC_HEADERS}
    DESTINATION
        ${KDE_INSTALL_INCLUDEDIR}/QtWebKitWidgets
)

file(GLOB WebKitWidgets_PRIVATE_HEADERS qt/WidgetApi/*_p.h)
install(
    FILES
        ${WebKitWidgets_PRIVATE_HEADERS}
    DESTINATION
        ${KDE_INSTALL_INCLUDEDIR}/QtWebKitWidgets/${PROJECT_VERSION}/QtWebKitWidgets/private
)

ecm_generate_pkgconfig_file(
    BASE_NAME Qt5WebKitWidgets
    DEPS "Qt5Core Qt5Gui Qt5Network Qt5Widgets Qt5WebKit"
    FILENAME_VAR WebKitWidgets_PKGCONFIG_FILENAME
    INSTALL
)

if (KDE_INSTALL_USE_QT_SYS_PATHS)
    set(WebKitWidgets_PRI_ARGUMENTS
        INCLUDE_INSTALL_DIR "$$QT_MODULE_INCLUDE_BASE"
        INCLUDE_INSTALL_DIR2 "$$QT_MODULE_INCLUDE_BASE/QtWebKitWidgets"
        LIB_INSTALL_DIR "$$QT_MODULE_LIB_BASE"
    )
else ()
    set(WebKitWidgets_PRI_ARGUMENTS
        INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR}
        INCLUDE_INSTALL_DIR2 "${KDE_INSTALL_INCLUDEDIR}/QtWebKitWidgets"
        SET_RPATH ON
    )
endif ()

ecm_generate_pri_file(
    BASE_NAME webkitwidgets
    LIB_NAME QtWebKitWidgets
    DEPS "core gui network widgets webkit"
    RUNTIME_DEPS "sensors positioning widgets_private printsupport opengl sql core_private gui_private"
    DEFINES QT_WEBKITWIDGETS_LIB
    QT_MODULES webkitwidgets
    FILENAME_VAR WebKitWidgets_PRI_FILENAME
    ${WebKitWidgets_PRI_ARGUMENTS}
)
install(FILES ${WebKitWidgets_PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

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
    file(WRITE "${WebKit_POST_BUILD_COMMAND}" "@xcopy /y /d /i /f \"${CMAKE_CURRENT_SOURCE_DIR}/qt/Api/*.h\" \"${DERIVED_SOURCES_DIR}/ForwardingHeaders/QtWebkit\" >nul 2>nul\n")
    file(APPEND "${WebKit_POST_BUILD_COMMAND}" "@xcopy /y /d /i /f \"${CMAKE_CURRENT_SOURCE_DIR}/qt/WidgetApi/*.h\" \"${DERIVED_SOURCES_DIR}/ForwardingHeaders/QtWebkitWidgets\" >nul 2>nul\n")
    add_custom_command(TARGET WebKit POST_BUILD
        COMMAND ${WebKit_POST_BUILD_COMMAND}
        VERBATIM
        DEPENDS ${WebKit_POST_BUILD_COMMAND}
    )

    ADD_PRECOMPILED_HEADER("WebKitWidgetsPrefix.h" "qt/WebKitWidgetsPrefix.cpp" WebKitWidgets_SOURCES)
endif ()

if (QT_STATIC_BUILD)
    set(WebKitWidgets_LIBRARY_TYPE STATIC)
else ()
    set(WebKitWidgets_LIBRARY_TYPE SHARED)
endif ()

if (APPLE)
    set(WebKitWidgets_OUTPUT_NAME QtWebKitWidgets)
else ()
    set(WebKitWidgets_OUTPUT_NAME Qt5WebKitWidgets)
endif ()
set(WebKitWidgets_PRIVATE_HEADERS_LOCATION Headers/${PROJECT_VERSION}/QtWebKitWidgets/Private)

WEBKIT_FRAMEWORK(WebKitWidgets)
add_dependencies(WebKitWidgets WebKit)
set_target_properties(WebKitWidgets PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
install(TARGETS WebKitWidgets EXPORT Qt5WebKitWidgetsTargets
        DESTINATION "${LIB_INSTALL_DIR}"
        INCLUDES DESTINATION "${KDE_INSTALL_INCLUDEDIR}/QtWebKitWidgets"
)

if (USE_LINKER_VERSION_SCRIPT)
    set(VERSION_SCRIPT "${CMAKE_BINARY_DIR}/QtWebKitWidgets.version")
    add_custom_command(TARGET WebKitWidgets PRE_LINK
        COMMAND ${PERL_EXECUTABLE} ${TOOLS_DIR}/qt/generate-version-script.pl ${Qt5_VERSION} > ${VERSION_SCRIPT}
        VERBATIM
    )
    set_target_properties(WebKitWidgets PROPERTIES LINK_FLAGS -Wl,--version-script,${VERSION_SCRIPT})
endif ()

if (COMPILER_IS_GCC_OR_CLANG)
    set_source_files_properties(
        qt/Api/qwebdatabase.cpp
        qt/Api/qwebelement.cpp
        qt/Api/qwebhistory.cpp
        qt/Api/qwebhistoryinterface.cpp
        qt/Api/qwebpluginfactory.cpp
        qt/Api/qwebscriptworld.cpp
        qt/Api/qwebsecurityorigin.cpp
        qt/Api/qwebsettings.cpp

        qt/WidgetApi/qgraphicswebview.cpp
        qt/WidgetApi/qwebframe.cpp
        qt/WidgetApi/qwebinspector.cpp
        qt/WidgetApi/qwebpage.cpp
        qt/WidgetApi/qwebview.cpp
    PROPERTIES
        COMPILE_FLAGS -frtti
    )
endif ()

add_subdirectory(qt/tests)
