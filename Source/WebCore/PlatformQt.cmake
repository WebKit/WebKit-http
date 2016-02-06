list(APPEND WebCore_INCLUDE_DIRECTORIES
    "${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}"
    "${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/inspector"
    "${JAVASCRIPTCORE_DIR}"
    "${JAVASCRIPTCORE_DIR}/ForwardingHeaders"
    "${JAVASCRIPTCORE_DIR}/API"
    "${JAVASCRIPTCORE_DIR}/assembler"
    "${JAVASCRIPTCORE_DIR}/bytecode"
    "${JAVASCRIPTCORE_DIR}/bytecompiler"
    "${JAVASCRIPTCORE_DIR}/dfg"
    "${JAVASCRIPTCORE_DIR}/disassembler"
    "${JAVASCRIPTCORE_DIR}/heap"
    "${JAVASCRIPTCORE_DIR}/debugger"
    "${JAVASCRIPTCORE_DIR}/interpreter"
    "${JAVASCRIPTCORE_DIR}/jit"
    "${JAVASCRIPTCORE_DIR}/llint"
    "${JAVASCRIPTCORE_DIR}/parser"
    "${JAVASCRIPTCORE_DIR}/profiler"
    "${JAVASCRIPTCORE_DIR}/runtime"
    "${JAVASCRIPTCORE_DIR}/yarr"
    "${THIRDPARTY_DIR}/ANGLE/"
    "${THIRDPARTY_DIR}/ANGLE/include/KHR"
    "${WEBCORE_DIR}/bridge/qt"
    "${WEBCORE_DIR}/history/qt"
    "${WEBCORE_DIR}/platform/qt"
    "${WEBCORE_DIR}/platform/audio/qt"
    "${WEBCORE_DIR}/platform/graphics/qt"
    "${WEBCORE_DIR}/platform/graphics/gpu/qt"
    "${WEBCORE_DIR}/platform/graphics/surfaces/qt"
    "${WEBCORE_DIR}/platform/network/qt"
    "${WEBCORE_DIR}/platform/text/qt"
    "${WTF_DIR}"
)

list(APPEND WebCore_SOURCES
    accessibility/qt/AccessibilityObjectQt.cpp

    bridge/qt/qt_class.cpp
    bridge/qt/qt_instance.cpp
    bridge/qt/qt_pixmapruntime.cpp
    bridge/qt/qt_runtime.cpp

    editing/qt/EditorQt.cpp

    history/qt/HistoryItemQt.cpp

    page/qt/DragControllerQt.cpp
    page/qt/EventHandlerQt.cpp

    platform/audio/qt/AudioBusQt.cpp

    platform/graphics/gpu/qt/DrawingBufferQt.cpp

    platform/graphics/qt/ColorQt.cpp
    platform/graphics/qt/FloatPointQt.cpp
    platform/graphics/qt/FloatRectQt.cpp
    platform/graphics/qt/FloatSizeQt.cpp
    platform/graphics/qt/FontCacheQt.cpp
    platform/graphics/qt/FontCascadeQt.cpp
    platform/graphics/qt/FontCustomPlatformDataQt.cpp
    platform/graphics/qt/FontPlatformDataQt.cpp
    platform/graphics/qt/FontQt.cpp
    platform/graphics/qt/GlyphPageTreeNodeQt.cpp
    platform/graphics/qt/GradientQt.cpp
#    platform/graphics/qt/GraphicsContext3DQt.cpp
    platform/graphics/qt/GraphicsContextQt.cpp
    platform/graphics/qt/IconQt.cpp
    platform/graphics/qt/ImageBufferQt.cpp
    platform/graphics/qt/ImageDecoderQt.cpp
    platform/graphics/qt/ImageQt.cpp
    platform/graphics/qt/IntPointQt.cpp
    platform/graphics/qt/IntRectQt.cpp
    platform/graphics/qt/IntSizeQt.cpp
    platform/graphics/qt/PathQt.cpp
    platform/graphics/qt/PatternQt.cpp
    platform/graphics/qt/StillImageQt.cpp
    platform/graphics/qt/TileQt.cpp
    platform/graphics/qt/TransformationMatrixQt.cpp

#    platform/graphics/surfaces/qt/GraphicsSurfaceQt.cpp

    platform/network/qt/CookieJarQt.cpp
    platform/network/qt/CredentialStorageQt.cpp
    platform/network/qt/DNSQt.cpp
    platform/network/qt/NetworkStateNotifierQt.cpp
    platform/network/qt/ProxyServerQt.cpp
    platform/network/qt/QNetworkReplyHandler.cpp
    platform/network/qt/QtMIMETypeSniffer.cpp
    platform/network/qt/ResourceHandleQt.cpp
    platform/network/qt/ResourceRequestQt.cpp
    platform/network/qt/ResourceResponseQt.cpp
    platform/network/qt/SocketStreamHandleQt.cpp

    platform/qt/CursorQt.cpp
    platform/qt/DataTransferItemListQt.cpp
    platform/qt/DataTransferItemQt.cpp
    platform/qt/DragDataQt.cpp
    platform/qt/DragImageQt.cpp
    platform/qt/EventLoopQt.cpp
    platform/qt/FileSystemQt.cpp
    platform/qt/LanguageQt.cpp
    platform/qt/LocalizedStringsQt.cpp
    platform/qt/LoggingQt.cpp
    platform/qt/MIMETypeRegistryQt.cpp
    platform/qt/PasteboardQt.cpp
    platform/qt/PlatformKeyboardEventQt.cpp
    platform/qt/PlatformScreenQt.cpp
    platform/qt/QtTestSupport.cpp
    platform/qt/RenderThemeQStyle.cpp
    platform/qt/RenderThemeQt.cpp
    platform/qt/RenderThemeQtMobile.cpp
    platform/qt/ScrollViewQt.cpp
    platform/qt/ScrollbarThemeQStyle.cpp
    platform/qt/ScrollbarThemeQt.cpp
    platform/qt/SharedBufferQt.cpp
    platform/qt/SharedTimerQt.cpp
    platform/qt/SoundQt.cpp
    platform/qt/TemporaryLinkStubsQt.cpp
    platform/qt/ThirdPartyCookiesQt.cpp
    platform/qt/URLQt.cpp
    platform/qt/UserAgentQt.cpp
    platform/qt/WidgetQt.cpp

    platform/text/qt/TextBoundariesQt.cpp
    platform/text/qt/TextBreakIteratorInternalICUQt.cpp
)


