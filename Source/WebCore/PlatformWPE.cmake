list(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/cairo"
    "${WEBCORE_DIR}/platform/geoclue"
    "${WEBCORE_DIR}/platform/graphics/cairo"
    "${WEBCORE_DIR}/platform/graphics/egl"
    "${WEBCORE_DIR}/platform/graphics/glx"
    "${WEBCORE_DIR}/platform/graphics/freetype"
    "${WEBCORE_DIR}/platform/graphics/gstreamer"
    "${WEBCORE_DIR}/platform/graphics/harfbuzz/"
    "${WEBCORE_DIR}/platform/graphics/harfbuzz/ng"
    "${WEBCORE_DIR}/platform/graphics/opengl"
    "${WEBCORE_DIR}/platform/graphics/opentype"
    "${WEBCORE_DIR}/platform/graphics/wayland"
    "${WEBCORE_DIR}/platform/linux"
    "${WEBCORE_DIR}/platform/mediastream/openwebrtc"
    "${WEBCORE_DIR}/platform/mock/mediasource"
    "${WEBCORE_DIR}/platform/network/soup"
    "${WEBCORE_DIR}/platform/text/icu"
)

# WaylandWPE protocol extension.
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/WaylandWPEProtocol.c
    DEPENDS ${WEBCORE_DIR}/platform/graphics/wayland/WaylandWPEProtocol.xml
    COMMAND wayland-scanner server-header < ${WEBCORE_DIR}/platform/graphics/wayland/WaylandWPEProtocol.xml > ${DERIVED_SOURCES_WEBCORE_DIR}/WaylandWPEProtocolServer.h
    COMMAND wayland-scanner client-header < ${WEBCORE_DIR}/platform/graphics/wayland/WaylandWPEProtocol.xml > ${DERIVED_SOURCES_WEBCORE_DIR}/WaylandWPEProtocolClient.h
    COMMAND wayland-scanner code < ${WEBCORE_DIR}/platform/graphics/wayland/WaylandWPEProtocol.xml > ${DERIVED_SOURCES_WEBCORE_DIR}/WaylandWPEProtocol.c
)

