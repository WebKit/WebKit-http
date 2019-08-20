include(ECMGenerateHeaders)
include(ECMGeneratePkgConfigFile)
include(ECMGeneratePriFile)

macro(generate_header _file _var _content)
    file(GENERATE OUTPUT ${_file} CONTENT ${_content})
    list(APPEND ${_var} ${_file})
    set_source_files_properties(${_file} PROPERTIES GENERATED TRUE SKIP_AUTOMOC TRUE)
endmacro()

macro(generate_version_header _file _var _prefix)
    set(HEADER_PREFIX ${_prefix})
    configure_file(VersionHeader.h.in ${_file} @ONLY)
    unset(HEADER_PREFIX)
    list(APPEND ${_var} ${_file})
    set_source_files_properties(${_file} PROPERTIES GENERATED TRUE SKIP_AUTOMOC TRUE)
endmacro()

macro(append_lib_names_to_list _lib_names_list)
    foreach (_lib_filename ${ARGN})
        get_filename_component(_lib_name_we ${_lib_filename} NAME_WE)
        if (NOT MSVC)
            string(REGEX REPLACE "^lib" "" _lib_name_we ${_lib_name_we})
        endif ()
        list(APPEND ${_lib_names_list} ${_lib_name_we})
    endforeach ()
endmacro()

if (${JavaScriptCore_LIBRARY_TYPE} MATCHES STATIC)
    add_definitions(-DSTATICALLY_LINKED_WITH_WTF -DSTATICALLY_LINKED_WITH_JavaScriptCore)
endif ()

QTWEBKIT_SKIP_AUTOMOC(WebKitLegacy)

list(APPEND WebKitLegacy_INCLUDE_DIRECTORIES
    "${CMAKE_BINARY_DIR}/include"
    "${WEBKITLEGACY_DIR}/qt/Api"
    "${WEBKITLEGACY_DIR}/qt/WebCoreSupport"
)

list(APPEND WebKitLegacy_PRIVATE_INCLUDE_DIRECTORIES
    "${WEBKITLEGACY_DIR}/Storage"
    "${WEBKITLEGACY_DIR}/win/Plugins"
    "${WebKitLegacy_DERIVED_SOURCES_DIR}"
)

# This files are not really port-independent
list(REMOVE_ITEM WebKitLegacy_SOURCES
    WebCoreSupport/NetworkStorageSessionMap.cpp
    WebCoreSupport/WebViewGroup.cpp
)

