LIST(APPEND WebCore_INCLUDE_DIRECTORIES
  "${WEBCORE_DIR}/platform/haiku"
  "${WEBCORE_DIR}/platform/graphics/haiku"
  "${WEBCORE_DIR}/platform/network/haiku"
)

LIST(APPEND WebCore_SOURCES
  bindings/js/ScriptControllerHaiku.cpp

  platform/Cursor.cpp

  platform/audio/haiku/AudioBusHaiku.cpp
  platform/audio/haiku/AudioDestinationHaiku.cpp
  platform/audio/haiku/AudioFileReaderHaiku.cpp

  platform/audio/ffmpeg/FFTFrameFFMPEG.cpp

  page/haiku/DragControllerHaiku.cpp
  page/haiku/EventHandlerHaiku.cpp

  platform/haiku/ContextMenuHaiku.cpp
  platform/haiku/ContextMenuItemHaiku.cpp
  platform/haiku/CursorHaiku.cpp
  platform/haiku/DragImageHaiku.cpp
  platform/haiku/DragDataHaiku.cpp
  platform/haiku/EventLoopHaiku.cpp
  platform/haiku/FileSystemHaiku.cpp
  platform/haiku/KURLHaiku.cpp
  platform/haiku/LanguageHaiku.cpp
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

  platform/graphics/WOFFFileFormat.cpp

  platform/graphics/haiku/AffineTransformHaiku.cpp
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
  platform/graphics/haiku/GraphicsLayerHaiku.cpp
  platform/graphics/haiku/IconHaiku.cpp
  platform/graphics/haiku/ImageBufferHaiku.cpp
  platform/graphics/haiku/ImageHaiku.cpp
  platform/graphics/haiku/IntPointHaiku.cpp
  platform/graphics/haiku/IntRectHaiku.cpp
  platform/graphics/haiku/IntSizeHaiku.cpp
  platform/graphics/haiku/PathHaiku.cpp
  platform/graphics/haiku/SimpleFontDataHaiku.cpp
  platform/graphics/haiku/StillImageHaiku.cpp
  
  platform/image-decoders/haiku/ImageDecoderHaiku.cpp

  platform/image-decoders/bmp/BMPImageDecoder.cpp
  platform/image-decoders/bmp/BMPImageReader.cpp
  platform/image-decoders/gif/GIFImageDecoder.cpp
  platform/image-decoders/gif/GIFImageReader.cpp
  platform/image-decoders/ico/ICOImageDecoder.cpp
  platform/image-decoders/jpeg/JPEGImageDecoder.cpp
  platform/image-decoders/png/PNGImageDecoder.cpp
  platform/image-decoders/webp/WEBPImageDecoder.cpp

  platform/network/haiku/BUrlProtocolHandler.cpp
  platform/network/haiku/DNSHaiku.cpp
  platform/network/haiku/ResourceHandleHaiku.cpp
  platform/network/haiku/ResourceRequestHaiku.cpp
  platform/network/haiku/CookieJarHaiku.cpp

  platform/network/curl/SocketStreamHandleCurl.cpp # not implemented
  platform/network/NetworkStorageSessionStub.cpp
  
  platform/posix/FileSystemPOSIX.cpp

  platform/text/haiku/TextBreakIteratorInternalICUHaiku.cpp
  platform/text/haiku/StringHaiku.cpp
  platform/text/LocaleICU.cpp
)

if (WTF_USE_TEXTURE_MAPPER)
    list(APPEND WebCore_SOURCES
        platform/graphics/texmap/GraphicsLayerTextureMapper.cpp
    )
endif ()

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
)

if (ENABLE_VIDEO OR ENABLE_WEB_AUDIO)
    #    list(APPEND WebCore_INCLUDE_DIRECTORIES
    #    "${WEBCORE_DIR}/platform/graphics/gstreamer"
    #)

    list(APPEND WebCore_LIBRARIES
        media avcodec
    )
endif ()

if (ENABLE_VIDEO)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        ${GSTREAMER_VIDEO_INCLUDE_DIRS}
    )
    list(APPEND WebCore_LIBRARIES
        ${GSTREAMER_VIDEO_LIBRARIES}
    )
endif ()

if (WTF_USE_3D_GRAPHICS)
    set(WTF_USE_OPENGL 1)
    add_definitions(-DWTF_USE_OPENGL=1)

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
        platform/graphics/cairo/DrawingBufferCairo.cpp
        platform/graphics/efl/GraphicsContext3DEfl.cpp
        platform/graphics/efl/GraphicsContext3DPrivate.cpp
        platform/graphics/opengl/Extensions3DOpenGLCommon.cpp
        platform/graphics/opengl/GLPlatformContext.cpp
        platform/graphics/opengl/GLPlatformSurface.cpp
        platform/graphics/opengl/GraphicsContext3DOpenGLCommon.cpp
        platform/graphics/surfaces/GraphicsSurface.cpp
        platform/graphics/surfaces/glx/GraphicsSurfaceGLX.cpp
        platform/graphics/surfaces/glx/X11WindowResources.cpp
        platform/graphics/texmap/TextureMapperGL.cpp
        platform/graphics/texmap/TextureMapperShaderProgram.cpp
    )

    if (WTF_USE_EGL)
        list(APPEND WebCore_SOURCES
            platform/graphics/surfaces/egl/EGLConfigSelector.cpp
            platform/graphics/surfaces/egl/EGLContext.cpp
            platform/graphics/surfaces/egl/EGLSurface.cpp
        )
    endif ()

    if (WTF_USE_OPENGL_ES_2)
        list(APPEND WebCore_SOURCES
            platform/graphics/opengl/Extensions3DOpenGLES.cpp
            platform/graphics/opengl/GraphicsContext3DOpenGLES.cpp
        )
    else ()
        list(APPEND WebCore_SOURCES
            platform/graphics/opengl/Extensions3DOpenGL.cpp
            platform/graphics/opengl/GraphicsContext3DOpenGL.cpp
            platform/graphics/OpenGLShims.cpp
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