list(APPEND WebCore_SOURCES
    loader/soup/CachedRawResourceSoup.cpp
    loader/soup/SubresourceLoaderSoup.cpp
    platform/Cursor.cpp
    platform/PlatformStrategies.cpp
    platform/Theme.cpp
    platform/audio/glib/AudioBusGlib.cpp
    platform/audio/gstreamer/AudioDestinationGStreamer.cpp
    platform/audio/gstreamer/AudioFileReaderGStreamer.cpp
    platform/audio/gstreamer/AudioSourceProviderGStreamer.cpp
    platform/audio/gstreamer/FFTFrameGStreamer.cpp
    platform/audio/gstreamer/WebKitWebAudioSourceGStreamer.cpp
    platform/geoclue/GeolocationProviderGeoclue1.cpp
    platform/geoclue/GeolocationProviderGeoclue2.cpp
    platform/graphics/GraphicsContext3DPrivate.cpp
    platform/graphics/ImageSource.cpp
    platform/graphics/PlatformDisplay.cpp
    platform/graphics/WOFFFileFormat.cpp
    platform/graphics/cairo/BackingStoreBackendCairoImpl.cpp
    platform/graphics/cairo/BitmapImageCairo.cpp
    platform/graphics/cairo/CairoUtilities.cpp
    platform/graphics/cairo/FloatRectCairo.cpp
    platform/graphics/cairo/FontCairo.cpp
    platform/graphics/cairo/FontCairoHarfbuzzNG.cpp
    platform/graphics/cairo/GradientCairo.cpp
    platform/graphics/cairo/GraphicsContext3DCairo.cpp
    platform/graphics/cairo/ImageBufferCairo.cpp
    platform/graphics/cairo/ImageCairo.cpp
    platform/graphics/cairo/IntRectCairo.cpp
    platform/graphics/cairo/PathCairo.cpp
    platform/graphics/cairo/PatternCairo.cpp
    platform/graphics/cairo/PlatformContextCairo.cpp
    platform/graphics/cairo/PlatformPathCairo.cpp
    platform/graphics/cairo/RefPtrCairo.cpp
    platform/graphics/cairo/TransformationMatrixCairo.cpp
    platform/graphics/egl/GLContextEGL.cpp
    platform/graphics/freetype/FontCacheFreeType.cpp
    platform/graphics/freetype/FontCustomPlatformDataFreeType.cpp
    platform/graphics/freetype/GlyphPageTreeNodeFreeType.cpp
    platform/graphics/freetype/SimpleFontDataFreeType.cpp
    platform/graphics/gstreamer/AudioTrackPrivateGStreamer.cpp
    platform/graphics/gstreamer/GRefPtrGStreamer.cpp
    platform/graphics/gstreamer/GStreamerUtilities.cpp
    platform/graphics/gstreamer/ImageGStreamerCairo.cpp
    platform/graphics/gstreamer/InbandTextTrackPrivateGStreamer.cpp
    platform/graphics/gstreamer/MediaPlayerPrivateGStreamer.cpp
    platform/graphics/gstreamer/MediaPlayerPrivateGStreamerBase.cpp
    platform/graphics/gstreamer/MediaSourceGStreamer.cpp
    platform/graphics/gstreamer/SourceBufferPrivateGStreamer.cpp
    platform/graphics/gstreamer/TextCombinerGStreamer.cpp
    platform/graphics/gstreamer/TextSinkGStreamer.cpp
    platform/graphics/gstreamer/TrackPrivateBaseGStreamer.cpp
    platform/graphics/gstreamer/VideoSinkGStreamer.cpp
    platform/graphics/gstreamer/VideoTrackPrivateGStreamer.cpp
    platform/graphics/gstreamer/WebKitMediaSourceGStreamer.cpp
    platform/graphics/gstreamer/WebKitWebSourceGStreamer.cpp
    platform/graphics/harfbuzz/HarfBuzzFace.cpp
    platform/graphics/harfbuzz/HarfBuzzFaceCairo.cpp
    platform/graphics/harfbuzz/HarfBuzzShaper.cpp
    platform/graphics/opengl/Extensions3DOpenGLCommon.cpp
    platform/graphics/opengl/Extensions3DOpenGLES.cpp
    platform/graphics/opengl/GraphicsContext3DOpenGLES.cpp
    platform/graphics/opengl/GraphicsContext3DOpenGLCommon.cpp
    platform/graphics/opengl/TemporaryOpenGLSetting.cpp
    platform/graphics/opentype/OpenTypeVerticalData.cpp
    platform/graphics/wayland/PlatformDisplayWayland.cpp
    platform/graphics/wayland/WaylandSurfaceWPE.cpp
    platform/graphics/GLContext.cpp
    platform/image-decoders/ImageDecoder.cpp
    platform/image-decoders/bmp/BMPImageDecoder.cpp
    platform/image-decoders/bmp/BMPImageReader.cpp
    platform/image-decoders/cairo/ImageDecoderCairo.cpp
    platform/image-decoders/gif/GIFImageDecoder.cpp
    platform/image-decoders/gif/GIFImageReader.cpp
    platform/image-decoders/ico/ICOImageDecoder.cpp
    platform/image-decoders/jpeg/JPEGImageDecoder.cpp
    platform/image-decoders/png/PNGImageDecoder.cpp
    platform/image-decoders/webp/WEBPImageDecoder.cpp
    platform/image-encoders/JPEGImageEncoder.cpp
    platform/linux/MemoryPressureHandlerLinux.cpp
    platform/mediastream/openwebrtc/OpenWebRTCUtilities.cpp
    platform/mediastream/openwebrtc/RealtimeMediaSourceCenterOwr.cpp
    platform/network/soup/AuthenticationChallengeSoup.cpp
    platform/network/soup/CertificateInfo.cpp
    platform/network/soup/CookieJarSoup.cpp
    platform/network/soup/CookieStorageSoup.cpp
    platform/network/soup/CredentialStorageSoup.cpp
    platform/network/soup/DNSSoup.cpp
    platform/network/soup/NetworkStorageSessionSoup.cpp
    platform/network/soup/ProxyServerSoup.cpp
    platform/network/soup/ResourceErrorSoup.cpp
    platform/network/soup/ResourceHandleSoup.cpp
    platform/network/soup/ResourceRequestSoup.cpp
    platform/network/soup/ResourceResponseSoup.cpp
    platform/network/soup/SocketStreamHandleSoup.cpp
    platform/network/soup/SoupNetworkSession.cpp
    platform/network/soup/SynchronousLoaderClientSoup.cpp
    platform/network/soup/WebKitSoupRequestGeneric.cpp
    platform/soup/SharedBufferSoup.cpp
    platform/soup/URLSoup.cpp
    platform/text/icu/UTextProvider.cpp
    platform/text/icu/UTextProviderLatin1.cpp
    platform/text/icu/UTextProviderUTF16.cpp
    platform/text/LocaleICU.cpp
    platform/text/TextCodecICU.cpp
    platform/text/TextEncodingDetectorICU.cpp

    # FIXME: Split into a WebCorePlatform library
    editing/wpe/EditorWPE.cpp
    page/wpe/EventHandlerWPE.cpp
    platform/graphics/cairo/GraphicsContextCairo.cpp
    platform/graphics/freetype/FontPlatformDataFreeType.cpp
    platform/graphics/wpe/IconWPE.cpp
    platform/graphics/wpe/ImageWPE.cpp
    platform/text/wpe/TextBreakIteratorInternalICUWPE.cpp
    platform/wpe/ContextMenuItemWPE.cpp
    platform/wpe/ContextMenuWPE.cpp
    platform/wpe/CursorWPE.cpp
    platform/wpe/EventLoopWPE.cpp
    platform/wpe/ErrorsWPE.cpp
    platform/wpe/FileSystemWPE.cpp
    platform/wpe/LanguageWPE.cpp
    platform/wpe/LoggingWPE.cpp
    platform/wpe/LocalizedStringsWPE.cpp
    platform/wpe/MIMETypeRegistryWPE.cpp
    platform/wpe/PasteboardWPE.cpp
    platform/wpe/PlatformKeyboardEventWPE.cpp
    platform/wpe/PlatformScreenWPE.cpp
    platform/wpe/RenderThemeWPE.cpp
    platform/wpe/SSLKeyGeneratorWPE.cpp
    platform/wpe/ScrollbarThemeWPE.cpp
    platform/wpe/SharedBufferWPE.cpp
    platform/wpe/SharedTimerWPE.cpp
    platform/wpe/SoundWPE.cpp
    platform/wpe/ThemeWPE.cpp
    platform/wpe/WidgetWPE.cpp

    ${DERIVED_SOURCES_WEBCORE_DIR}/WaylandWPEProtocol.c
)

