LIST(APPEND WebCore_LINK_FLAGS
    libbnetapi be bsd network textencoding translation
)

LIST(APPEND WebCore_INCLUDE_DIRECTORIES
  "${WEBCORE_DIR}/platform/haiku"
  "${WEBCORE_DIR}/platform/graphics/haiku"
  "${WEBCORE_DIR}/platform/network/haiku"
)

LIST(APPEND WebCore_SOURCES
  bindings/js/ScriptControllerHaiku.cpp
  page/haiku/DragControllerHaiku.cpp
  page/haiku/EventHandlerHaiku.cpp
  page/haiku/FrameHaiku.cpp
  platform/text/haiku/TextBreakIteratorInternalICUHaiku.cpp
  platform/text/haiku/StringHaiku.cpp
  platform/haiku/LanguageHaiku.cpp
  platform/graphics/haiku/ColorHaiku.cpp
  platform/graphics/haiku/FontCacheHaiku.cpp
  platform/graphics/haiku/FontCustomPlatformData.cpp
  platform/graphics/haiku/FontPlatformDataHaiku.cpp
  platform/graphics/haiku/FloatPointHaiku.cpp
  platform/graphics/haiku/FloatRectHaiku.cpp
  platform/graphics/haiku/FontHaiku.cpp
  platform/graphics/haiku/GlyphPageTreeNodeHaiku.cpp
  platform/graphics/haiku/GradientHaiku.cpp
  platform/graphics/haiku/GraphicsContextHaiku.cpp
  platform/graphics/haiku/IconHaiku.cpp
  platform/graphics/haiku/ImageHaiku.cpp
  platform/graphics/haiku/IntPointHaiku.cpp
  platform/graphics/haiku/IntRectHaiku.cpp
  platform/graphics/haiku/IntSizeHaiku.cpp
  platform/graphics/haiku/PathHaiku.cpp
  platform/graphics/haiku/SimpleFontDataHaiku.cpp
  platform/graphics/haiku/StillImageHaiku.cpp
  platform/haiku/ClipboardHaiku.cpp
  platform/haiku/ContextMenuHaiku.cpp
  platform/haiku/ContextMenuItemHaiku.cpp
  platform/haiku/CursorHaiku.cpp
  platform/haiku/DragImageHaiku.cpp
  platform/haiku/DragDataHaiku.cpp
  platform/haiku/EventLoopHaiku.cpp
  platform/haiku/FileSystemHaiku.cpp
  platform/haiku/KURLHaiku.cpp
  platform/haiku/LocalizedStringsHaiku.cpp
  platform/haiku/LoggingHaiku.cpp
  platform/haiku/MIMETypeRegistryHaiku.cpp
  platform/haiku/PasteboardHaiku.cpp
  platform/haiku/PlatformKeyboardEventHaiku.cpp
  platform/haiku/PlatformMouseEventHaiku.cpp
  platform/haiku/PlatformScreenHaiku.cpp
  platform/haiku/PlatformWheelEventHaiku.cpp
  platform/haiku/PopupMenuHaiku.cpp
  platform/haiku/RenderThemeHaiku.cpp
  platform/haiku/ScrollbarThemeHaiku.cpp
  platform/haiku/SearchPopupMenuHaiku.cpp
  platform/posix/SharedBufferPOSIX.cpp
  platform/haiku/SharedTimerHaiku.cpp
  platform/haiku/SoundHaiku.cpp
  platform/haiku/TemporaryLinkStubs.cpp
  platform/haiku/WidgetHaiku.cpp
  editing/haiku/EditorHaiku.cpp
  platform/image-decoders/haiku/ImageDecoderHaiku.cpp
  platform/graphics/haiku/ImageBufferHaiku.cpp
  platform/graphics/haiku/FontPlatformDataHaiku.cpp
  platform/network/haiku/BUrlProtocolHandler.cpp
  platform/network/haiku/DNSHaiku.cpp
  platform/network/haiku/ResourceHandleHaiku.cpp
  platform/network/haiku/ResourceRequestHaiku.cpp
  platform/haiku/CookieJarHaiku.cpp # FIXME move to network
  #platform/network/soup/CredentialStorageSoup.cpp
  #platform/network/soup/GOwnPtrSoup.cpp
  #platform/network/soup/ProxyServerSoup.cpp
  #platform/network/soup/ResourceResponseSoup.cpp
  platform/network/curl/SocketStreamHandleCurl.cpp # not implemented
  #platform/network/soup/SoupURIUtils.cpp
  
  platform/graphics/ImageSource.cpp
  platform/Cursor.cpp
  platform/image-decoders/ImageDecoder.cpp
  platform/image-decoders/bmp/BMPImageDecoder.cpp
  platform/image-decoders/bmp/BMPImageReader.cpp
  platform/image-decoders/gif/GIFImageDecoder.cpp
  platform/image-decoders/gif/GIFImageReader.cpp
  platform/image-decoders/ico/ICOImageDecoder.cpp
  platform/image-decoders/jpeg/JPEGImageDecoder.cpp
  platform/image-decoders/png/PNGImageDecoder.cpp
  platform/image-decoders/webp/WEBPImageDecoder.cpp
  platform/PlatformStrategies.cpp
  platform/posix/FileSystemPOSIX.cpp
)

IF (ENABLE_NETSCAPE_PLUGIN_API)
  LIST(APPEND WebCore_SOURCES
    plugins/PluginDatabase.cpp
    plugins/PluginDebug.cpp
    plugins/PluginPackage.cpp
    plugins/PluginStream.cpp
    plugins/PluginView.cpp

    plugins/PluginPackageNone.cpp
    plugins/PluginViewNone.cpp
  )
ELSE ()
  LIST(APPEND WebCore_SOURCES
    plugins/PluginDataNone.cpp
    plugins/PluginPackageNone.cpp
    plugins/PluginViewNone.cpp
  )
ENDIF ()

IF (WTF_USE_ICU_UNICODE)
  LIST(APPEND WebCore_SOURCES
    editing/SmartReplaceICU.cpp
    platform/text/TextEncodingDetectorICU.cpp
    platform/text/TextBreakIteratorICU.cpp
    platform/text/TextCodecICU.cpp
  )
ENDIF ()

LIST(APPEND WebCore_LIBRARIES
  ${ICU_LIBRARIES}
  ${JPEG_LIBRARY}
  ${LIBXML2_LIBRARIES}
  ${LIBXSLT_LIBRARIES}
  ${PNG_LIBRARY}
  ${SQLITE_LIBRARIES}
  ${ZLIB_LIBRARIES}
  be bsd network bnetapi textencoding translation
)

LIST(APPEND WebCore_INCLUDE_DIRECTORIES
  ${ICU_INCLUDE_DIRS}
  ${LIBXML2_INCLUDE_DIR}
  ${LIBXSLT_INCLUDE_DIR}
  ${SQLITE_INCLUDE_DIR}
  ${Glib_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIRS}
  /boot/develop/headers/os/interface
)
