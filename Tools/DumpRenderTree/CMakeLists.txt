set(DumpRenderTree_SOURCES
    AccessibilityController.cpp
    AccessibilityTextMarker.cpp
    AccessibilityUIElement.cpp
    CyclicRedundancyCheck.cpp
    DumpRenderTreeCommon.cpp
    GCController.cpp
    JavaScriptThreading.cpp
    PixelDumpSupport.cpp
    TestRunner.cpp
    WorkQueue.cpp
)

set(DumpRenderTree_LIBRARIES
    JavaScriptCore
    WTF
    WebCoreTestSupport
    WebKit
)

set(DumpRenderTree_INCLUDE_DIRECTORIES
    ${WEBCORE_DIR}
    ${WEBCORE_DIR}/bindings
    ${WEBCORE_DIR}/bridge
    ${WEBCORE_DIR}/bridge/jsc
    ${WEBCORE_DIR}/css
    ${WEBCORE_DIR}/dom
    ${WEBCORE_DIR}/editing
    ${WEBCORE_DIR}/history
    ${WEBCORE_DIR}/html
    ${WEBCORE_DIR}/inspector
    ${WEBCORE_DIR}/loader
    ${WEBCORE_DIR}/loader/cache
    ${WEBCORE_DIR}/loader/icon
    ${WEBCORE_DIR}/page
    ${WEBCORE_DIR}/page/animation
    ${WEBCORE_DIR}/platform
    ${WEBCORE_DIR}/platform/animation
    ${WEBCORE_DIR}/platform/graphics
    ${WEBCORE_DIR}/platform/graphics/transforms
    ${WEBCORE_DIR}/platform/network
    ${WEBCORE_DIR}/platform/text
    ${WEBCORE_DIR}/plugins
    ${WEBCORE_DIR}/rendering
    ${WEBCORE_DIR}/rendering/shapes
    ${WEBCORE_DIR}/rendering/style
    ${JAVASCRIPTCORE_DIR}
    ${JAVASCRIPTCORE_DIR}/API
    ${JAVASCRIPTCORE_DIR}/assembler
    ${JAVASCRIPTCORE_DIR}/bytecode
    ${JAVASCRIPTCORE_DIR}/dfg
    ${JAVASCRIPTCORE_DIR}/disassembler
    ${JAVASCRIPTCORE_DIR}/heap
    ${JAVASCRIPTCORE_DIR}/interpreter
    ${JAVASCRIPTCORE_DIR}/jit
    ${JAVASCRIPTCORE_DIR}/llint
    ${JAVASCRIPTCORE_DIR}/parser
    ${JAVASCRIPTCORE_DIR}/profiler
    ${JAVASCRIPTCORE_DIR}/runtime
    ${JAVASCRIPTCORE_DIR}/ForwardingHeaders
    ${DERIVED_SOURCES_DIR}/ForwardingHeaders
    ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}
    ${TOOLS_DIR}/DumpRenderTree
    ${WTF_DIR}
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_BINARY_DIR}
    ${DERIVED_SOURCES_DIR}
    ${DERIVED_SOURCES_WEBCORE_DIR}
    ${WEBCORE_DIR}/bindings/js
    ${WEBCORE_DIR}/testing/js
)

set(TestNetscapePlugin_SOURCES
    TestNetscapePlugIn/PluginObject.cpp
    TestNetscapePlugIn/PluginTest.cpp
    TestNetscapePlugIn/TestObject.cpp
    TestNetscapePlugIn/main.cpp

    TestNetscapePlugIn/Tests/DocumentOpenInDestroyStream.cpp
    TestNetscapePlugIn/Tests/EvaluateJSAfterRemovingPluginElement.cpp
    TestNetscapePlugIn/Tests/FormValue.cpp
    TestNetscapePlugIn/Tests/GetURLNotifyWithURLThatFailsToLoad.cpp
    TestNetscapePlugIn/Tests/GetURLWithJavaScriptURL.cpp
    TestNetscapePlugIn/Tests/GetURLWithJavaScriptURLDestroyingPlugin.cpp
    TestNetscapePlugIn/Tests/GetUserAgentWithNullNPPFromNPPNew.cpp
    TestNetscapePlugIn/Tests/LogNPPSetWindow.cpp
    TestNetscapePlugIn/Tests/NPDeallocateCalledBeforeNPShutdown.cpp
    TestNetscapePlugIn/Tests/NPPNewFails.cpp
    TestNetscapePlugIn/Tests/NPPSetWindowCalledDuringDestruction.cpp
    TestNetscapePlugIn/Tests/NPRuntimeCallsWithNullNPP.cpp
    TestNetscapePlugIn/Tests/NPRuntimeObjectFromDestroyedPlugin.cpp
    TestNetscapePlugIn/Tests/NPRuntimeRemoveProperty.cpp
    TestNetscapePlugIn/Tests/NullNPPGetValuePointer.cpp
    TestNetscapePlugIn/Tests/PassDifferentNPPStruct.cpp
    TestNetscapePlugIn/Tests/PluginScriptableNPObjectInvokeDefault.cpp
    TestNetscapePlugIn/Tests/PluginScriptableObjectOverridesAllProperties.cpp
    TestNetscapePlugIn/Tests/PrivateBrowsing.cpp
    TestNetscapePlugIn/Tests/ToStringAndValueOfObject.cpp
    TestNetscapePlugIn/Tests/URLRedirect.cpp
)

set(TestNetscapePlugin_LIBRARIES
    JavaScriptCore
    WTF
    WebCoreTestSupport
    WebKit
)

list(APPEND TestNetscapePlugin_LIBRARIES
    WebKit
)

WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

include_directories(${DumpRenderTree_INCLUDE_DIRECTORIES})
include_directories(SYSTEM ${DumpRenderTree_SYSTEM_INCLUDE_DIRECTORIES})

add_executable(DumpRenderTree ${DumpRenderTree_SOURCES})
target_link_libraries(DumpRenderTree ${DumpRenderTree_LIBRARIES})
set_target_properties(DumpRenderTree PROPERTIES FOLDER "Tools")

add_library(TestNetscapePlugin SHARED ${TestNetscapePlugin_SOURCES})
target_link_libraries(TestNetscapePlugin ${TestNetscapePlugin_LIBRARIES})
set_target_properties(TestNetscapePlugin PROPERTIES FOLDER "Tools")

if (WIN32)
    add_dependencies(DumpRenderTree DumpRenderTreeLib)
endif ()