list(APPEND WebCore_LIBRARIES
    ${CAIRO_LIBRARIES}
    ${EGL_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${FREETYPE2_LIBRARIES}
    ${GLIB_GIO_LIBRARIES}
    ${GLIB_GMODULE_LIBRARIES}
    ${GLIB_GOBJECT_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_BASE_LIBRARIES}
    ${GSTREAMER_AUDIO_LIBRARIES}
    ${GSTREAMER_APP_LIBRARIES}
    ${GSTREAMER_GL_LIBRARIES}
    ${GSTREAMER_PBUTILS_LIBRARIES}
    ${GSTREAMER_TAG_LIBRARIES}
    ${GSTREAMER_VIDEO_LIBRARIES}
    ${HARFBUZZ_LIBRARIES}
    ${ICU_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${LIBSOUP_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${PNG_LIBRARIES}
    ${SQLITE_LIBRARIES}
    ${WAYLAND_EGL_LIBRARIES}
    ${WAYLAND_LIBRARIES}
    ${WEBP_LIBRARIES}
)

list(APPEND WebCore_INCLUDE_DIRECTORIES
    ${CAIRO_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${FONTCONFIG_INCLUDE_DIRS}
    ${FREETYPE2_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_BASE_INCLUDE_DIRS}
    ${GSTREAMER_AUDIO_INCLUDE_DIRS}
    ${GSTREAMER_APP_INCLUDE_DIRS}
    ${GSTREAMER_PBUTILS_INCLUDE_DIRS}
    ${GSTREAMER_TAG_INCLUDE_DIRS}
    ${GSTREAMER_VIDEO_INCLUDE_DIRS}
    ${HARFBUZZ_INCLUDE_DIRS}
    ${ICU_INCLUDE_DIRS}
    ${JPEG_INCLUDE_DIRS}
    ${LIBSOUP_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIR}
    ${LIBXSLT_INCLUDE_DIR}
    ${PNG_INCLUDE_DIRS}
    ${SQLITE_INCLUDE_DIR}
    ${WAYLAND_EGL_INCLUDE_DIRS}
    ${WAYLAND_INCLUDE_DIRS}
    ${WEBP_INCLUDE_DIRS}
)

if (USE_TEXTURE_MAPPER)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/graphics/texmap"
    )
    list(APPEND WebCore_SOURCES
        platform/graphics/texmap/BitmapTexture.cpp
        platform/graphics/texmap/BitmapTextureGL.cpp
        platform/graphics/texmap/BitmapTexturePool.cpp
        platform/graphics/texmap/GraphicsLayerTextureMapper.cpp
        platform/graphics/texmap/TextureMapperGL.cpp
        platform/graphics/texmap/TextureMapperShaderProgram.cpp
    )
endif ()

if (ENABLE_THREADED_COMPOSITOR)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/page/scrolling/coordinatedgraphics"
        "${WEBCORE_DIR}/platform/graphics/texmap/coordinated"
        "${WEBCORE_DIR}/platform/graphics/texmap/threadedcompositor"
    )
    list(APPEND WebCore_SOURCES
        page/scrolling/coordinatedgraphics/ScrollingCoordinatorCoordinatedGraphics.cpp
        page/scrolling/coordinatedgraphics/ScrollingStateNodeCoordinatedGraphics.cpp
        page/scrolling/coordinatedgraphics/ScrollingStateScrollingNodeCoordinatedGraphics.cpp
        page/scrolling/ScrollingStateStickyNode.cpp
        page/scrolling/ScrollingThread.cpp
        page/scrolling/ScrollingTreeNode.cpp
        page/scrolling/ScrollingTreeScrollingNode.cpp
        platform/graphics/texmap/TextureMapperPlatformLayerBuffer.cpp
        platform/graphics/texmap/TextureMapperPlatformLayerProxy.cpp
        platform/graphics/texmap/coordinated/AreaAllocator.cpp
        platform/graphics/texmap/coordinated/CompositingCoordinator.cpp
        platform/graphics/texmap/coordinated/CoordinatedGraphicsLayer.cpp
        platform/graphics/texmap/coordinated/CoordinatedImageBacking.cpp
        platform/graphics/texmap/coordinated/CoordinatedSurface.cpp
        platform/graphics/texmap/coordinated/Tile.cpp
        platform/graphics/texmap/coordinated/TiledBackingStore.cpp
        platform/graphics/texmap/coordinated/UpdateAtlas.cpp
    )
endif ()

if (ENABLE_WEB_AUDIO)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        ${WEBCORE_DIR}/platform/audio/gstreamer
        ${GSTREAMER_FFT_INCLUDE_DIRS}
    )
    list(APPEND WebCore_LIBRARIES
        ${GSTREAMER_FFT_LIBRARIES}
    )
endif ()

if (ENABLE_SUBTLE_CRYPTO)
    list(APPEND WebCore_SOURCES
        crypto/CryptoAlgorithm.cpp
        crypto/CryptoAlgorithmDescriptionBuilder.cpp
        crypto/CryptoAlgorithmRegistry.cpp
        crypto/CryptoKey.cpp
        crypto/CryptoKeyPair.cpp
        crypto/SubtleCrypto.cpp
        crypto/algorithms/CryptoAlgorithmAES_CBC.cpp
        crypto/algorithms/CryptoAlgorithmAES_KW.cpp
        crypto/algorithms/CryptoAlgorithmHMAC.cpp
        crypto/algorithms/CryptoAlgorithmRSAES_PKCS1_v1_5.cpp
        crypto/algorithms/CryptoAlgorithmRSA_OAEP.cpp
        crypto/algorithms/CryptoAlgorithmRSASSA_PKCS1_v1_5.cpp
        crypto/algorithms/CryptoAlgorithmSHA1.cpp
        crypto/algorithms/CryptoAlgorithmSHA224.cpp
        crypto/algorithms/CryptoAlgorithmSHA256.cpp
        crypto/algorithms/CryptoAlgorithmSHA384.cpp
        crypto/algorithms/CryptoAlgorithmSHA512.cpp

        crypto/gnutls/CryptoAlgorithmRegistryGnuTLS.cpp
        crypto/gnutls/CryptoAlgorithmAES_CBCGnuTLS.cpp
        crypto/gnutls/CryptoAlgorithmAES_KWGnuTLS.cpp
        crypto/gnutls/CryptoAlgorithmHMACGnuTLS.cpp
        crypto/gnutls/CryptoAlgorithmRSAES_PKCS1_v1_5GnuTLS.cpp
        crypto/gnutls/CryptoAlgorithmRSA_OAEPGnuTLS.cpp
        crypto/gnutls/CryptoAlgorithmRSASSA_PKCS1_v1_5GnuTLS.cpp
        crypto/gnutls/CryptoDigestGnuTLS.cpp
        crypto/gnutls/CryptoKeyRSAGnuTLS.cpp
        crypto/gnutls/SerializedCryptoKeyWrapGnuTLS.cpp

        crypto/keys/CryptoKeyAES.cpp
        crypto/keys/CryptoKeyDataOctetSequence.cpp
        crypto/keys/CryptoKeyDataRSAComponents.cpp
        crypto/keys/CryptoKeyHMAC.cpp
        crypto/keys/CryptoKeySerializationRaw.cpp
    )

    list(APPEND WebCore_INCLUDE_DIRECTORIES
        ${GNUTLS_INCLUDE_DIRS}
    )
    list(APPEND WebCore_LIBRARIES
        ${GNUTLS_LIBRARIES}
    )
endif ()

if (ENABLE_DXDRM)
    list(APPEND WebCore_LIBRARIES
        -lDxDrm
    )
endif ()

if (ENABLE_ENCRYPTED_MEDIA OR ENABLE_ENCRYPTED_MEDIA_V2)
    list(APPEND WebCore_LIBRARIES
        -lcrypto
    )

    list(APPEND WebCore_SOURCES
        platform/graphics/gstreamer/WebKitCommonEncryptionDecryptorGStreamer.cpp
        platform/graphics/gstreamer/WebKitMediaAesCtr.c
    )

    if (ENABLE_ENCRYPTED_MEDIA_V2 AND ENABLE_DXDRM)
        list(APPEND WebCore_SOURCES
            platform/graphics/gstreamer/CDMPRSessionGStreamer.cpp
            platform/graphics/gstreamer/WebKitPlayReadyDecryptorGStreamer.cpp
        )
    endif ()
endif ()
