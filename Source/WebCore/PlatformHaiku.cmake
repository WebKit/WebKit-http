include(platform/ImageDecoders.cmake)
include(platform/TextureMapper.cmake)

list(APPEND WebCore_INCLUDE_DIRECTORIES
  "${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}"
  "${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/inspector"
  "${JAVASCRIPTCORE_DIR}"
  "${JAVASCRIPTCORE_DIR}/API"
  "${JAVASCRIPTCORE_DIR}/assembler"
  "${JAVASCRIPTCORE_DIR}/bytecode"
  "${JAVASCRIPTCORE_DIR}/dfg"
  "${JAVASCRIPTCORE_DIR}/disassembler"
  "${JAVASCRIPTCORE_DIR}/interpreter"
  "${JAVASCRIPTCORE_DIR}/jit"
  "${JAVASCRIPTCORE_DIR}/llint"
  "${JAVASCRIPTCORE_DIR}/profiler"
  "${JAVASCRIPTCORE_DIR}/runtime"
  "${WEBCORE_DIR}/page/scrolling/coordinatedgraphics"
  "${WEBCORE_DIR}/platform/haiku"
  "${WEBCORE_DIR}/platform/graphics/haiku"
  "${WEBCORE_DIR}/platform/graphics/opentype"
  "${WEBCORE_DIR}/platform/graphics/texmap/coordinated"
  "${WEBCORE_DIR}/platform/network/haiku"
  "${WTF_DIR}"
)

list(APPEND WebCore_SOURCES
  bindings/js/JSSubtleCryptoCustom.cpp
  bindings/js/ScriptControllerHaiku.cpp

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
  crypto/algorithms/CryptoAlgorithmRSASSA_PKCS1_v1_5.cpp
  crypto/algorithms/CryptoAlgorithmRSA_OAEP.cpp
  crypto/algorithms/CryptoAlgorithmSHA1.cpp
  crypto/algorithms/CryptoAlgorithmSHA224.cpp
  crypto/algorithms/CryptoAlgorithmSHA256.cpp
  crypto/algorithms/CryptoAlgorithmSHA384.cpp
  crypto/algorithms/CryptoAlgorithmSHA512.cpp

  crypto/keys/CryptoKeyAES.cpp
  crypto/keys/CryptoKeyDataOctetSequence.cpp
  crypto/keys/CryptoKeyDataRSAComponents.cpp
  crypto/keys/CryptoKeyHMAC.cpp
  crypto/keys/CryptoKeySerializationRaw.cpp

  editing/haiku/EditorHaiku.cpp

  html/shadow/MediaControlsApple.cpp

  platform/Cursor.cpp
  platform/KillRingNone.cpp

  page/haiku/DragControllerHaiku.cpp
  page/haiku/EventHandlerHaiku.cpp

  page/scrolling/coordinatedgraphics/ScrollingCoordinatorCoordinatedGraphics.cpp
  page/scrolling/coordinatedgraphics/ScrollingStateNodeCoordinatedGraphics.cpp

  page/PointerLockController.cpp

  platform/crypto/gnutls/CryptoDigestGnuTLS.cpp

  platform/haiku/CursorHaiku.cpp
  platform/haiku/DragImageHaiku.cpp
  platform/haiku/DragDataHaiku.cpp
  platform/haiku/ErrorsHaiku.cpp
  platform/haiku/EventLoopHaiku.cpp
  platform/haiku/FileSystemHaiku.cpp
  platform/haiku/KeyboardUtilities.cpp
  platform/haiku/KeyedDecoderHaiku.cpp
  platform/haiku/KeyedEncoderHaiku.cpp
  platform/haiku/KURLHaiku.cpp
  platform/haiku/LanguageHaiku.cpp
  platform/haiku/LocalizedStringsHaiku.cpp
  platform/haiku/LoggingHaiku.cpp
  platform/haiku/MIMETypeRegistryHaiku.cpp
  platform/haiku/MainThreadSharedTimerHaiku.cpp
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

  platform/graphics/ImageSource.cpp
  platform/graphics/WOFFFileFormat.cpp

  platform/graphics/haiku/AffineTransformHaiku.cpp
  platform/graphics/haiku/BitmapImageHaiku.cpp
  platform/graphics/haiku/ColorHaiku.cpp
  platform/graphics/haiku/FontCacheHaiku.cpp
  platform/graphics/haiku/FontCustomPlatformData.cpp
  platform/graphics/haiku/FontPlatformDataHaiku.cpp
  platform/graphics/haiku/FloatPointHaiku.cpp
  platform/graphics/haiku/FloatRectHaiku.cpp
  platform/graphics/haiku/FloatSizeHaiku.cpp
  platform/graphics/haiku/FontHaiku.cpp
  platform/graphics/haiku/FontPlatformDataHaiku.cpp
  platform/graphics/haiku/GlyphPageTreeNodeHaiku.cpp
  platform/graphics/haiku/GradientHaiku.cpp
  platform/graphics/haiku/GraphicsContextHaiku.cpp
  platform/graphics/haiku/IconHaiku.cpp
  platform/graphics/haiku/ImageBufferHaiku.cpp
  platform/graphics/haiku/ImageHaiku.cpp
  platform/graphics/haiku/IntPointHaiku.cpp
  platform/graphics/haiku/IntRectHaiku.cpp
  platform/graphics/haiku/IntSizeHaiku.cpp
  platform/graphics/haiku/MediaPlayerPrivateHaiku.cpp
  platform/graphics/haiku/PathHaiku.cpp
  platform/graphics/haiku/SimpleFontDataHaiku.cpp
  platform/graphics/haiku/StillImageHaiku.cpp
  platform/graphics/haiku/TileHaiku.cpp
  platform/graphics/haiku/TiledBackingStoreHaiku.cpp

  platform/graphics/texmap/BitmapTexture.cpp
  platform/graphics/texmap/BitmapTextureImageBuffer.cpp
  platform/graphics/texmap/BitmapTexturePool.cpp
  platform/graphics/texmap/GraphicsLayerTextureMapper.cpp
  platform/graphics/texmap/TextureMapperImageBuffer.cpp

  platform/graphics/texmap/coordinated/CoordinatedGraphicsLayer.cpp
  platform/graphics/texmap/coordinated/CoordinatedImageBacking.cpp
  platform/graphics/texmap/coordinated/CoordinatedSurface.cpp

  platform/image-decoders/ImageDecoder.cpp

  platform/image-decoders/bmp/BMPImageDecoder.cpp
  platform/image-decoders/bmp/BMPImageReader.cpp

  platform/image-decoders/gif/GIFImageDecoder.cpp
  platform/image-decoders/gif/GIFImageReader.cpp

  platform/image-decoders/haiku/ImageDecoderHaiku.cpp

  platform/image-decoders/ico/ICOImageDecoder.cpp

  platform/image-decoders/jpeg/JPEGImageDecoder.cpp

  platform/image-decoders/png/PNGImageDecoder.cpp

  platform/image-decoders/webp/WEBPImageDecoder.cpp

  platform/mock/GeolocationClientMock.cpp

  platform/network/haiku/BUrlProtocolHandler.cpp
  platform/network/haiku/CertificateInfo.cpp
  platform/network/haiku/CookieJarHaiku.cpp
  platform/network/haiku/DNSHaiku.cpp
  platform/network/haiku/ProxyServerHaiku.cpp
  platform/network/haiku/ResourceHandleHaiku.cpp
  platform/network/haiku/ResourceRequestHaiku.cpp

  platform/network/haiku/CredentialStorageHaiku.cpp
  platform/network/haiku/SocketStreamHandleHaiku.cpp
  platform/network/NetworkStorageSessionStub.cpp

  platform/posix/FileSystemPOSIX.cpp

  platform/text/Hyphenation.cpp
  platform/text/LocaleICU.cpp

  platform/text/haiku/TextBreakIteratorInternalICUHaiku.cpp
  platform/text/haiku/StringHaiku.cpp
)

