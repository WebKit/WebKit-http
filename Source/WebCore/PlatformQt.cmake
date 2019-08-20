include(platform/ImageDecoders.cmake)
include(platform/TextureMapper.cmake)

set(WebCore_OUTPUT_NAME WebCore)

if (NOT USE_LIBJPEG)
    list(REMOVE_ITEM WebCore_SOURCES
        platform/image-decoders/jpeg/JPEGImageDecoder.cpp
    )
endif ()

if (JPEG_DEFINITIONS)
    add_definitions(${JPEG_DEFINITIONS})
endif ()

list(APPEND WebCore_PRIVATE_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/bridge/qt"
    "${WEBCORE_DIR}/dom/qt"
    "${WEBCORE_DIR}/editing/qt"
    "${WEBCORE_DIR}/history/qt"
    "${WEBCORE_DIR}/page/qt"
    "${WEBCORE_DIR}/platform/qt"
    "${WEBCORE_DIR}/platform/audio/qt"
    "${WEBCORE_DIR}/platform/graphics/egl"
    "${WEBCORE_DIR}/platform/graphics/glx"
    "${WEBCORE_DIR}/platform/graphics/gpu/qt"
    "${WEBCORE_DIR}/platform/graphics/opengl"
    "${WEBCORE_DIR}/platform/graphics/surfaces"
    "${WEBCORE_DIR}/platform/graphics/surfaces/qt"
    "${WEBCORE_DIR}/platform/graphics/qt"
    "${WEBCORE_DIR}/platform/graphics/win"
    "${WEBCORE_DIR}/platform/network/qt"
    "${WEBCORE_DIR}/platform/text/qt"
    "${WEBCORE_DIR}/platform/win"
    "${WEBCORE_DIR}/platform/graphics/x11"
)

list(APPEND WebCore_PRIVATE_FRAMEWORK_HEADERS
    bindings/js/CachedScriptSourceProvider.h
    bindings/js/ScriptSourceCode.h

    bridge/qt/qt_instance.h
    bridge/qt/qt_runtime.h

    dom/StaticNodeList.h

    loader/NavigationScheduler.h

    loader/cache/CachedScript.h

    page/SpatialNavigation.h

    platform/graphics/qt/ImageBufferDataQt.h

    platform/network/HTTPStatusCodes.h
    platform/network/MIMESniffing.h
    platform/network/ResourceHandleInternal.h

    platform/network/qt/AuthenticationChallenge.h
    platform/network/qt/CertificateInfo.h
    platform/network/qt/QNetworkReplyHandler.h
    platform/network/qt/QtMIMETypeSniffer.h
    platform/network/qt/ResourceError.h
    platform/network/qt/ResourceRequest.h
    platform/network/qt/ResourceResponse.h

    platform/qt/KeyedDecoderQt.h
    platform/qt/KeyedEncoderQt.h
    platform/qt/PlatformGestureEvent.h
    platform/qt/QStyleFacade.h
    platform/qt/QWebPageClient.h
    platform/qt/UserAgentQt.h
)

