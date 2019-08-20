list(APPEND DumpRenderTree_INCLUDE_DIRECTORIES
    "${QtWebKit_FRAMEWORK_HEADERS_DIR}"
    "${QtWebKitWidgets_FRAMEWORK_HEADERS_DIR}"
    "${WEBKITLEGACY_DIR}/qt/WidgetSupport"
    TestNetscapePlugIn
    TestNetscapePlugIn/ForwardingHeaders
    qt
)

list(REMOVE_ITEM DumpRenderTree_SOURCES
    JavaScriptThreading.cpp
    PixelDumpSupport.cpp
    WorkQueueItem.cpp
)

list(APPEND DumpRenderTree_SOURCES
    qt/DumpRenderTreeMain.cpp
    qt/DumpRenderTreeQt.cpp
    qt/EventSenderQt.cpp
    qt/GCControllerQt.cpp
    qt/TestRunnerQt.cpp
    qt/TextInputControllerQt.cpp
    qt/WorkQueueItemQt.cpp
    qt/testplugin.cpp
)

qt5_add_resources(DumpRenderTree_SOURCES
    qt/DumpRenderTree.qrc
)

list(APPEND DumpRenderTree_SYSTEM_INCLUDE_DIRECTORIES
    ${ICU_INCLUDE_DIRS}
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
)

list(APPEND DumpRenderTree_LIBRARIES
    ${Qt5PrintSupport_LIBRARIES}
    ${Qt5Test_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    WebKitWidgets
)

if (WIN32)
    add_definitions(-DWEBCORE_EXPORT=)
    add_definitions(-DSTATICALLY_LINKED_WITH_WTF -DSTATICALLY_LINKED_WITH_JavaScriptCore)
endif ()

if (ENABLE_X11_TARGET AND ENABLE_NETSCAPE_PLUGIN_API)
    add_definitions(-DXP_UNIX)
    link_libraries(${X11_X11_LIB})
endif ()
