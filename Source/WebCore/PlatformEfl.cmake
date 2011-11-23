LIST(APPEND WebCore_LINK_FLAGS
    ${ECORE_X_LDFLAGS}
    ${EFLDEPS_LDFLAGS}
)

LIST(APPEND WebCore_INCLUDE_DIRECTORIES
  "${JAVASCRIPTCORE_DIR}/wtf/gobject"
  "${WEBCORE_DIR}/platform/efl"
  "${WEBCORE_DIR}/platform/text/efl"
  "${WEBCORE_DIR}/platform/graphics/efl"
  "${WEBCORE_DIR}/page/efl"
  "${WEBCORE_DIR}/accessibility/efl"
  "${WEBKIT_DIR}/efl/WebCoreSupport"
  "${WEBKIT_DIR}/efl/ewk"
  "${DERIVED_SOURCES_DIR}"
)

LIST(APPEND WebCore_SOURCES
  accessibility/efl/AccessibilityObjectEfl.cpp
  bindings/js/ScriptControllerEfl.cpp
  page/efl/DragControllerEfl.cpp
  page/efl/EventHandlerEfl.cpp
  platform/Cursor.cpp
  platform/efl/ClipboardEfl.cpp
  platform/efl/ContextMenuEfl.cpp
  platform/efl/ContextMenuItemEfl.cpp
  platform/efl/CursorEfl.cpp
  platform/efl/DragDataEfl.cpp
  platform/efl/DragImageEfl.cpp
  platform/efl/EflKeyboardUtilities.cpp
  platform/efl/EventLoopEfl.cpp
  platform/efl/FileSystemEfl.cpp
  platform/efl/KURLEfl.cpp
  platform/efl/LanguageEfl.cpp
  platform/efl/LocalizedStringsEfl.cpp
  platform/efl/LoggingEfl.cpp
  platform/efl/MIMETypeRegistryEfl.cpp
  platform/efl/PasteboardEfl.cpp
  platform/efl/PlatformKeyboardEventEfl.cpp
  platform/efl/PlatformMouseEventEfl.cpp
  platform/efl/PlatformScreenEfl.cpp
  platform/efl/PlatformTouchEventEfl.cpp
  platform/efl/PlatformTouchPointEfl.cpp
  platform/efl/PlatformWheelEventEfl.cpp
  platform/efl/PopupMenuEfl.cpp
  platform/efl/RenderThemeEfl.cpp
  platform/efl/ScrollViewEfl.cpp
  platform/efl/ScrollbarEfl.cpp
  platform/efl/ScrollbarThemeEfl.cpp
  platform/efl/SearchPopupMenuEfl.cpp
  platform/efl/SharedBufferEfl.cpp
  platform/efl/SharedTimerEfl.cpp
  platform/efl/SoundEfl.cpp
  platform/efl/SystemTimeEfl.cpp
  platform/efl/TemporaryLinkStubs.cpp
  platform/efl/WidgetEfl.cpp
  platform/graphics/ImageSource.cpp
  platform/graphics/efl/GraphicsLayerEfl.cpp
  platform/graphics/efl/IconEfl.cpp
  platform/graphics/efl/ImageEfl.cpp
  platform/graphics/efl/IntPointEfl.cpp
  platform/graphics/efl/IntRectEfl.cpp
  platform/image-decoders/ImageDecoder.cpp
  platform/image-decoders/bmp/BMPImageDecoder.cpp
  platform/image-decoders/bmp/BMPImageReader.cpp
  platform/image-decoders/gif/GIFImageDecoder.cpp
  platform/image-decoders/gif/GIFImageReader.cpp
  platform/image-decoders/ico/ICOImageDecoder.cpp
  platform/image-decoders/jpeg/JPEGImageDecoder.cpp
  platform/image-decoders/png/PNGImageDecoder.cpp
  platform/image-decoders/webp/WEBPImageDecoder.cpp
  platform/posix/FileSystemPOSIX.cpp
  platform/text/efl/TextBreakIteratorInternalICUEfl.cpp
  plugins/PluginDataNone.cpp
  plugins/PluginPackageNone.cpp
  plugins/PluginViewNone.cpp
)