list(APPEND WebCore_SOURCES
    accessibility/qt/AccessibilityObjectQt.cpp

    bindings/js/ScriptControllerQt.cpp

    bridge/qt/qt_class.cpp
    bridge/qt/qt_instance.cpp
    bridge/qt/qt_pixmapruntime.cpp
    bridge/qt/qt_runtime.cpp

    dom/qt/GestureEvent.cpp

    editing/qt/EditorQt.cpp

    page/qt/DragControllerQt.cpp
    page/qt/EventHandlerQt.cpp

    platform/ScrollAnimationKinetic.cpp
    platform/ScrollAnimationSmooth.cpp
    platform/UserAgentQuirks.cpp

    platform/audio/qt/AudioBusQt.cpp

    platform/generic/ScrollAnimatorGeneric.cpp

    platform/graphics/ImageSource.cpp
#    platform/graphics/PlatformDisplay.cpp
    platform/graphics/WOFFFileFormat.cpp

#    platform/graphics/texmap/BitmapTextureImageBuffer.cpp
#    platform/graphics/texmap/TextureMapperImageBuffer.cpp

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
    platform/graphics/qt/GraphicsContextQt.cpp
    platform/graphics/qt/IconQt.cpp
    platform/graphics/qt/ImageBufferDataQt.cpp
    platform/graphics/qt/ImageBufferQt.cpp
#    platform/graphics/qt/ImageDecoderQt.cpp
    platform/graphics/qt/ImageQt.cpp
    platform/graphics/qt/IntPointQt.cpp
    platform/graphics/qt/IntRectQt.cpp
    platform/graphics/qt/IntSizeQt.cpp
    platform/graphics/qt/NativeImageQt.cpp
    platform/graphics/qt/PathQt.cpp
    platform/graphics/qt/PatternQt.cpp
    platform/graphics/qt/StillImageQt.cpp
    platform/graphics/qt/TileQt.cpp
    platform/graphics/qt/TransformationMatrixQt.cpp

#    platform/graphics/x11/PlatformDisplayX11.cpp
    platform/graphics/x11/XUniqueResource.cpp

    platform/image-decoders/qt/ImageBackingStoreQt.cpp

    platform/network/MIMESniffing.cpp

    platform/network/qt/BlobUrlConversion.cpp
    platform/network/qt/CookieJarQt.cpp
    platform/network/qt/CredentialStorageQt.cpp
    platform/network/qt/DNSResolveQueueQt.cpp
    platform/network/qt/NetworkStateNotifierQt.cpp
    platform/network/qt/NetworkStorageSessionQt.cpp
    platform/network/qt/ProxyServerQt.cpp
    platform/network/qt/QNetworkReplyHandler.cpp
    platform/network/qt/QtMIMETypeSniffer.cpp
    platform/network/qt/ResourceHandleQt.cpp
    platform/network/qt/ResourceRequestQt.cpp
    platform/network/qt/ResourceResponseQt.cpp
    platform/network/qt/SocketStreamHandleImplQt.cpp
    platform/network/qt/SynchronousLoaderClientQt.cpp

    platform/qt/CursorQt.cpp
    platform/qt/DataTransferItemListQt.cpp
    platform/qt/DataTransferItemQt.cpp
    platform/qt/DragDataQt.cpp
    platform/qt/DragImageQt.cpp
    platform/qt/EventLoopQt.cpp
    platform/qt/KeyedDecoderQt.cpp
    platform/qt/KeyedEncoderQt.cpp
    platform/qt/LocalizedStringsQt.cpp
    platform/qt/LoggingQt.cpp
    platform/qt/MainThreadSharedTimerQt.cpp
    platform/qt/MIMETypeRegistryQt.cpp
    platform/qt/PasteboardQt.cpp
    platform/qt/PlatformKeyboardEventQt.cpp
    platform/qt/PlatformScreenQt.cpp
    platform/qt/RenderThemeQStyle.cpp
    platform/qt/RenderThemeQt.cpp
    platform/qt/RenderThemeQtMobile.cpp
    platform/qt/ScrollViewQt.cpp
    platform/qt/ScrollbarThemeQStyle.cpp
    platform/qt/ScrollbarThemeQt.cpp
    platform/qt/SharedBufferQt.cpp
    platform/qt/TemporaryLinkStubsQt.cpp
    platform/qt/ThirdPartyCookiesQt.cpp
    platform/qt/UserAgentQt.cpp
    platform/qt/WidgetQt.cpp

    platform/text/Hyphenation.cpp
    platform/text/LocaleICU.cpp

    platform/text/hyphen/HyphenationLibHyphen.cpp
)

QTWEBKIT_GENERATE_MOC_FILES_CPP(WebCore
    platform/network/qt/DNSResolveQueueQt.cpp
    platform/qt/MainThreadSharedTimerQt.cpp
)

QTWEBKIT_GENERATE_MOC_FILES_H(WebCore
    platform/network/qt/CookieJarQt.h
    platform/network/qt/QNetworkReplyHandler.h
    platform/network/qt/QtMIMETypeSniffer.h
)

QTWEBKIT_GENERATE_MOC_FILE_H(WebCore platform/network/qt/NetworkStateNotifierPrivate.h platform/network/qt/NetworkStateNotifierQt.cpp)
QTWEBKIT_GENERATE_MOC_FILE_H(WebCore platform/network/qt/SocketStreamHandlePrivate.h platform/network/qt/SocketStreamHandleImplQt.cpp)