list(APPEND WebCore_USER_AGENT_STYLE_SHEETS
    ${WEBCORE_DIR}/Modules/mediacontrols/mediaControlsBase.css
)

if (WTF_USE_COORDINATED_GRAPHICS)
    list(APPEND WebCore_SOURCES
        platform/graphics/texmap/coordinated/CoordinatedTile.cpp
    )
else()

endif()

set(WebCore_USER_AGENT_SCRIPTS
    ${WEBCORE_DIR}/English.lproj/mediaControlsLocalizedStrings.js
    ${WEBCORE_DIR}/Modules/mediacontrols/mediaControlsBase.js
)

add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/WebKitVersion.h
    MAIN_DEPENDENCY ${WEBKIT_DIR}/scripts/generate-webkitversion.pl
    DEPENDS ${WEBKIT_DIR}/mac/Configurations/Version.xcconfig
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT_DIR}/scripts/generate-webkitversion.pl --config ${WEBKIT_DIR}/mac/Configurations/Version.xcconfig --outputDir ${DERIVED_SOURCES_WEBCORE_DIR}
    VERBATIM)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/WebKitVersion.h)

set(WebCore_USER_AGENT_SCRIPTS_DEPENDENCIES ${WEBCORE_DIR}/platform/haiku/RenderThemeHaiku.cpp)

list(APPEND WebCore_LIBRARIES
  ${GNUTLS_LIBRARIES}
  ${ICU_LIBRARIES}
  ${JPEG_LIBRARY}
  ${LIBXML2_LIBRARIES}
  ${LIBXSLT_LIBRARIES}
  ${PNG_LIBRARY}
  ${SQLITE_LIBRARIES}
  ${WEBP_LIBRARIES}
  ${ZLIB_LIBRARIES}
  be bsd network bnetapi textencoding translation execinfo
)

