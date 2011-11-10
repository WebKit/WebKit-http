LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/wince"
    "${WEBCORE_DIR}/platform/win"
    "${WEBCORE_DIR}/platform/text/wince"
    "${WEBCORE_DIR}/platform/graphics/wince"
    "${WEBCORE_DIR}/platform/graphics/win"
    "${WEBCORE_DIR}/platform/network/win"
    "${WEBCORE_DIR}/plugins/win"
    "${WEBCORE_DIR}/page/wince"
    "${WEBCORE_DIR}/page/win"
    "${3RDPARTY_DIR}/libjpeg"
    "${3RDPARTY_DIR}/libpng"
    "${3RDPARTY_DIR}/libxml2/include"
    "${3RDPARTY_DIR}/libxslt/include"
    "${3RDPARTY_DIR}/sqlite"
    "${3RDPARTY_DIR}/zlib"
)

LIST(APPEND WebCore_SOURCES
    bindings/js/ScriptControllerWin.cpp

    page/win/DragControllerWin.cpp
    page/win/EventHandlerWin.cpp
    page/wince/FrameWinCE.cpp

    rendering/RenderThemeWince.cpp

    plugins/PluginDatabase.cpp

    plugins/win/PluginDatabaseWin.cpp

    platform/Cursor.cpp
    platform/LocalizedStrings.cpp
    platform/PlatformStrategies.cpp
    platform/ScrollAnimatorWin.cpp

    platform/graphics/ImageSource.cpp

    platform/image-decoders/ImageDecoder.cpp
    platform/image-decoders/bmp/BMPImageDecoder.cpp
    platform/image-decoders/bmp/BMPImageReader.cpp
    platform/image-decoders/gif/GIFImageDecoder.cpp
    platform/image-decoders/gif/GIFImageReader.cpp
    platform/image-decoders/ico/ICOImageDecoder.cpp
    platform/image-decoders/jpeg/JPEGImageDecoder.cpp
    platform/image-decoders/png/PNGImageDecoder.cpp
    platform/image-decoders/webp/WEBPImageDecoder.cpp

    platform/win/BitmapInfo.cpp
    platform/win/ClipboardUtilitiesWin.cpp
    platform/win/ClipboardWin.cpp
    platform/win/ContextMenuItemWin.cpp
    platform/win/ContextMenuWin.cpp
    platform/win/CursorWin.cpp
    platform/win/EditorWin.cpp
    platform/win/EventLoopWin.cpp
    platform/win/KeyEventWin.cpp
    platform/win/LanguageWin.cpp
    platform/win/LocalizedStringsWin.cpp
    platform/win/PasteboardWin.cpp
    platform/win/PopupMenuWin.cpp
    platform/win/PlatformMouseEventWin.cpp
    platform/win/PlatformScreenWin.cpp
    platform/win/SSLKeyGeneratorWin.cpp
    platform/win/ScrollbarThemeWin.cpp
    platform/win/SearchPopupMenuWin.cpp
    platform/win/SharedBufferWin.cpp
    platform/win/SoundWin.cpp
    platform/win/SystemInfo.cpp
    platform/win/SystemTimeWin.cpp
    platform/win/WCDataObject.cpp
    platform/win/WebCoreInstanceHandle.cpp
    platform/win/WidgetWin.cpp
    platform/win/WheelEventWin.cpp

    platform/wince/DragDataWince.cpp
    platform/wince/DragImageWince.cpp
    platform/wince/FileSystemWince.cpp
    platform/wince/KURLWince.cpp
    platform/wince/MIMETypeRegistryWince.cpp
    platform/wince/SharedTimerWince.cpp

    platform/network/win/CredentialStorageWin.cpp
    platform/network/win/CookieJarWin.cpp
    platform/network/win/CookieStorageWin.cpp
    platform/network/win/NetworkStateNotifierWin.cpp
    platform/network/win/ProxyServerWin.cpp
    platform/network/win/ResourceHandleWin.cpp
    platform/network/win/SocketStreamHandleWin.cpp

    platform/graphics/opentype/OpenTypeUtilities.cpp

    platform/graphics/win/GDIExtras.cpp
    platform/graphics/win/IconWin.cpp
    platform/graphics/win/ImageWin.cpp
    platform/graphics/win/IntPointWin.cpp
    platform/graphics/win/IntRectWin.cpp
    platform/graphics/win/IntSizeWin.cpp

    platform/graphics/wince/FontCacheWince.cpp
    platform/graphics/wince/FontCustomPlatformData.cpp
    platform/graphics/wince/FontPlatformData.cpp
    platform/graphics/wince/FontWince.cpp
    platform/graphics/wince/GlyphPageTreeNodeWince.cpp
    platform/graphics/wince/GradientWince.cpp
    platform/graphics/wince/GraphicsContextWince.cpp
    platform/graphics/wince/ImageBufferWince.cpp
    platform/graphics/wince/ImageWinCE.cpp
    platform/graphics/wince/PathWince.cpp
    platform/graphics/wince/PlatformPathWince.cpp
    platform/graphics/wince/SharedBitmap.cpp
    platform/graphics/wince/SimpleFontDataWince.cpp

    platform/text/TextEncodingDetectorNone.cpp

    platform/text/wince/TextBreakIteratorWince.cpp
    platform/text/wince/TextCodecWinCE.cpp
)

LIST(APPEND WebCore_LIBRARIES
    libjpeg
    libpng
    libxml2
    libxslt
    sqlite
    crypt32
    iphlpapi
    wininet
)


IF (ENABLE_NETSCAPE_PLUGIN_API)
    LIST(APPEND WebCore_SOURCES
        plugins/win/PluginMessageThrottlerWin.cpp
        plugins/win/PluginPackageWin.cpp
        plugins/win/PluginViewWin.cpp
        plugins/PluginPackage.cpp
        plugins/PluginView.cpp
    )
ELSE ()
    LIST(APPEND WebCore_SOURCES
        plugins/PluginPackage.cpp
        plugins/PluginPackageNone.cpp
        plugins/PluginViewNone.cpp
    )
ENDIF ()