if (COMPILER_IS_GCC_OR_CLANG)
    set_source_files_properties(
        platform/graphics/qt/ImageBufferDataQt.cpp
    PROPERTIES
        COMPILE_FLAGS "-frtti -UQT_NO_DYNAMIC_CAST"
    )

    set_source_files_properties(
        platform/network/qt/BlobUrlConversion.cpp
    PROPERTIES
        COMPILE_FLAGS "-fexceptions -UQT_NO_EXCEPTIONS"
    )
endif ()

if (ENABLE_DEVICE_ORIENTATION)
    list(APPEND WebCore_SOURCES
        platform/qt/DeviceMotionClientQt.cpp
        platform/qt/DeviceMotionProviderQt.cpp
        platform/qt/DeviceOrientationClientQt.cpp
        platform/qt/DeviceOrientationProviderQt.cpp
    )
endif ()

if (ENABLE_GRAPHICS_CONTEXT_3D)
    list(APPEND WebCore_SOURCES
        platform/graphics/qt/GraphicsContext3DQt.cpp
    )
endif ()

if (ENABLE_TOUCH_ADJUSTMENT)
    list(APPEND WebCore_SOURCES
        page/qt/TouchAdjustment.cpp
    )
endif ()

if (ENABLE_NETSCAPE_PLUGIN_API)
    if (WIN32)
        list(APPEND WebCore_SOURCES
            platform/graphics/win/TransformationMatrixWin.cpp

            platform/win/BitmapInfo.cpp
            platform/win/WebCoreInstanceHandle.cpp
        )
        list(APPEND WebCore_LIBRARIES
            shlwapi
            version
        )
    elseif (PLUGIN_BACKEND_XLIB)
        list(APPEND WebCore_SOURCES
            plugins/qt/QtX11ImageConversion.cpp
        )
    endif ()
endif ()

# Do it in the WebCore to support SHARED_CORE since WebKitWidgets won't load WebKitLegacy in that case.
# This should match the opposite statement in WebKitLegacy/PlatformQt.cmake
if (SHARED_CORE)
    qt5_add_resources(WebCore_SOURCES
        WebCore.qrc
    )

    if (ENABLE_INSPECTOR_UI)
        include("${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/PlatformQt.cmake")
        list(APPEND WebCore_SOURCES
            "${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/qrc_WebInspector.cpp"
        )
    endif ()
endif ()