LIST(APPEND WebCore_USER_AGENT_STYLE_SHEETS
    ${WEBCORE_DIR}/css/mediaControlsEfl.css
)

IF (WTF_USE_CAIRO)
  LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/cairo"
    "${WEBCORE_DIR}/platform/graphics/cairo"
  )
  LIST(APPEND WebCore_SOURCES
    platform/cairo/WidgetBackingStoreCairo.cpp
    platform/graphics/cairo/CairoUtilities.cpp
    platform/graphics/cairo/FontCairo.cpp
    platform/graphics/cairo/GradientCairo.cpp
    platform/graphics/cairo/GraphicsContextCairo.cpp
    platform/graphics/cairo/ImageBufferCairo.cpp
    platform/graphics/cairo/ImageCairo.cpp
    platform/graphics/cairo/OwnPtrCairo.cpp
    platform/graphics/cairo/PathCairo.cpp
    platform/graphics/cairo/PatternCairo.cpp
    platform/graphics/cairo/PlatformContextCairo.cpp
    platform/graphics/cairo/PlatformPathCairo.cpp
    platform/graphics/cairo/RefPtrCairo.cpp
    platform/graphics/cairo/TransformationMatrixCairo.cpp

    platform/image-decoders/cairo/ImageDecoderCairo.cpp
  )

  IF (WTF_USE_FREETYPE)
    LIST(APPEND WebCore_INCLUDE_DIRECTORIES
      "${WEBCORE_DIR}/platform/graphics/freetype"
    )
    LIST(APPEND WebCore_SOURCES
      platform/graphics/WOFFFileFormat.cpp
      platform/graphics/efl/FontEfl.cpp
      platform/graphics/freetype/FontCacheFreeType.cpp
      platform/graphics/freetype/FontCustomPlatformDataFreeType.cpp
      platform/graphics/freetype/FontPlatformDataFreeType.cpp
      platform/graphics/freetype/GlyphPageTreeNodeFreeType.cpp
      platform/graphics/freetype/SimpleFontDataFreeType.cpp
    )
    LIST(APPEND WebCore_LIBRARIES
      ${ZLIB_LIBRARIES}
    )
  ENDIF ()

  IF (WTF_USE_PANGO)
    LIST(APPEND WebCore_INCLUDE_DIRECTORIES
      "${WEBCORE_DIR}/platform/graphics/pango"
      ${Pango_INCLUDE_DIRS}
    )
    LIST(APPEND WebCore_SOURCES
      platform/graphics/pango/FontPango.cpp
      platform/graphics/pango/FontCachePango.cpp
      platform/graphics/pango/FontCustomPlatformDataPango.cpp
      platform/graphics/pango/FontPlatformDataPango.cpp
      platform/graphics/pango/GlyphPageTreeNodePango.cpp
      platform/graphics/pango/SimpleFontDataPango.cpp
      platform/graphics/pango/PangoUtilities.cpp
    )
    LIST(APPEND WebCore_LIBRARIES
      ${Pango_LIBRARY}
      ${Pango_Cairo_LIBRARY}
    )
  ENDIF ()
ENDIF ()

IF (WTF_USE_SOUP)
  LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/network/soup"
    "${WEBCORE_DIR}/platform/network/soup/cache"
    "${WEBCORE_DIR}/platform/network/soup/cache/webkit"
  )
  LIST(APPEND WebCore_SOURCES
    platform/network/soup/CookieJarSoup.cpp
    platform/network/soup/CredentialStorageSoup.cpp
    platform/network/soup/GOwnPtrSoup.cpp
    platform/network/soup/ProxyServerSoup.cpp
    platform/network/soup/ResourceHandleSoup.cpp
    platform/network/soup/ResourceRequestSoup.cpp
    platform/network/soup/ResourceResponseSoup.cpp
    platform/network/soup/SocketStreamHandleSoup.cpp
    platform/network/soup/SoupURIUtils.cpp
  )
ENDIF ()