if (ENABLE_DEVICE_ORIENTATION)
    list(APPEND WebCore_SOURCES
        platform/qt/DeviceMotionClientQt.cpp
        platform/qt/DeviceMotionProviderQt.cpp
        platform/qt/DeviceOrientationClientQt.cpp
        platform/qt/DeviceOrientationProviderQt.cpp
    )
endif ()

if (ENABLE_GAMEPAD)
    list(APPEND WebCore_SOURCES
        platform/qt/GamepadsQt.cpp
    )
endif ()

if (USE_QT_MULTIMEDIA)
    list(APPEND WebCore_SOURCES
        platform/graphics/qt/MediaPlayerPrivateQt.cpp
    )
endif ()

# Note: Qt5Network_INCLUDE_DIRS includes Qt5Core_INCLUDE_DIRS
list(APPEND WebCore_SYSTEM_INCLUDE_DIRECTORIES
    ${LIBXML2_INCLUDE_DIR}
    ${LIBXSLT_INCLUDE_DIR}
    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
    ${Qt5Sql_INCLUDE_DIRS}
    ${SQLITE_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)
# Build the include path with duplicates removed
list(REMOVE_DUPLICATES WebCore_SYSTEM_INCLUDE_DIRECTORIES)

list(APPEND WebCore_LIBRARIES
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${Qt5Core_LIBRARIES}
    ${Qt5Gui_LIBRARIES}
    ${Qt5Network_LIBRARIES}
    ${Qt5Sql_LIBRARIES}
    ${SQLITE_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

list(APPEND WebCore_USER_AGENT_STYLE_SHEETS
    ${WEBCORE_DIR}/css/mediaControlsGtk.css
    ${WEBCORE_DIR}/css/mediaControlsQt.css
    ${WEBCORE_DIR}/css/mediaControlsQtFullscreen.css
    ${WEBCORE_DIR}/css/mobileThemeQt.css
    ${WEBCORE_DIR}/css/themeQtNoListboxes.css
)

# From PlatformWin.cmake
if (WIN32)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${CMAKE_BINARY_DIR}/../include/private"
        "${CMAKE_BINARY_DIR}/../include/private/JavaScriptCore"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/ANGLE"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/ANGLE/include/KHR"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore/ForwardingHeaders"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore/API"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore/assembler"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore/builtins"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore/bytecode"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore/bytecompiler"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore/dfg"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore/disassembler"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore/heap"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore/debugger"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore/interpreter"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore/jit"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore/llint"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore/parser"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore/profiler"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore/runtime"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore/yarr"
        "${DERIVED_SOURCES_DIR}/ForwardingHeaders/WTF"
        "${WEBCORE_DIR}/ForwardingHeaders"
        "${WEBCORE_DIR}/platform/win"
    )

    file(MAKE_DIRECTORY ${DERIVED_SOURCES_DIR}/ForwardingHeaders/WebCore)

    set(WebCore_PRE_BUILD_COMMAND "${CMAKE_BINARY_DIR}/DerivedSources/WebCore/preBuild.cmd")
    file(WRITE "${WebCore_PRE_BUILD_COMMAND}" "@xcopy /y /s /d /f \"${WEBCORE_DIR}/ForwardingHeaders/*.h\" \"${DERIVED_SOURCES_DIR}/ForwardingHeaders/WebCore\" >nul 2>nul\n")
    foreach (_directory ${WebCore_FORWARDING_HEADERS_DIRECTORIES})
        file(APPEND "${WebCore_PRE_BUILD_COMMAND}" "@xcopy /y /d /f \"${WEBCORE_DIR}/${_directory}/*.h\" \"${DERIVED_SOURCES_DIR}/ForwardingHeaders/WebCore\" >nul 2>nul\n")
    endforeach ()

    set(WebCore_POST_BUILD_COMMAND "${CMAKE_BINARY_DIR}/DerivedSources/WebCore/postBuild.cmd")
    file(WRITE "${WebCore_POST_BUILD_COMMAND}" "@xcopy /y /s /d /f \"${DERIVED_SOURCES_WEBCORE_DIR}/*.h\" \"${DERIVED_SOURCES_DIR}/ForwardingHeaders/WebCore\" >nul 2>nul\n")
endif ()

# From PlatformEfl.cmake
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/WebKitVersion.h
    MAIN_DEPENDENCY ${WEBKIT_DIR}/scripts/generate-webkitversion.pl
    DEPENDS ${WEBKIT_DIR}/mac/Configurations/Version.xcconfig
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT_DIR}/scripts/generate-webkitversion.pl --config ${WEBKIT_DIR}/mac/Configurations/Version.xcconfig --outputDir ${DERIVED_SOURCES_WEBCORE_DIR}
    VERBATIM)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/WebKitVersion.h)
