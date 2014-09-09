list(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/editing/atk"
    "${WEBCORE_DIR}/page/efl"
    "${WEBCORE_DIR}/page/scrolling/coordinatedgraphics"
    "${WEBCORE_DIR}/platform/cairo"
    "${WEBCORE_DIR}/platform/efl"
    "${WEBCORE_DIR}/platform/geoclue"
    "${WEBCORE_DIR}/platform/graphics/cairo"
    "${WEBCORE_DIR}/platform/graphics/efl"
    "${WEBCORE_DIR}/platform/graphics/freetype"
    "${WEBCORE_DIR}/platform/graphics/harfbuzz/"
    "${WEBCORE_DIR}/platform/graphics/harfbuzz/ng"
    "${WEBCORE_DIR}/platform/graphics/opentype"
    "${WEBCORE_DIR}/platform/graphics/texmap/coordinated"
    "${WEBCORE_DIR}/platform/linux"
    "${WEBCORE_DIR}/platform/mediastream/gstreamer"
    "${WEBCORE_DIR}/platform/mock/mediasource"
    "${WEBCORE_DIR}/platform/network/soup"
    "${WEBCORE_DIR}/platform/text/efl"
    "${WEBCORE_DIR}/plugins/efl"
)

list(APPEND WebCore_SOURCES
    accessibility/atk/AXObjectCacheAtk.cpp
    accessibility/atk/AccessibilityObjectAtk.cpp
    accessibility/atk/WebKitAccessibleHyperlink.cpp
    accessibility/atk/WebKitAccessibleInterfaceAction.cpp
    accessibility/atk/WebKitAccessibleInterfaceComponent.cpp
    accessibility/atk/WebKitAccessibleInterfaceDocument.cpp
    accessibility/atk/WebKitAccessibleInterfaceEditableText.cpp
    accessibility/atk/WebKitAccessibleInterfaceHyperlinkImpl.cpp
    accessibility/atk/WebKitAccessibleInterfaceHypertext.cpp
    accessibility/atk/WebKitAccessibleInterfaceImage.cpp
    accessibility/atk/WebKitAccessibleInterfaceSelection.cpp
    accessibility/atk/WebKitAccessibleInterfaceTable.cpp
    accessibility/atk/WebKitAccessibleInterfaceTableCell.cpp
    accessibility/atk/WebKitAccessibleInterfaceText.cpp
    accessibility/atk/WebKitAccessibleInterfaceValue.cpp
    accessibility/atk/WebKitAccessibleUtil.cpp
    accessibility/atk/WebKitAccessibleWrapperAtk.cpp

    editing/SmartReplace.cpp

    editing/atk/FrameSelectionAtk.cpp

    editing/efl/EditorEfl.cpp

    html/shadow/MediaControlsApple.cpp

    loader/soup/CachedRawResourceSoup.cpp
    loader/soup/SubresourceLoaderSoup.cpp

    page/efl/DragControllerEfl.cpp
    page/efl/EventHandlerEfl.cpp

    page/scrolling/coordinatedgraphics/ScrollingCoordinatorCoordinatedGraphics.cpp
    page/scrolling/coordinatedgraphics/ScrollingStateNodeCoordinatedGraphics.cpp
    page/scrolling/coordinatedgraphics/ScrollingStateScrollingNodeCoordinatedGraphics.cpp

    platform/ContextMenuItemNone.cpp
    platform/ContextMenuNone.cpp
    platform/Cursor.cpp

    platform/audio/efl/AudioBusEfl.cpp

    platform/audio/gstreamer/AudioDestinationGStreamer.cpp
    platform/audio/gstreamer/AudioFileReaderGStreamer.cpp
    platform/audio/gstreamer/FFTFrameGStreamer.cpp
    platform/audio/gstreamer/WebKitWebAudioSourceGStreamer.cpp

    platform/cairo/WidgetBackingStoreCairo.cpp

    platform/efl/BatteryProviderEfl.cpp
    platform/efl/CursorEfl.cpp
    platform/efl/DragDataEfl.cpp
    platform/efl/DragImageEfl.cpp
    platform/efl/EflInspectorUtilities.cpp
    platform/efl/EflKeyboardUtilities.cpp
    platform/efl/EflScreenUtilities.cpp
    platform/efl/ErrorsEfl.cpp
    platform/efl/EventLoopEfl.cpp
    platform/efl/FileSystemEfl.cpp
    platform/efl/GamepadsEfl.cpp
    platform/efl/LanguageEfl.cpp
    platform/efl/LocalizedStringsEfl.cpp
    platform/efl/LoggingEfl.cpp
    platform/efl/MIMETypeRegistryEfl.cpp
    platform/efl/PasteboardEfl.cpp
    platform/efl/PlatformKeyboardEventEfl.cpp
    platform/efl/PlatformMouseEventEfl.cpp
    platform/efl/PlatformScreenEfl.cpp
    platform/efl/PlatformWheelEventEfl.cpp
    platform/efl/RenderThemeEfl.cpp
    platform/efl/ScrollbarEfl.cpp
    platform/efl/ScrollbarThemeEfl.cpp
    platform/efl/SharedTimerEfl.cpp
    platform/efl/SoundEfl.cpp
    platform/efl/TemporaryLinkStubs.cpp
    platform/efl/WidgetEfl.cpp

    platform/geoclue/GeolocationProviderGeoclue1.cpp
    platform/geoclue/GeolocationProviderGeoclue2.cpp

    platform/graphics/WOFFFileFormat.cpp

    platform/graphics/cairo/BitmapImageCairo.cpp
    platform/graphics/cairo/CairoUtilities.cpp
    platform/graphics/cairo/FontCairo.cpp
    platform/graphics/cairo/FontCairoHarfbuzzNG.cpp
    platform/graphics/cairo/GradientCairo.cpp
    platform/graphics/cairo/GraphicsContextCairo.cpp
    platform/graphics/cairo/ImageBufferCairo.cpp
    platform/graphics/cairo/ImageCairo.cpp
    platform/graphics/cairo/IntRectCairo.cpp
    platform/graphics/cairo/OwnPtrCairo.cpp
    platform/graphics/cairo/PathCairo.cpp
    platform/graphics/cairo/PatternCairo.cpp
    platform/graphics/cairo/PlatformContextCairo.cpp
    platform/graphics/cairo/PlatformPathCairo.cpp
    platform/graphics/cairo/RefPtrCairo.cpp
    platform/graphics/cairo/TransformationMatrixCairo.cpp

    platform/graphics/efl/CairoUtilitiesEfl.cpp
    platform/graphics/efl/IconEfl.cpp
    platform/graphics/efl/ImageEfl.cpp
    platform/graphics/efl/IntPointEfl.cpp
    platform/graphics/efl/IntRectEfl.cpp

    platform/graphics/freetype/FontCacheFreeType.cpp
    platform/graphics/freetype/FontCustomPlatformDataFreeType.cpp
    platform/graphics/freetype/FontPlatformDataFreeType.cpp
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

    platform/graphics/opentype/OpenTypeVerticalData.cpp

    platform/graphics/texmap/coordinated/AreaAllocator.cpp
    platform/graphics/texmap/coordinated/CompositingCoordinator.cpp
    platform/graphics/texmap/coordinated/CoordinatedBackingStore.cpp
    platform/graphics/texmap/coordinated/CoordinatedGraphicsLayer.cpp
    platform/graphics/texmap/coordinated/CoordinatedGraphicsScene.cpp
    platform/graphics/texmap/coordinated/CoordinatedImageBacking.cpp
    platform/graphics/texmap/coordinated/CoordinatedSurface.cpp
    platform/graphics/texmap/coordinated/CoordinatedTile.cpp
    platform/graphics/texmap/coordinated/UpdateAtlas.cpp

    platform/image-decoders/cairo/ImageDecoderCairo.cpp

    platform/linux/GamepadDeviceLinux.cpp

    platform/mediastream/gstreamer/MediaStreamCenterGStreamer.cpp

    platform/network/efl/NetworkStateNotifierEfl.cpp

    platform/network/soup/AuthenticationChallengeSoup.cpp
    platform/network/soup/CertificateInfo.cpp
    platform/network/soup/CookieJarSoup.cpp
    platform/network/soup/CookieStorageSoup.cpp
    platform/network/soup/CredentialStorageSoup.cpp
    platform/network/soup/DNSSoup.cpp
    platform/network/soup/NetworkStorageSessionSoup.cpp
    platform/network/soup/ProxyResolverSoup.cpp
    platform/network/soup/ProxyServerSoup.cpp
    platform/network/soup/ResourceErrorSoup.cpp
    platform/network/soup/ResourceHandleSoup.cpp
    platform/network/soup/ResourceRequestSoup.cpp
    platform/network/soup/ResourceResponseSoup.cpp
    platform/network/soup/SocketStreamHandleSoup.cpp
    platform/network/soup/SoupNetworkSession.cpp
    platform/network/soup/SynchronousLoaderClientSoup.cpp

    platform/posix/FileSystemPOSIX.cpp
    platform/posix/SharedBufferPOSIX.cpp

    platform/soup/SharedBufferSoup.cpp
    platform/soup/URLSoup.cpp

    platform/text/LocaleICU.cpp

    platform/text/efl/TextBreakIteratorInternalICUEfl.cpp

    platform/text/enchant/TextCheckerEnchant.cpp

    plugins/PluginPackageNone.cpp
    plugins/PluginViewNone.cpp
)