# Note: Qt5Network_INCLUDE_DIRS includes Qt5Core_INCLUDE_DIRS
list(APPEND WebCore_SYSTEM_INCLUDE_DIRECTORIES
    ${HYPHEN_INCLUDE_DIR}
    ${LIBXML2_INCLUDE_DIR}
    ${LIBXSLT_INCLUDE_DIR}
    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
    ${Qt5Sensors_INCLUDE_DIRS}
    ${SQLITE_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)

list(APPEND WebCore_LIBRARIES
    ${HYPHEN_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${Qt5Core_LIBRARIES}
    ${Qt5Gui_LIBRARIES}
    ${Qt5Network_LIBRARIES}
    ${Qt5Sensors_LIBRARIES}
    ${SQLITE_LIBRARIES}
    ${X11_X11_LIB}
    ${ZLIB_LIBRARIES}
)

if (QT_STATIC_BUILD)
    list(APPEND WebCore_LIBRARIES
        ${STATIC_LIB_DEPENDENCIES}
    )
endif ()

list(APPEND WebCore_USER_AGENT_STYLE_SHEETS
#    ${WEBCORE_DIR}/css/mediaControlsGtk.css
#    ${WEBCORE_DIR}/css/mediaControlsQt.css
#    ${WEBCORE_DIR}/css/mediaControlsQtFullscreen.css
    ${WEBCORE_DIR}/css/mobileThemeQt.css
    ${WEBCORE_DIR}/css/themeQtNoListboxes.css
)

if (ENABLE_WEBKIT)
    list(APPEND WebCore_SOURCES
        page/qt/GestureTapHighlighter.cpp
    )
endif ()

if (ENABLE_OPENGL)
    list(APPEND WebCore_SOURCES
        platform/graphics/opengl/Extensions3DOpenGLCommon.cpp
        platform/graphics/opengl/GraphicsContext3DOpenGLCommon.cpp
        platform/graphics/opengl/TemporaryOpenGLSetting.cpp

        platform/graphics/qt/QFramebufferPaintDevice.cpp
    )

    if (${Qt5Gui_OPENGL_IMPLEMENTATION} STREQUAL GLESv2)
        list(APPEND WebCore_SOURCES
            platform/graphics/opengl/Extensions3DOpenGLES.cpp
            platform/graphics/opengl/GraphicsContext3DOpenGLES.cpp
        )
        list(APPEND WebCore_LIBRARIES
            ${Qt5Gui_EGL_LIBRARIES}
            ${Qt5Gui_OPENGL_LIBRARIES}
        )
    else ()
        list(APPEND WebCore_SOURCES
            platform/graphics/opengl/Extensions3DOpenGL.cpp
            platform/graphics/opengl/GraphicsContext3DOpenGL.cpp
        )
    endif ()
endif ()

if (USE_GLIB)
    list(APPEND WebCore_SYSTEM_INCLUDE_DIRECTORIES
        ${GIO_UNIX_INCLUDE_DIRS}
        ${GLIB_INCLUDE_DIRS}
    )
    list(APPEND WebCore_LIBRARIES
        ${GLIB_GIO_LIBRARIES}
        ${GLIB_GOBJECT_LIBRARIES}
        ${GLIB_LIBRARIES}
    )
endif ()

if (USE_GSTREAMER)
    include(platform/GStreamer.cmake)
    list(APPEND WebCore_SOURCES
        platform/graphics/gstreamer/ImageGStreamerQt.cpp
    )
endif ()

if (USE_MEDIA_FOUNDATION)
    list(APPEND WebCore_SOURCES
        platform/graphics/win/MediaPlayerPrivateMediaFoundation.cpp
    )
    list(APPEND WebCore_LIBRARIES
        mfuuid
        strmbase
    )
endif ()

if (USE_QT_MULTIMEDIA)
    list(APPEND WebCore_SOURCES
        platform/graphics/qt/MediaPlayerPrivateQt.cpp
    )
    list(APPEND WebCore_LIBRARIES
        ${Qt5Multimedia_LIBRARIES}
    )
    QTWEBKIT_GENERATE_MOC_FILES_H(WebCore platform/graphics/qt/MediaPlayerPrivateQt.h)
endif ()

if (ENABLE_VIDEO)
    list(APPEND WebCore_USER_AGENT_STYLE_SHEETS
        ${WEBCORE_DIR}/Modules/mediacontrols/mediaControlsBase.css
    )
    set(WebCore_USER_AGENT_SCRIPTS
        ${WEBCORE_DIR}/en.lproj/mediaControlsLocalizedStrings.js
        ${WEBCORE_DIR}/Modules/mediacontrols/mediaControlsBase.js
    )
    set(WebCore_USER_AGENT_SCRIPTS_DEPENDENCIES ${WEBCORE_DIR}/platform/qt/RenderThemeQt.cpp)
endif ()

# Build the include path with duplicates removed
list(REMOVE_DUPLICATES WebCore_SYSTEM_INCLUDE_DIRECTORIES)

list(APPEND WebCoreTestSupport_LIBRARIES
    WebCore
)

if (HAVE_FONTCONFIG)
    list(APPEND WebCoreTestSupport_LIBRARIES
        ${FONTCONFIG_LIBRARIES}
    )
endif ()

# From PlatformWin.cmake

if (WIN32)
    # Eliminate C2139 errors
    if (MSVC)
        add_compile_options(/D_ENABLE_EXTENDED_ALIGNED_STORAGE)
    endif ()

    if (${JavaScriptCore_LIBRARY_TYPE} MATCHES STATIC)
        add_definitions(-DSTATICALLY_LINKED_WITH_WTF -DSTATICALLY_LINKED_WITH_JavaScriptCore)
    endif ()

    list(APPEND WebCore_SOURCES
        platform/win/SystemInfo.cpp
    )
endif ()

if (APPLE)
    list(APPEND WebCore_SOURCES
        platform/cf/SharedBufferCF.cpp
    )
endif ()