list(APPEND WebCore_INCLUDE_DIRECTORIES
  ${GNUTLS_INCLUDE_DIRS}
  ${ICU_INCLUDE_DIRS}
  ${LIBXML2_INCLUDE_DIR}
  ${LIBXSLT_INCLUDE_DIR}
  ${SQLITE_INCLUDE_DIR}
  ${WEBP_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIRS}
)

if (ENABLE_WEB_AUDIO)
    #    list(APPEND WebCore_INCLUDE_DIRECTORIES
    #    "${WEBCORE_DIR}/platform/graphics/gstreamer"
    #)

    list(APPEND WebCore_LIBRARIES
        media avcodec
    )
endif ()

if (ENABLE_VIDEO)
    list(APPEND WebCore_LIBRARIES
        media
    )
endif ()

if (WTF_USE_3D_GRAPHICS)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/graphics/opengl"
        "${WEBCORE_DIR}/platform/graphics/surfaces"
        "${WEBCORE_DIR}/platform/graphics/surfaces/glx"
        "${WEBCORE_DIR}/platform/graphics/texmap"
    )

    if (WTF_USE_EGL)
        list(APPEND WebCore_INCLUDE_DIRECTORIES
            ${EGL_INCLUDE_DIR}
            "${WEBCORE_DIR}/platform/graphics/surfaces/egl"
    )
    endif ()

    list(APPEND WebCore_SOURCES
        platform/graphics/opengl/Extensions3DOpenGLCommon.cpp
        platform/graphics/opengl/GLPlatformContext.cpp
        platform/graphics/opengl/GLPlatformSurface.cpp
        platform/graphics/opengl/GraphicsContext3DOpenGLCommon.cpp
        platform/graphics/opengl/TemporaryOpenGLSetting.cpp

        platform/graphics/surfaces/GLTransportSurface.cpp
        platform/graphics/surfaces/GraphicsSurface.cpp
    )

    if (WTF_USE_EGL)
        list(APPEND WebCore_SOURCES
            platform/graphics/surfaces/egl/EGLConfigSelector.cpp
            platform/graphics/surfaces/egl/EGLContext.cpp
            platform/graphics/surfaces/egl/EGLHelper.cpp
            platform/graphics/surfaces/egl/EGLSurface.cpp
            platform/graphics/surfaces/egl/EGLXSurface.cpp
        )
    else ()
        list(APPEND WebCore_SOURCES
            platform/graphics/surfaces/glx/GLXContext.cpp
            platform/graphics/surfaces/glx/GLXSurface.cpp
        )
    endif ()

    if (WTF_USE_OPENGL_ES_2)
        list(APPEND WebCore_SOURCES
            platform/graphics/opengl/Extensions3DOpenGLES.cpp
            platform/graphics/opengl/GraphicsContext3DOpenGLES.cpp
        )
    else ()
        list(APPEND WebCore_SOURCES
            platform/graphics/OpenGLShims.cpp

            platform/graphics/opengl/Extensions3DOpenGL.cpp
            platform/graphics/opengl/GraphicsContext3DOpenGL.cpp
        )
    endif ()

    if (WTF_USE_EGL)
        list(APPEND WebCore_LIBRARIES
            ${EGL_LIBRARY}
        )
    endif ()
endif ()

if (ENABLE_WEB_AUDIO)
    #list(APPEND WebCore_INCLUDE_DIRECTORIES
    #    "${WEBCORE_DIR}/platform/audio/gstreamer"
    #)
    set(WEB_AUDIO_DIR ${CMAKE_INSTALL_PREFIX}/${DATA_INSTALL_DIR}/webaudio/resources)
    file(GLOB WEB_AUDIO_DATA "${WEBCORE_DIR}/platform/audio/resources/*.wav")
    install(FILES ${WEB_AUDIO_DATA} DESTINATION ${WEB_AUDIO_DIR})
    add_definitions(-DUNINSTALLED_AUDIO_RESOURCES_DIR="${WEBCORE_DIR}/platform/audio/resources")
endif ()

# Directory listing template for FTP/file/gopher directory listings.
install(FILES "${WEBCORE_DIR}/platform/haiku/resources/Directory Listing Template.html"
    DESTINATION ${CMAKE_INSTALL_PREFIX}/${DATA_INSTALL_DIR})

if (ENABLE_SPELLCHECK)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        ${ENCHANT_INCLUDE_DIRS}
    )
    list(APPEND WebCore_LIBRARIES
        ${ENCHANT_LIBRARIES}
    )
endif ()

if (ENABLE_ACCESSIBILITY)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/accessibility/atk"
        ${ATK_INCLUDE_DIRS}
    )
    list(APPEND WebCore_LIBRARIES
        ${ATK_LIBRARIES}
    )
endif ()