list(APPEND WebKitLegacy_SOURCES
    qt/Api/qhttpheader.cpp
    qt/Api/qwebdatabase.cpp
    qt/Api/qwebelement.cpp
    qt/Api/qwebfullscreenrequest.cpp
    qt/Api/qwebhistory.cpp
    qt/Api/qwebhistoryinterface.cpp
    qt/Api/qwebkitglobal.cpp
    qt/Api/qwebkitplatformplugin.h
    qt/Api/qwebplugindatabase.cpp
    qt/Api/qwebpluginfactory.cpp
    qt/Api/qwebscriptworld.cpp
    qt/Api/qwebsecurityorigin.cpp
    qt/Api/qwebsettings.cpp

    qt/WebCoreSupport/BackForwardList.cpp
    qt/WebCoreSupport/ChromeClientQt.cpp
    qt/WebCoreSupport/ContextMenuClientQt.cpp
    qt/WebCoreSupport/DataListSuggestionPickerQt.cpp
    qt/WebCoreSupport/DragClientQt.cpp
    qt/WebCoreSupport/EditorClientQt.cpp
    qt/WebCoreSupport/FrameLoaderClientQt.cpp
    qt/WebCoreSupport/FrameNetworkingContextQt.cpp
    qt/WebCoreSupport/HistorySerialization.cpp
#    qt/WebCoreSupport/IconDatabaseClientQt.cpp
    qt/WebCoreSupport/InitWebCoreQt.cpp
    qt/WebCoreSupport/InspectorClientQt.cpp
    qt/WebCoreSupport/InspectorServerQt.cpp
    qt/WebCoreSupport/NotificationPresenterClientQt.cpp
    qt/WebCoreSupport/PlatformStrategiesQt.cpp
    qt/WebCoreSupport/PluginInfoProviderQt.cpp
    qt/WebCoreSupport/PopupMenuQt.cpp
    qt/WebCoreSupport/ProgressTrackerClientQt.cpp
    qt/WebCoreSupport/QWebFrameAdapter.cpp
    qt/WebCoreSupport/QWebFrameData.cpp
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
list(APPEND WebKitLegacy_SYSTEM_INCLUDE_DIRECTORIES
    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
    ${Qt5Positioning_INCLUDE_DIRS}
    ${SQLITE_INCLUDE_DIR}
)
# Build the include path with duplicates removed
list(REMOVE_DUPLICATES WebKitLegacy_SYSTEM_INCLUDE_DIRECTORIES)

if (ENABLE_WEBKIT)
    if (APPLE)
        set(WEBKIT_LIBRARY -Wl,-force_load WebKit)
    elseif (MSVC)
        set(WEBKIT_LIBRARY "-WHOLEARCHIVE:WebKit${CMAKE_DEBUG_POSTFIX}")
    elseif (UNIX OR MINGW)
        set(WEBKIT_LIBRARY -Wl,--whole-archive WebKit -Wl,--no-whole-archive)
    else ()
        message(WARNING "Unknown system, linking with WebKit may fail!")
        set(WEBKIT_LIBRARY WebKit)
    endif ()
endif ()

list(APPEND WebKitLegacy_LIBRARIES
    PRIVATE
        ${WEBKIT_LIBRARY}
        ${Qt5Quick_LIBRARIES}
        ${Qt5WebChannel_LIBRARIES}
)

list(APPEND WebKitLegacy_LIBRARIES
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
    list(APPEND WebKitLegacy_SOURCES
        qt/WebCoreSupport/GeolocationClientQt.cpp
        qt/WebCoreSupport/GeolocationPermissionClientQt.cpp
    )
endif ()

if (USE_QT_MULTIMEDIA)
    qt_wrap_cpp(WebKitLegacy WebKitLegacy_SOURCES
        qt/Api/qwebfullscreenvideohandler.h
    )
    list(APPEND WebKitLegacy_SOURCES
        qt/WebCoreSupport/FullScreenVideoQt.cpp
    )
endif ()

if (ENABLE_TEST_SUPPORT)
    list(APPEND WebKitLegacy_SOURCES
        qt/WebCoreSupport/DumpRenderTreeSupportQt.cpp
        qt/WebCoreSupport/QtTestSupport.cpp
    )
    if (SHARED_CORE)
        list(APPEND WebKitLegacy_LIBRARIES PUBLIC WebCoreTestSupport)
    else ()
        list(APPEND WebKitLegacy_LIBRARIES PRIVATE WebCoreTestSupport)
    endif ()
endif ()

if (ENABLE_NETSCAPE_PLUGIN_API)
    list(APPEND WebKitLegacy_SOURCES
        win/Plugins/PluginMainThreadScheduler.cpp
        win/Plugins/npapi.cpp
    )

    if (UNIX AND NOT APPLE)
        list(APPEND WebKitLegacy_SOURCES
            qt/Plugins/PluginPackageQt.cpp
            qt/Plugins/PluginViewQt.cpp
        )
    endif ()

    if (WIN32)
        list(APPEND WebKitLegacy_PRIVATE_INCLUDE_DIRECTORIES
            ${WEBCORE_DIR}/platform/win
        )

        list(APPEND WebKitLegacy_SOURCES
            win/Plugins/PluginDatabaseWin.cpp
            win/Plugins/PluginMessageThrottlerWin.cpp
            win/Plugins/PluginPackageWin.cpp
            win/Plugins/PluginViewWin.cpp
        )
    endif ()
else ()
    list(APPEND WebKitLegacy_SOURCES
        qt/Plugins/PluginPackageNone.cpp
        qt/Plugins/PluginViewNone.cpp
    )
endif ()

# Resources have to be included directly in the final binary.
# The linker won't pick them from a static library since they aren't referenced.
if (NOT SHARED_CORE)
    qt5_add_resources(WebKitLegacy_SOURCES
        "${WEBCORE_DIR}/WebCore.qrc"
    )

    if (ENABLE_INSPECTOR_UI)
        include("${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/PlatformQt.cmake")
        list(APPEND WebKitLegacy_SOURCES
            "${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/qrc_WebInspector.cpp"
        )
        set_property(SOURCE "${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/qrc_WebInspector.cpp" PROPERTY SKIP_AUTOMOC ON)
    endif ()
endif ()

set(QtWebKit_PUBLIC_FRAMEWORK_HEADERS
    qt/Api/qwebdatabase.h
    qt/Api/qwebelement.h
    qt/Api/qwebfullscreenrequest.h
    qt/Api/qwebfullscreenvideohandler.h
    qt/Api/qwebhistory.h
    qt/Api/qwebhistoryinterface.h
    qt/Api/qwebkitglobal.h
    qt/Api/qwebkitplatformplugin.h
    qt/Api/qwebpluginfactory.h
    qt/Api/qwebscriptworld.h
    qt/Api/qwebsecurityorigin.h
    qt/Api/qwebsettings.h
)

WEBKIT_MAKE_FORWARDING_HEADERS(WebKitLegacy
    TARGET_NAME QtWebKitFrameworkHeaders
    DESTINATION ${QtWebKit_FRAMEWORK_HEADERS_DIR}
    FILES ${QtWebKit_PUBLIC_FRAMEWORK_HEADERS}
    FLATTENED
)
add_dependencies(QtWebKitFrameworkHeaders WebCorePrivateFrameworkHeaders)

ecm_generate_headers(
    QtWebKit_FORWARDING_HEADERS
    HEADER_NAMES
        QWebDatabase
        QWebElement,QWebElementCollection
        QWebFullScreenRequest
        QWebHistory,QWebHistoryItem
        QWebHistoryInterface
        QWebKitPlatformPlugin,QWebHapticFeedbackPlayer,QWebFullScreenVideoHandler,QWebNotificationData,QWebNotificationPresenter,QWebSelectData,QWebSelectMethod,QWebSpellChecker,QWebTouchModifier
        QWebPluginFactory
        QWebSecurityOrigin
        QWebSettings
    COMMON_HEADER
        QtWebKit
    COMMON_HEADER_EXTRAS
        <QtWebKit/QtWebKitDepends>
        \"qwebkitglobal.h\"
        \"qtwebkitversion.h\"
    COMMON_HEADER_GUARD_NAME
        QT_QTWEBKIT_MODULE_H
    RELATIVE
        qt/Api
    OUTPUT_DIR
        ${QtWebKit_FRAMEWORK_HEADERS_DIR}
    REQUIRED_HEADERS
        QtWebKit_HEADERS
)

set(WebKitLegacy_PUBLIC_HEADERS
    qt/Api/qwebkitglobal.h
    ${QtWebKit_HEADERS}
    ${QtWebKit_FORWARDING_HEADERS}
)

generate_version_header("${QtWebKit_FRAMEWORK_HEADERS_DIR}/qtwebkitversion.h"
    WebKitLegacy_PUBLIC_HEADERS
    QTWEBKIT
)

generate_header("${QtWebKit_FRAMEWORK_HEADERS_DIR}/QtWebKitVersion"
    WebKitLegacy_PUBLIC_HEADERS
    "#include \"qtwebkitversion.h\"")

generate_header("${QtWebKit_FRAMEWORK_HEADERS_DIR}/QtWebKitDepends"
    WebKitLegacy_PUBLIC_HEADERS
    "#ifdef __cplusplus /* create empty PCH in C mode */
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtNetwork/QtNetwork>
#endif
")

install(
    FILES
        ${WebKitLegacy_PUBLIC_HEADERS}
    DESTINATION
        ${KDE_INSTALL_INCLUDEDIR}/QtWebKit
    COMPONENT Data
)

file(GLOB WebKitLegacy_PRIVATE_HEADERS qt/Api/*_p.h)
install(
    FILES
        ${WebKitLegacy_PRIVATE_HEADERS}
    DESTINATION
        ${KDE_INSTALL_INCLUDEDIR}/QtWebKit/${PROJECT_VERSION}/QtWebKit/private
    COMPONENT Data
)

set(WEBKIT_PKGCONFIG_DEPS "Qt5Core Qt5Gui Qt5Network")
set(WEBKIT_PRI_DEPS "core gui network")
set(WEBKIT_PRI_EXTRA_LIBS "")
set(WEBKIT_PRI_RUNTIME_DEPS "core_private gui_private")

if (QT_WEBCHANNEL)
    set(WEBKIT_PRI_RUNTIME_DEPS "webchannel ${WEBKIT_PRI_RUNTIME_DEPS}")
endif ()
if (ENABLE_WEBKIT)
    set(WEBKIT_PRI_RUNTIME_DEPS "qml quick ${WEBKIT_PRI_RUNTIME_DEPS}")
endif ()
if (ENABLE_GEOLOCATION)
    set(WEBKIT_PRI_RUNTIME_DEPS "positioning ${WEBKIT_PRI_RUNTIME_DEPS}")
endif ()
if (ENABLE_DEVICE_ORIENTATION)
    set(WEBKIT_PRI_RUNTIME_DEPS "sensors ${WEBKIT_PRI_RUNTIME_DEPS}")
endif ()
if (USE_MEDIA_FOUNDATION)
    set(WEBKIT_PRI_EXTRA_LIBS "-lmfuuid -lstrmiids ${WEBKIT_PRI_EXTRA_LIBS}")
endif ()
if (USE_QT_MULTIMEDIA)
    set(WEBKIT_PKGCONFIG_DEPS "${WEBKIT_PKGCONFIG_DEPS} Qt5Multimedia")
    set(WEBKIT_PRI_RUNTIME_DEPS "multimedia ${WEBKIT_PRI_RUNTIME_DEPS}")
endif ()

set(WEBKITWIDGETS_PKGCONFIG_DEPS "${WEBKIT_PKGCONFIG_DEPS} Qt5Widgets Qt5WebKit")
set(WEBKITWIDGETS_PRI_DEPS "${WEBKIT_PRI_DEPS} widgets webkit")
set(WEBKITWIDGETS_PRI_RUNTIME_DEPS "${WEBKIT_PRI_RUNTIME_DEPS} widgets_private")

if (Qt5OpenGL_FOUND)
    set(WEBKITWIDGETS_PRI_RUNTIME_DEPS "${WEBKITWIDGETS_PRI_RUNTIME_DEPS} opengl")
endif ()

if (ENABLE_PRINT_SUPPORT)
    set(WEBKITWIDGETS_PRI_RUNTIME_DEPS "${WEBKITWIDGETS_PRI_RUNTIME_DEPS} printsupport")
endif ()

if (USE_QT_MULTIMEDIA)
    set(WEBKITWIDGETS_PKGCONFIG_DEPS "${WEBKITWIDGETS_PKGCONFIG_DEPS} Qt5MultimediaWidgets")
    set(WEBKITWIDGETS_PRI_RUNTIME_DEPS "${WEBKITWIDGETS_PRI_RUNTIME_DEPS} multimediawidgets")
endif ()

if (QT_STATIC_BUILD)
    set(WEBKITWIDGETS_PKGCONFIG_DEPS "${WEBKITWIDGETS_PKGCONFIG_DEPS} Qt5PrintSupport")
    set(WEBKITWIDGETS_PRI_DEPS "${WEBKITWIDGETS_PRI_DEPS} printsupport")
    set(EXTRA_LIBS_NAMES WebCore JavaScriptCore WTF)
    append_lib_names_to_list(EXTRA_LIBS_NAMES ${LIBXML2_LIBRARIES} ${SQLITE_LIBRARIES} ${ZLIB_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES})
    if (NOT USE_SYSTEM_MALLOC)
        list(APPEND EXTRA_LIBS_NAMES bmalloc)
    endif ()
    if (ENABLE_XSLT)
        append_lib_names_to_list(EXTRA_LIBS_NAMES ${LIBXSLT_LIBRARIES})
    endif ()
    if (USE_LIBHYPHEN)
        append_lib_names_to_list(EXTRA_LIBS_NAMES ${HYPHEN_LIBRARIES})
    endif ()
    if (USE_WEBP)
        append_lib_names_to_list(EXTRA_LIBS_NAMES ${WEBP_LIBRARIES})
    endif ()
    if (USE_WOFF2)
        list(APPEND EXTRA_LIBS_NAMES woff2 brotli)
    endif ()
    if (APPLE)
        list(APPEND EXTRA_LIBS_NAMES icucore)
    endif ()
    list(REMOVE_DUPLICATES EXTRA_LIBS_NAMES)
    foreach (LIB_NAME ${EXTRA_LIBS_NAMES})
        set(WEBKIT_PKGCONFIG_DEPS "${WEBKIT_PKGCONFIG_DEPS} ${LIB_PREFIX}${LIB_NAME}")
        set(WEBKIT_PRI_EXTRA_LIBS "${WEBKIT_PRI_EXTRA_LIBS} -l${LIB_PREFIX}${LIB_NAME}")
    endforeach ()
endif ()

if (NOT MACOS_BUILD_FRAMEWORKS)
    ecm_generate_pkgconfig_file(
        BASE_NAME Qt5WebKit
        DESCRIPTION "Qt WebKit module"
        INCLUDE_INSTALL_DIR "${KDE_INSTALL_INCLUDEDIR}/QtWebKit"
        DEPS "${WEBKIT_PKGCONFIG_DEPS}"
        FILENAME_VAR QtWebKit_PKGCONFIG_FILENAME
    )
    set(ECM_PKGCONFIG_INSTALL_DIR "${LIB_INSTALL_DIR}/pkgconfig" CACHE PATH "The directory where pkgconfig will be installed to.")
    install(FILES ${QtWebKit_PKGCONFIG_FILENAME} DESTINATION ${ECM_PKGCONFIG_INSTALL_DIR} COMPONENT Data)
endif ()

if (KDE_INSTALL_USE_QT_SYS_PATHS)
    set(QtWebKit_PRI_ARGUMENTS
        BIN_INSTALL_DIR "$$QT_MODULE_BIN_BASE"
        LIB_INSTALL_DIR "$$QT_MODULE_LIB_BASE"
    )
    if (MACOS_BUILD_FRAMEWORKS)
        list(APPEND QtWebKit_PRI_ARGUMENTS
            INCLUDE_INSTALL_DIR "$$QT_MODULE_LIB_BASE/QtWebKit.framework/Headers"
            MODULE_CONFIG "lib_bundle"
        )
        list(APPEND QtWebKit_Private_PRI_ARGUMENTS
            INCLUDE_INSTALL_DIR "$$QT_MODULE_LIB_BASE/QtWebKit.framework/Headers/${PROJECT_VERSION}"
            INCLUDE_INSTALL_DIR2 "$$QT_MODULE_LIB_BASE/QtWebKit.framework/Headers/${PROJECT_VERSION}/QtWebKit"
        )
    else ()
        list(APPEND QtWebKit_PRI_ARGUMENTS
            INCLUDE_INSTALL_DIR "$$QT_MODULE_INCLUDE_BASE"
            INCLUDE_INSTALL_DIR2 "$$QT_MODULE_INCLUDE_BASE/QtWebKit"
        )
        list(APPEND QtWebKit_Private_PRI_ARGUMENTS
            INCLUDE_INSTALL_DIR "$$QT_MODULE_INCLUDE_BASE/QtWebKit/${PROJECT_VERSION}"
            INCLUDE_INSTALL_DIR2 "$$QT_MODULE_INCLUDE_BASE/QtWebKit/${PROJECT_VERSION}/QtWebKit"
        )
    endif ()
else ()
    set(QtWebKit_PRI_ARGUMENTS
        SET_RPATH ON
    )
    if (MACOS_BUILD_FRAMEWORKS)
        list(APPEND QtWebKit_PRI_ARGUMENTS
            INCLUDE_INSTALL_DIR "${LIB_INSTALL_DIR}/QtWebKit.framework/Headers"
            MODULE_CONFIG "lib_bundle"
        )
        list(APPEND QtWebKit_Private_PRI_ARGUMENTS
            INCLUDE_INSTALL_DIR "${LIB_INSTALL_DIR}/QtWebKit.framework/Headers/${PROJECT_VERSION}"
            INCLUDE_INSTALL_DIR2 "${LIB_INSTALL_DIR}/QtWebKit.framework/Headers/${PROJECT_VERSION}/QtWebKit"
        )
    else ()
        list(APPEND QtWebKit_PRI_ARGUMENTS
            INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR}
            INCLUDE_INSTALL_DIR2 "${KDE_INSTALL_INCLUDEDIR}/QtWebKit"
        )
        list(APPEND QtWebKit_Private_PRI_ARGUMENTS
            INCLUDE_INSTALL_DIR "${KDE_INSTALL_INCLUDEDIR}/QtWebKit/${PROJECT_VERSION}"
            INCLUDE_INSTALL_DIR2 "${KDE_INSTALL_INCLUDEDIR}/QtWebKit/${PROJECT_VERSION}/QtWebKit"
        )
    endif ()
endif ()

list(APPEND QtWebKit_Private_PRI_ARGUMENTS MODULE_CONFIG "internal_module no_link")

if (MACOS_BUILD_FRAMEWORKS)
    set(WebKitLegacy_OUTPUT_NAME QtWebKit)
else ()
    set(WebKitLegacy_OUTPUT_NAME Qt5WebKit)
endif ()

ecm_generate_pri_file(
    BASE_NAME webkit
    NAME QtWebKit
    LIB_NAME ${WebKitLegacy_OUTPUT_NAME}
    INCLUDE_INSTALL_DIR "${KDE_INSTALL_INCLUDEDIR}/QtWebKit"
    DEPS "${WEBKIT_PRI_DEPS}"
    RUNTIME_DEPS "${WEBKIT_PRI_RUNTIME_DEPS}"
    DEFINES QT_WEBKIT_LIB
    QT_MODULES webkit
    EXTRA_LIBS "${WEBKIT_PRI_EXTRA_LIBS}"
    FILENAME_VAR QtWebKit_PRI_FILENAME
    ${QtWebKit_PRI_ARGUMENTS}
)
ecm_generate_pri_file(
    BASE_NAME webkit_private
    NAME "QtWebKit"
    LIB_NAME " "
    DEPS "webkit"
    RUNTIME_DEPS " "
    DEFINES " "
    QT_MODULES webkit
    EXTRA_LIBS " "
    FILENAME_VAR QtWebKit_Private_PRI_FILENAME
    ${QtWebKit_Private_PRI_ARGUMENTS}
)
install(
    FILES ${QtWebKit_PRI_FILENAME} ${QtWebKit_Private_PRI_FILENAME}
    DESTINATION ${ECM_MKSPECS_INSTALL_DIR}
    COMPONENT Data
)

if (QT_STATIC_BUILD)
    set(WebKitLegacy_LIBRARY_TYPE STATIC)
else ()
    set(WebKitLegacy_LIBRARY_TYPE SHARED)
endif ()


############     WebKitWidgets     ############

if (QT_STATIC_BUILD)
    set(WebKitWidgets_LIBRARY_TYPE STATIC)
else ()
    set(WebKitWidgets_LIBRARY_TYPE SHARED)
endif ()

WEBKIT_FRAMEWORK_DECLARE(WebKitWidgets)

set(WebKitWidgets_INCLUDE_DIRECTORIES
    "${CMAKE_BINARY_DIR}/include"
)

set(WebKitWidgets_PRIVATE_INCLUDE_DIRECTORIES
    "${CMAKE_BINARY_DIR}"
    "${WEBKITLEGACY_DIR}/qt/WidgetApi"
    "${WEBKITLEGACY_DIR}/qt/WidgetSupport"
    "${WTF_FRAMEWORK_HEADERS_DIR}"
    "${WebCore_PRIVATE_FRAMEWORK_HEADERS_DIR}"
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
        WebKitLegacy
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

set(QtWebKitWidgets_PUBLIC_FRAMEWORK_HEADERS
    qt/WidgetApi/qgraphicswebview.h
    qt/WidgetApi/qwebframe.h
    qt/WidgetApi/qwebinspector.h
    qt/WidgetApi/qwebpage.h
    qt/WidgetApi/qwebview.h
)

WEBKIT_MAKE_FORWARDING_HEADERS(WebKitWidgets
    TARGET_NAME QtWebKitWidgetsFrameworkHeaders
    DESTINATION ${QtWebKitWidgets_FRAMEWORK_HEADERS_DIR}
    FILES ${QtWebKitWidgets_PUBLIC_FRAMEWORK_HEADERS}
    FLATTENED
)
add_dependencies(QtWebKitWidgetsFrameworkHeaders QtWebKitFrameworkHeaders)

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
    COMMON_HEADER_EXTRAS
        <QtWebKitWidgets/QtWebKitWidgetsDepends>
        \"qtwebkitwidgetsversion.h\"
    COMMON_HEADER_GUARD_NAME
        QT_QTWEBKITWIDGETS_MODULE_H
    RELATIVE
        qt/WidgetApi
    OUTPUT_DIR
        ${QtWebKitWidgets_FRAMEWORK_HEADERS_DIR}
    REQUIRED_HEADERS
        QtWebKitWidgets_HEADERS
)

set(WebKitWidgets_PUBLIC_HEADERS
    ${QtWebKitWidgets_HEADERS}
    ${QtWebKitWidgets_FORWARDING_HEADERS}
)

generate_version_header("${QtWebKitWidgets_FRAMEWORK_HEADERS_DIR}/qtwebkitwidgetsversion.h"
    WebKitWidgets_PUBLIC_HEADERS
    QTWEBKITWIDGETS
)

generate_header("${QtWebKitWidgets_FRAMEWORK_HEADERS_DIR}/QtWebKitWidgetsVersion"
    WebKitWidgets_PUBLIC_HEADERS
    "#include \"qtwebkitwidgetsversion.h\"")

generate_header("${QtWebKitWidgets_FRAMEWORK_HEADERS_DIR}/QtWebKitWidgetsDepends"
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
    COMPONENT Data
)

file(GLOB WebKitWidgets_PRIVATE_HEADERS qt/WidgetApi/*_p.h)
install(
    FILES
        ${WebKitWidgets_PRIVATE_HEADERS}
    DESTINATION
        ${KDE_INSTALL_INCLUDEDIR}/QtWebKitWidgets/${PROJECT_VERSION}/QtWebKitWidgets/private
    COMPONENT Data
)

if (NOT MACOS_BUILD_FRAMEWORKS)
    ecm_generate_pkgconfig_file(
        BASE_NAME Qt5WebKitWidgets
        DESCRIPTION "Qt WebKitWidgets module"
        INCLUDE_INSTALL_DIR "${KDE_INSTALL_INCLUDEDIR}/QtWebKitWidgets"
        DEPS "${WEBKITWIDGETS_PKGCONFIG_DEPS}"
        FILENAME_VAR WebKitWidgets_PKGCONFIG_FILENAME
    )
    install(FILES ${WebKitWidgets_PKGCONFIG_FILENAME} DESTINATION ${ECM_PKGCONFIG_INSTALL_DIR} COMPONENT Data)
endif ()

if (KDE_INSTALL_USE_QT_SYS_PATHS)
    set(WebKitWidgets_PRI_ARGUMENTS
        BIN_INSTALL_DIR "$$QT_MODULE_BIN_BASE"
        LIB_INSTALL_DIR "$$QT_MODULE_LIB_BASE"
    )
    if (MACOS_BUILD_FRAMEWORKS)
        list(APPEND WebKitWidgets_PRI_ARGUMENTS
            INCLUDE_INSTALL_DIR "$$QT_MODULE_LIB_BASE/QtWebKitWidgets.framework/Headers"
            MODULE_CONFIG "lib_bundle"
        )
        list(APPEND WebKitWidgets_Private_PRI_ARGUMENTS
            INCLUDE_INSTALL_DIR "$$QT_MODULE_LIB_BASE/QtWebKitWidgets.framework/Headers/${PROJECT_VERSION}"
            INCLUDE_INSTALL_DIR2 "$$QT_MODULE_LIB_BASE/QtWebKitWidgets.framework/Headers/${PROJECT_VERSION}/QtWebKitWidgets"
        )
    else ()
        list(APPEND WebKitWidgets_PRI_ARGUMENTS
            INCLUDE_INSTALL_DIR "$$QT_MODULE_INCLUDE_BASE"
            INCLUDE_INSTALL_DIR2 "$$QT_MODULE_INCLUDE_BASE/QtWebKitWidgets"
        )
        list(APPEND WebKitWidgets_Private_PRI_ARGUMENTS
            INCLUDE_INSTALL_DIR "$$QT_MODULE_INCLUDE_BASE/QtWebKitWidgets/${PROJECT_VERSION}"
            INCLUDE_INSTALL_DIR2 "$$QT_MODULE_INCLUDE_BASE/QtWebKitWidgets/${PROJECT_VERSION}/QtWebKitWidgets"
        )
    endif ()
else ()
    set(WebKitWidgets_PRI_ARGUMENTS
        SET_RPATH ON
    )
    if (MACOS_BUILD_FRAMEWORKS)
        list(APPEND WebKitWidgets_PRI_ARGUMENTS
            INCLUDE_INSTALL_DIR "${LIB_INSTALL_DIR}/QtWebKitWidgets.framework/Headers"
            MODULE_CONFIG "lib_bundle"
        )
        list(APPEND WebKitWidgets_Private_PRI_ARGUMENTS
            INCLUDE_INSTALL_DIR "${LIB_INSTALL_DIR}/QtWebKitWidgets.framework/Headers/${PROJECT_VERSION}"
            INCLUDE_INSTALL_DIR2 "${LIB_INSTALL_DIR}/QtWebKitWidgets.framework/Headers/${PROJECT_VERSION}/QtWebKitWidgets"
        )
    else ()
        list(APPEND WebKitWidgets_PRI_ARGUMENTS
            INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR}
            INCLUDE_INSTALL_DIR2 "${KDE_INSTALL_INCLUDEDIR}/QtWebKitWidgets"
        )
        list(APPEND WebKitWidgets_Private_PRI_ARGUMENTS
            INCLUDE_INSTALL_DIR "${KDE_INSTALL_INCLUDEDIR}/QtWebKitWidgets/${PROJECT_VERSION}"
            INCLUDE_INSTALL_DIR2 "${KDE_INSTALL_INCLUDEDIR}/QtWebKitWidgets/${PROJECT_VERSION}/QtWebKitWidgets"
        )
    endif ()
endif ()

list(APPEND WebKitWidgets_Private_PRI_ARGUMENTS MODULE_CONFIG "internal_module no_link")

if (MACOS_BUILD_FRAMEWORKS)
    set(WebKitWidgets_OUTPUT_NAME QtWebKitWidgets)
else ()
    set(WebKitWidgets_OUTPUT_NAME Qt5WebKitWidgets)
endif ()

ecm_generate_pri_file(
    BASE_NAME webkitwidgets
    NAME QtWebKitWidgets
    LIB_NAME ${WebKitWidgets_OUTPUT_NAME}
    INCLUDE_INSTALL_DIR "${KDE_INSTALL_INCLUDEDIR}/QtWebKitWidgets"
    DEPS "${WEBKITWIDGETS_PRI_DEPS}"
    RUNTIME_DEPS "${WEBKITWIDGETS_PRI_RUNTIME_DEPS}"
    DEFINES QT_WEBKITWIDGETS_LIB
    QT_MODULES webkitwidgets
    FILENAME_VAR WebKitWidgets_PRI_FILENAME
    ${WebKitWidgets_PRI_ARGUMENTS}
)
ecm_generate_pri_file(
    BASE_NAME webkitwidgets_private
    NAME "QtWebKitWidgets"
    LIB_NAME " "
    DEPS "webkitwidgets"
    RUNTIME_DEPS " "
    DEFINES " "
    QT_MODULES webkitwidgets
    EXTRA_LIBS " "
    FILENAME_VAR WebKitWidgets_Private_PRI_FILENAME
    ${WebKitWidgets_Private_PRI_ARGUMENTS}
)
install(
    FILES ${WebKitWidgets_PRI_FILENAME}  ${WebKitWidgets_Private_PRI_FILENAME}
    DESTINATION ${ECM_MKSPECS_INSTALL_DIR}
    COMPONENT Data
)

if (MSVC)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        enable_language(ASM_MASM)
        list(APPEND WebKitLegacy_SOURCES
            win/Plugins/PaintHooks.asm
        )
    endif ()

    list(APPEND WebKitLegacy_INCLUDE_DIRECTORIES
        ${DERIVED_SOURCES_WEBKIT_DIR}
    )

    WEBKIT_ADD_PRECOMPILED_HEADER("WebKitWidgetsPrefix.h" "qt/WebKitWidgetsPrefix.cpp" WebKitWidgets_SOURCES)
endif ()

set(WebKitWidgets_PRIVATE_HEADERS_LOCATION Headers/${PROJECT_VERSION}/QtWebKitWidgets/private)

WEBKIT_FRAMEWORK(WebKitWidgets)
add_dependencies(WebKitWidgets WebKitLegacy)
set_target_properties(WebKitWidgets PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
install(TARGETS WebKitWidgets EXPORT Qt5WebKitWidgetsTargets
        DESTINATION "${LIB_INSTALL_DIR}"
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}"
)
if (MSVC AND NOT QT_STATIC_BUILD)
    install(FILES $<TARGET_PDB_FILE:WebKitWidgets> DESTINATION "${BIN_INSTALL_DIR}" OPTIONAL)
endif ()

if (NOT MSVC)
    ADD_PREFIX_HEADER(WebKitWidgets "qt/WebKitWidgetsPrefix.h")
endif ()

if (MACOS_BUILD_FRAMEWORKS)
    set_target_properties(WebKitWidgets PROPERTIES
        FRAMEWORK_VERSION ${PROJECT_VERSION_MAJOR}
        SOVERSION ${MACOS_COMPATIBILITY_VERSION}
        MACOSX_FRAMEWORK_IDENTIFIER org.qt-project.QtWebKitWidgets
    )
endif ()

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
        qt/Api/qwebfullscreenrequest.cpp
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

if (ENABLE_WEBKIT)
    add_subdirectory(qt/declarative)
endif ()

if (ENABLE_API_TESTS)
    add_subdirectory(qt/tests)
endif ()

# From PlatformEfl.cmake
add_custom_command(
    OUTPUT ${WebKitLegacy_DERIVED_SOURCES_DIR}/WebKitVersion.h
    MAIN_DEPENDENCY ${WEBKITLEGACY_DIR}/scripts/generate-webkitversion.pl
    DEPENDS ${WEBKITLEGACY_DIR}/mac/Configurations/Version.xcconfig
    COMMAND ${PERL_EXECUTABLE} ${WEBKITLEGACY_DIR}/scripts/generate-webkitversion.pl --config ${WEBKITLEGACY_DIR}/mac/Configurations/Version.xcconfig --outputDir ${WebKitLegacy_DERIVED_SOURCES_DIR}
    VERBATIM)
list(APPEND WebKitLegacy_SOURCES ${WebKitLegacy_DERIVED_SOURCES_DIR}/WebKitVersion.h)

set(common_generator_dependencies
    ${CMAKE_BINARY_DIR}/cmakeconfig.h
)
