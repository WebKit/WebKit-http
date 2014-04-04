LIST(APPEND WebKit_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}/Source"
    "${WEBKIT_DIR}/haiku/API"
    "${WEBKIT_DIR}/haiku/WebCoreSupport"
    "${JAVASCRIPTCORE_DIR}/ForwardingHeaders"
    "${WEBCORE_DIR}/platform/haiku"
    "${WEBCORE_DIR}/platform/graphics/haiku"
    "${WEBCORE_DIR}/platform/graphics/opentype"
    "${WEBCORE_DIR}/platform/network/haiku"
    "${WEBCORE_DIR}/rendering/svg"
    "${WEBCORE_DIR}/svg"
    "${WEBCORE_DIR}/svg/animation"
    ${LIBXML2_INCLUDE_DIR}
    ${LIBXSLT_INCLUDE_DIR}
    ${SQLITE_INCLUDE_DIR}
  )

IF (ENABLE_VIDEO_TRACK)
  LIST(APPEND WebKit_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/html/track"
  )
ENDIF ()

IF (ENABLE_NOTIFICATIONS)
  LIST(APPEND WebKit_INCLUDE_DIRECTORIES
      "${WEBCORE_DIR}/Modules/notifications"
  )
ENDIF ()

LIST(APPEND WebKit_SOURCES
    haiku/WebCoreSupport/ChromeClientHaiku.cpp
    haiku/WebCoreSupport/ContextMenuClientHaiku.cpp
    haiku/WebCoreSupport/DragClientHaiku.cpp
    haiku/WebCoreSupport/DumpRenderTreeSupportHaiku.cpp
    haiku/WebCoreSupport/EditorClientHaiku.cpp
    haiku/WebCoreSupport/FrameLoaderClientHaiku.cpp
    haiku/WebCoreSupport/FrameNetworkingContextHaiku.cpp
    haiku/WebCoreSupport/InspectorClientHaiku.cpp
    haiku/WebCoreSupport/PlatformStrategiesHaiku.cpp
    haiku/WebCoreSupport/ProgressTrackerHaiku.cpp

    haiku/API/WebDownload.cpp
    haiku/API/WebDownloadPrivate.cpp
    haiku/API/WebFrame.cpp
    haiku/API/WebKitInfo.cpp
    haiku/API/WebPage.cpp
    haiku/API/WebSettings.cpp
    haiku/API/WebSettingsPrivate.cpp
    haiku/API/WebView.cpp
    haiku/API/WebWindow.cpp
)

LIST(APPEND WebKit_LIBRARIES
    ${LIBXML2_LIBRARIES}
    ${SQLITE_LIBRARIES}
    ${PNG_LIBRARY}
    ${JPEG_LIBRARY}
    ${CMAKE_DL_LIBS}
    be bnetapi translation tracker
)

INSTALL(FILES
    haiku/API/WebWindow.h
    haiku/API/WebViewConstants.h
    haiku/API/WebView.h
    haiku/API/WebSettings.h
    haiku/API/WebPage.h
    haiku/API/WebKitInfo.h
    haiku/API/WebFrame.h
    haiku/API/WebDownload.h
    DESTINATION develop/headers${CMAKE_HAIKU_SECONDARY_ARCH_SUBDIR}
    COMPONENT devel
)