if (WTF_USE_GEOCLUE2)
    list(APPEND WebCore_SOURCES
        ${DERIVED_SOURCES_WEBCORE_DIR}/Geoclue2Interface.c
    )
    execute_process(COMMAND pkg-config --variable dbus_interface geoclue-2.0 OUTPUT_VARIABLE GEOCLUE_DBUS_INTERFACE)
    add_custom_command(
         OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/Geoclue2Interface.c ${DERIVED_SOURCES_WEBCORE_DIR}/Geoclue2Interface.h
         COMMAND gdbus-codegen --interface-prefix org.freedesktop.GeoClue2. --c-namespace Geoclue --generate-c-code ${DERIVED_SOURCES_WEBCORE_DIR}/Geoclue2Interface ${GEOCLUE_DBUS_INTERFACE}
    )
endif ()

if (ENABLE_BATTERY_STATUS OR (EFL_REQUIRED_VERSION VERSION_LESS 1.8))
    list(APPEND WebCore_INCLUDE_DIRECTORIES ${DBUS_INCLUDE_DIRS})
    list(APPEND WebCore_INCLUDE_DIRECTORIES ${E_DBUS_INCLUDE_DIRS})
    list(APPEND WebCore_INCLUDE_DIRECTORIES ${E_DBUS_EUKIT_INCLUDE_DIRS})
    list(APPEND WebCore_LIBRARIES ${DBUS_LIBRARIES})
    list(APPEND WebCore_LIBRARIES ${E_DBUS_LIBRARIES})
    list(APPEND WebCore_LIBRARIES ${E_DBUS_EUKIT_LIBRARIES})