IF (WTF_USE_CURL)
  LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/network/curl"
  )
  LIST(APPEND WebCore_SOURCES
    platform/network/curl/CookieJarCurl.cpp
    platform/network/curl/CredentialStorageCurl.cpp
    platform/network/curl/DNSCurl.cpp
    platform/network/curl/FormDataStreamCurl.cpp
    platform/network/curl/ProxyServerCurl.cpp
    platform/network/curl/ResourceHandleCurl.cpp
    platform/network/curl/ResourceHandleManager.cpp
    platform/network/curl/SocketStreamHandleCurl.cpp
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

IF (ENABLE_GEOLOCATION)
  LIST(APPEND WebCore_SOURCES
    platform/efl/GeolocationServiceEfl.cpp
  )
ENDIF()

IF (ENABLE_VIDEO)
  LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/graphics/gstreamer"
  )
  LIST(APPEND WebCore_SOURCES
    platform/graphics/gstreamer/GRefPtrGStreamer.cpp
    platform/graphics/gstreamer/GStreamerGWorld.cpp
    platform/graphics/gstreamer/ImageGStreamerCairo.cpp
    platform/graphics/gstreamer/MediaPlayerPrivateGStreamer.cpp
    platform/graphics/gstreamer/PlatformVideoWindowEfl.cpp
    platform/graphics/gstreamer/VideoSinkGStreamer.cpp
    platform/graphics/gstreamer/WebKitWebSourceGStreamer.cpp
  )
ENDIF ()

LIST(APPEND WebCore_LIBRARIES
  ${Cairo_LIBRARIES}
  ${ECORE_X_LIBRARIES}
  ${EFLDEPS_LIBRARIES}
  ${EVAS_LIBRARIES}
  ${FREETYPE_LIBRARIES}
  ${ICU_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${LIBXSLT_LIBRARIES}
  ${SQLITE_LIBRARIES}
)

IF (WTF_USE_SOUP)
  LIST(APPEND WebCore_LIBRARIES
    ${LIBSOUP24_LIBRARIES}
  )
ENDIF ()

IF (WTF_USE_CURL)
  LIST(APPEND WebCore_LIBRARIES
    ${CURL_LIBRARIES}
  )
ENDIF ()

IF (ENABLE_VIDEO)
  LIST(APPEND WebCore_LIBRARIES
    ${GStreamer-App_LIBRARIES}
    ${GStreamer-Interfaces_LIBRARIES}
    ${GStreamer-Pbutils_LIBRARIES}
    ${GStreamer-Video_LIBRARIES}
  )
ENDIF ()

LIST(APPEND WebCore_INCLUDE_DIRECTORIES
  ${Cairo_INCLUDE_DIRS}
  ${ECORE_X_INCLUDE_DIRS}
  ${EFLDEPS_INCLUDE_DIRS}
  ${EVAS_INCLUDE_DIRS}
  ${FREETYPE_INCLUDE_DIRS}
  ${ICU_INCLUDE_DIRS}
  ${LIBXML2_INCLUDE_DIR}
  ${LIBXSLT_INCLUDE_DIR}
  ${SQLITE_INCLUDE_DIR}
)

IF (ENABLE_VIDEO)
  LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    ${GStreamer-App_INCLUDE_DIRS}
    ${GStreamer-Interfaces_INCLUDE_DIRS}
    ${GStreamer-Pbutils_INCLUDE_DIRS}
    ${GStreamer-Video_INCLUDE_DIRS}
  )
ENDIF ()


IF (ENABLE_GLIB_SUPPORT)
  LIST(APPEND WebCore_LIBRARIES
    ${Glib_LIBRARIES}
  )
  LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    ${Glib_INCLUDE_DIRS}
  )
ENDIF ()

IF (WTF_USE_SOUP)
  LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    ${LIBSOUP24_INCLUDE_DIRS}
  )
ENDIF ()

IF (WTF_USE_CURL)
  LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    ${CURL_INCLUDE_DIRS}
  )
ENDIF ()

ADD_DEFINITIONS(-DWTF_USE_CROSS_PLATFORM_CONTEXT_MENUS=1
                -DDATA_DIR="${CMAKE_INSTALL_PREFIX}/${DATA_INSTALL_DIR}")
