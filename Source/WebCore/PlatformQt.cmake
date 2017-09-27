include(platform/ImageDecoders.cmake)
include(platform/Linux.cmake)
include(platform/TextureMapper.cmake)

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
    "${WEBCORE_DIR}/Modules/gamepad"
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
    "${WTF_DIR}"
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
    page/qt/TouchAdjustment.cpp

    platform/KillRingNone.cpp

    platform/audio/qt/AudioBusQt.cpp

    platform/graphics/ImageSource.cpp
    platform/graphics/PlatformDisplay.cpp
    platform/graphics/WOFFFileFormat.cpp

    platform/graphics/texmap/BitmapTextureImageBuffer.cpp
    platform/graphics/texmap/TextureMapperImageBuffer.cpp

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

    platform/graphics/surfaces/qt/GraphicsSurfaceQt.cpp

    platform/graphics/x11/PlatformDisplayX11.cpp
    platform/graphics/x11/XUniqueResource.cpp

    platform/network/NetworkStorageSessionStub.cpp
    platform/network/MIMESniffing.cpp

    platform/network/qt/BlobUrlConversion.cpp
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
    platform/network/qt/SynchronousLoaderClientQt.cpp

    platform/qt/CursorQt.cpp
    platform/qt/DataTransferItemListQt.cpp
    platform/qt/DataTransferItemQt.cpp
    platform/qt/DragDataQt.cpp
    platform/qt/DragImageQt.cpp
    platform/qt/EventLoopQt.cpp
    platform/qt/FileSystemQt.cpp
    platform/qt/KeyedDecoderQt.cpp
    platform/qt/KeyedEncoderQt.cpp
    platform/qt/LanguageQt.cpp
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
    platform/qt/SoundQt.cpp
    platform/qt/TemporaryLinkStubsQt.cpp
    platform/qt/ThirdPartyCookiesQt.cpp
    platform/qt/URLQt.cpp
    platform/qt/UserAgentQt.cpp
    platform/qt/WidgetQt.cpp

    platform/text/Hyphenation.cpp
    platform/text/LocaleICU.cpp

    platform/text/hyphen/HyphenationLibHyphen.cpp

    platform/text/qt/TextBreakIteratorInternalICUQt.cpp
)

QTWEBKIT_GENERATE_MOC_FILES_CPP(WebCore
    platform/network/qt/DNSQt.cpp
    platform/qt/MainThreadSharedTimerQt.cpp
)

QTWEBKIT_GENERATE_MOC_FILES_H(WebCore
    platform/network/qt/CookieJarQt.h
    platform/network/qt/QNetworkReplyHandler.h
    platform/network/qt/QtMIMETypeSniffer.h
)

QTWEBKIT_GENERATE_MOC_FILE_H(WebCore platform/network/qt/NetworkStateNotifierPrivate.h platform/network/qt/NetworkStateNotifierQt.cpp)
QTWEBKIT_GENERATE_MOC_FILE_H(WebCore platform/network/qt/SocketStreamHandlePrivate.h platform/network/qt/SocketStreamHandleQt.cpp)

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

if (ENABLE_GAMEPAD_DEPRECATED)
    list(APPEND WebCore_SOURCES
        platform/qt/GamepadsQt.cpp
    )
    QTWEBKIT_GENERATE_MOC_FILES_CPP(WebCore platform/qt/GamepadsQt.cpp)
endif ()

if (ENABLE_GRAPHICS_CONTEXT_3D)
    list(APPEND WebCore_SOURCES
        platform/graphics/qt/GraphicsContext3DQt.cpp
    )
endif ()

if (ENABLE_NETSCAPE_PLUGIN_API)
    if (WIN32)
        list(APPEND WebCore_FORWARDING_HEADERS_FILES
            platform/graphics/win/LocalWindowsContext.h

            platform/win/BitmapInfo.h
            platform/win/WebCoreInstanceHandle.h
        )
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
        list(APPEND WebCore_FORWARDING_HEADERS_FILES
           plugins/qt/QtX11ImageConversion.h
        )
        list(APPEND WebCore_SOURCES
            plugins/qt/QtX11ImageConversion.cpp
        )
    endif ()
endif ()

if (ENABLE_SMOOTH_SCROLLING)
    list(APPEND WebCore_SOURCES
        platform/ScrollAnimationSmooth.cpp
        platform/ScrollAnimatorSmooth.cpp
    )
endif ()

# Do it in the WebCore to support SHARED_CORE since WebKitWidgets won't load WebKit in that case.
# This should match the opposite statement in WebKit/PlatformQt.cmake
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

if (ENABLE_WEBKIT2)
    list(APPEND WebCore_SOURCES
        page/qt/GestureTapHighlighter.cpp
    )
    if (USE_MACH_PORTS)
        list(APPEND WebCore_FORWARDING_HEADERS_FILES
            platform/cocoa/MachSendRight.h

            platform/spi/cocoa/MachVMSPI.h
        )
        list(APPEND WebCore_SOURCES
            platform/cocoa/MachSendRight.cpp
        )
    endif ()
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
        ${WEBCORE_DIR}/English.lproj/mediaControlsLocalizedStrings.js
        ${WEBCORE_DIR}/Modules/mediacontrols/mediaControlsBase.js
    )
    set(WebCore_USER_AGENT_SCRIPTS_DEPENDENCIES ${WEBCORE_DIR}/platform/qt/RenderThemeQt.cpp)
endif ()

# Build the include path with duplicates removed
list(REMOVE_DUPLICATES WebCore_SYSTEM_INCLUDE_DIRECTORIES)

# TODO: Think how to unify fwd headers handling throughout WebKit
set(WebCore_FORWARDING_HEADERS_DIRECTORIES
    bridge
    dom
    html
    loader
    page
    platform
    rendering
    storage

    Modules/indexeddb/legacy
    Modules/indexeddb/shared

    bindings/js

    bridge/c
    bridge/jsc

    platform/graphics
    platform/network
    platform/sql
    platform/text

    platform/network/qt
)

WEBKIT_CREATE_FORWARDING_HEADERS(WebCore DIRECTORIES ${WebCore_FORWARDING_HEADERS_DIRECTORIES} FILES ${WebCore_FORWARDING_HEADERS_FILES})

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
    if (${JavaScriptCore_LIBRARY_TYPE} MATCHES STATIC)
        add_definitions(-DSTATICALLY_LINKED_WITH_WTF -DSTATICALLY_LINKED_WITH_JavaScriptCore)
    endif ()

    list(APPEND WebCore_SOURCES
        platform/win/SystemInfo.cpp
    )
endif ()

if (APPLE)
    list(APPEND WebCore_SOURCES
        platform/VNodeTracker.cpp

        platform/cf/SharedBufferCF.cpp
    )
endif ()

# From PlatformEfl.cmake
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/WebKitVersion.h
    MAIN_DEPENDENCY ${WEBKIT_DIR}/scripts/generate-webkitversion.pl
    DEPENDS ${WEBKIT_DIR}/mac/Configurations/Version.xcconfig
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT_DIR}/scripts/generate-webkitversion.pl --config ${WEBKIT_DIR}/mac/Configurations/Version.xcconfig --outputDir ${DERIVED_SOURCES_WEBCORE_DIR}
    VERBATIM)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/WebKitVersion.h)