endif ()

list(APPEND WebCore_USER_AGENT_STYLE_SHEETS
    ${WEBCORE_DIR}/Modules/mediacontrols/mediaControlsApple.css
)

if (WTF_USE_TEXTURE_MAPPER)
    list(APPEND WebCore_SOURCES
        platform/graphics/texmap/GraphicsLayerTextureMapper.cpp
    )
endif ()

set(WebCore_USER_AGENT_SCRIPTS
    ${WEBCORE_DIR}/English.lproj/mediaControlsLocalizedStrings.js
    ${WEBCORE_DIR}/Modules/mediacontrols/mediaControlsApple.js
)

set(WebCore_USER_AGENT_SCRIPTS_DEPENDENCIES ${WEBCORE_DIR}/platform/efl/RenderThemeEfl.cpp)

list(APPEND WebCore_LIBRARIES
    ${CAIRO_LIBRARIES}
    ${ECORE_EVAS_LIBRARIES}
    ${ECORE_FILE_LIBRARIES}
    ${ECORE_LIBRARIES}
    ${ECORE_X_LIBRARIES}
    ${EDJE_LIBRARIES}
    ${EEZE_LIBRARIES}
    ${EINA_LIBRARIES}
    ${EO_LIBRARIES}
    ${EVAS_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${FREETYPE2_LIBRARIES}
    ${GEOCLUE_LIBRARIES}
    ${GLIB_GIO_LIBRARIES}
    ${GLIB_GOBJECT_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${HARFBUZZ_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${LIBSOUP_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${PNG_LIBRARIES}
    ${SQLITE_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

list(APPEND WebCore_INCLUDE_DIRECTORIES
    ${CAIRO_INCLUDE_DIRS}
    ${ECORE_INCLUDE_DIRS}
    ${ECORE_EVAS_INCLUDE_DIRS}
    ${ECORE_FILE_INCLUDE_DIRS}
    ${ECORE_X_INCLUDE_DIRS}
    ${EO_INCLUDE_DIRS}
    ${EDJE_INCLUDE_DIRS}
    ${EEZE_INCLUDE_DIRS}
    ${EINA_INCLUDE_DIRS}
    ${EVAS_INCLUDE_DIRS}
    ${FREETYPE2_INCLUDE_DIRS}
    ${GEOCLUE_INCLUDE_DIRS}
    ${GIO_UNIX_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIR}
    ${LIBXSLT_INCLUDE_DIR}
    ${SQLITE_INCLUDE_DIR}
    ${GLIB_INCLUDE_DIRS}
    ${LIBSOUP_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    ${HARFBUZZ_INCLUDE_DIRS}
)

if (ENABLE_VIDEO OR ENABLE_WEB_AUDIO)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/graphics/gstreamer"

        ${GSTREAMER_INCLUDE_DIRS}
        ${GSTREAMER_BASE_INCLUDE_DIRS}
        ${GSTREAMER_APP_INCLUDE_DIRS}
        ${GSTREAMER_PBUTILS_INCLUDE_DIRS}
    )

    list(APPEND WebCore_LIBRARIES
        ${GSTREAMER_APP_LIBRARIES}
        ${GSTREAMER_BASE_LIBRARIES}
        ${GSTREAMER_LIBRARIES}
        ${GSTREAMER_PBUTILS_LIBRARIES}
    )
    # Avoiding a GLib deprecation warning due to GStreamer API using deprecated classes.
    set_source_files_properties(platform/audio/gstreamer/WebKitWebAudioSourceGStreamer.cpp PROPERTIES COMPILE_DEFINITIONS "GLIB_DISABLE_DEPRECATION_WARNINGS=1")
endif ()

if (ENABLE_VIDEO)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        ${GSTREAMER_TAG_INCLUDE_DIRS}
        ${GSTREAMER_VIDEO_INCLUDE_DIRS}
    )
    list(APPEND WebCore_LIBRARIES
        ${GSTREAMER_TAG_LIBRARIES}
        ${GSTREAMER_VIDEO_LIBRARIES}
    )
endif ()

if (WTF_USE_3D_GRAPHICS)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/graphics/opengl"
        "${WEBCORE_DIR}/platform/graphics/surfaces"
        "${WEBCORE_DIR}/platform/graphics/surfaces/efl"
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

        platform/graphics/efl/EvasGLContext.cpp
        platform/graphics/efl/EvasGLSurface.cpp
        platform/graphics/efl/GraphicsContext3DEfl.cpp
        platform/graphics/efl/GraphicsContext3DPrivate.cpp

        platform/graphics/opengl/Extensions3DOpenGLCommon.cpp
        platform/graphics/opengl/GLPlatformContext.cpp
        platform/graphics/opengl/GLPlatformSurface.cpp
        platform/graphics/opengl/GraphicsContext3DOpenGLCommon.cpp
        platform/graphics/opengl/TemporaryOpenGLSetting.cpp

        platform/graphics/surfaces/GLTransportSurface.cpp
        platform/graphics/surfaces/GraphicsSurface.cpp

        platform/graphics/surfaces/efl/GraphicsSurfaceCommon.cpp

        platform/graphics/surfaces/glx/X11Helper.cpp

        platform/graphics/texmap/TextureMapperGL.cpp
        platform/graphics/texmap/TextureMapperShaderProgram.cpp
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

    list(APPEND WebCore_LIBRARIES
        ${X11_X11_LIB}
    )

    if (WTF_USE_EGL)
        list(APPEND WebCore_LIBRARIES
            ${EGL_LIBRARY}
        )
    elseif (X11_Xcomposite_FOUND AND X11_Xrender_FOUND)
        list(APPEND WebCore_LIBRARIES
            ${X11_Xcomposite_LIB}
            ${X11_Xrender_LIB}
        )
    endif ()
endif ()

if (ENABLE_WEB_AUDIO)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/audio/gstreamer"

        ${GSTREAMER_AUDIO_INCLUDE_DIRS}
        ${GSTREAMER_FFT_INCLUDE_DIRS}
    )
    list(APPEND WebCore_LIBRARIES
        ${GSTREAMER_AUDIO_LIBRARIES}
        ${GSTREAMER_FFT_LIBRARIES}
    )
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

if (ENABLE_SPEECH_SYNTHESIS)
    list(APPEND WebCore_SOURCES
        platform/efl/PlatformSpeechSynthesizerEfl.cpp
        platform/efl/PlatformSpeechSynthesisProviderEfl.cpp
    )
endif ()
