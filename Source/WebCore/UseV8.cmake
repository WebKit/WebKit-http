ADD_DEFINITIONS(-DUSING_V8_SHARED)
ADD_DEFINITIONS(-DWTF_CHANGES=1)

LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/bindings/v8"
    "${WEBCORE_DIR}/bindings/v8/custom"
    "${WEBCORE_DIR}/bindings/v8/specialization"
    "${JAVASCRIPTCORE_DIR}/runtime"
)

LIST(APPEND WebCore_IDL_INCLUDES
    bindings/v8
)

LIST(APPEND WebCore_SOURCES
    bindings/v8/DOMData.cpp
    bindings/v8/DOMDataStore.cpp
    bindings/v8/DOMWrapperWorld.cpp
    bindings/v8/DateExtension.cpp
    bindings/v8/IDBBindingUtilities.cpp
    bindings/v8/IsolatedWorld.cpp
    bindings/v8/OptionsObject.cpp
    bindings/v8/PageScriptDebugServer.cpp
    bindings/v8/RetainedDOMInfo.cpp
    bindings/v8/ScheduledAction.cpp
    bindings/v8/ScopedDOMDataStore.cpp
    bindings/v8/ScriptCachedFrameData.cpp
    bindings/v8/ScriptCallStackFactory.cpp
    bindings/v8/ScriptController.cpp
    bindings/v8/ScriptEventListener.cpp
    bindings/v8/ScriptFunctionCall.cpp
    bindings/v8/ScriptGCEvent.cpp
    bindings/v8/ScriptInstance.cpp
    bindings/v8/ScriptObject.cpp
    bindings/v8/ScriptScope.cpp
    bindings/v8/ScriptState.cpp
    bindings/v8/ScriptValue.cpp
    bindings/v8/SerializedScriptValue.cpp
    bindings/v8/StaticDOMDataStore.cpp
    bindings/v8/V8AbstractEventListener.cpp
    bindings/v8/V8Binding.cpp
    bindings/v8/V8Collection.cpp
    bindings/v8/V8DOMMap.cpp
    bindings/v8/V8DOMWindowShell.cpp
    bindings/v8/V8DOMWrapper.cpp
    bindings/v8/V8EventListener.cpp
    bindings/v8/V8EventListenerList.cpp
    bindings/v8/V8GCController.cpp
    bindings/v8/V8GCForContextDispose.cpp
    bindings/v8/V8Helpers.cpp
    bindings/v8/V8HiddenPropertyName.cpp
    bindings/v8/V8IsolatedContext.cpp
    bindings/v8/V8LazyEventListener.cpp
    bindings/v8/V8NodeFilterCondition.cpp
    bindings/v8/V8Proxy.cpp
    bindings/v8/V8Utilities.cpp
    bindings/v8/V8WindowErrorHandler.cpp
    bindings/v8/V8WorkerContextErrorHandler.cpp
    bindings/v8/V8WorkerContextEventListener.cpp
    bindings/v8/WorkerContextExecutionProxy.cpp
    bindings/v8/WorkerScriptController.cpp
    bindings/v8/WorkerScriptDebugServer.cpp
    bindings/v8/WorldContextHandle.cpp
    bindings/v8/npruntime.cpp

    bindings/v8/custom/V8ArrayBufferCustom.cpp
    bindings/v8/custom/V8ArrayBufferViewCustom.cpp
    bindings/v8/custom/V8AudioContextCustom.cpp
    bindings/v8/custom/V8AudioNodeCustom.cpp
    bindings/v8/custom/V8CSSRuleCustom.cpp
    bindings/v8/custom/V8CSSStyleDeclarationCustom.cpp
    bindings/v8/custom/V8CSSStyleSheetCustom.cpp
    bindings/v8/custom/V8CSSValueCustom.cpp
    bindings/v8/custom/V8CanvasPixelArrayCustom.cpp
    bindings/v8/custom/V8CanvasRenderingContext2DCustom.cpp
    bindings/v8/custom/V8ClipboardCustom.cpp
    bindings/v8/custom/V8ConsoleCustom.cpp
    bindings/v8/custom/V8CoordinatesCustom.cpp
    bindings/v8/custom/V8CustomSQLStatementErrorCallback.cpp
    bindings/v8/custom/V8CustomVoidCallback.cpp
    bindings/v8/custom/V8CustomXPathNSResolver.cpp
    bindings/v8/custom/V8DOMFormDataCustom.cpp
    bindings/v8/custom/V8DOMSettableTokenListCustom.cpp
    bindings/v8/custom/V8DOMStringMapCustom.cpp
    bindings/v8/custom/V8DOMTokenListCustom.cpp
    bindings/v8/custom/V8DOMWindowCustom.cpp
    bindings/v8/custom/V8DataViewCustom.cpp
    bindings/v8/custom/V8DedicatedWorkerContextCustom.cpp
    bindings/v8/custom/V8DeviceMotionEventCustom.cpp
    bindings/v8/custom/V8DeviceOrientationEventCustom.cpp
    bindings/v8/custom/V8DirectoryEntryCustom.cpp
    bindings/v8/custom/V8DirectoryEntrySyncCustom.cpp
    bindings/v8/custom/V8DocumentCustom.cpp
    bindings/v8/custom/V8DocumentLocationCustom.cpp
    bindings/v8/custom/V8ElementCustom.cpp
    bindings/v8/custom/V8EntrySyncCustom.cpp
    bindings/v8/custom/V8EventConstructors.cpp
    bindings/v8/custom/V8EventCustom.cpp
    bindings/v8/custom/V8FileReaderCustom.cpp
    bindings/v8/custom/V8Float32ArrayCustom.cpp
    bindings/v8/custom/V8Float64ArrayCustom.cpp
    bindings/v8/custom/V8GeolocationCustom.cpp
    bindings/v8/custom/V8HTMLAllCollectionCustom.cpp
    bindings/v8/custom/V8HTMLCanvasElementCustom.cpp
    bindings/v8/custom/V8HTMLCollectionCustom.cpp
    bindings/v8/custom/V8HTMLDocumentCustom.cpp
    bindings/v8/custom/V8HTMLElementCustom.cpp
    bindings/v8/custom/V8HTMLFormElementCustom.cpp
    bindings/v8/custom/V8HTMLFrameElementCustom.cpp
    bindings/v8/custom/V8HTMLFrameSetElementCustom.cpp
    bindings/v8/custom/V8HTMLImageElementConstructor.cpp
    bindings/v8/custom/V8HTMLInputElementCustom.cpp
    bindings/v8/custom/V8HTMLLinkElementCustom.cpp
    bindings/v8/custom/V8HTMLOptionElementConstructor.cpp
    bindings/v8/custom/V8HTMLOptionsCollectionCustom.cpp
    bindings/v8/custom/V8HTMLOutputElementCustom.cpp
    bindings/v8/custom/V8HTMLPlugInElementCustom.cpp
    bindings/v8/custom/V8HTMLSelectElementCustom.cpp
    bindings/v8/custom/V8HistoryCustom.cpp
    bindings/v8/custom/V8IDBAnyCustom.cpp
    bindings/v8/custom/V8IDBKeyCustom.cpp
    bindings/v8/custom/V8ImageDataCustom.cpp
    bindings/v8/custom/V8InjectedScriptHostCustom.cpp
    bindings/v8/custom/V8InjectedScriptManager.cpp
    bindings/v8/custom/V8InspectorFrontendHostCustom.cpp
    bindings/v8/custom/V8Int16ArrayCustom.cpp
    bindings/v8/custom/V8Int32ArrayCustom.cpp
    bindings/v8/custom/V8Int8ArrayCustom.cpp
    bindings/v8/custom/V8LocationCustom.cpp
    bindings/v8/custom/V8MessageChannelConstructor.cpp
    bindings/v8/custom/V8MessageEventCustom.cpp
    bindings/v8/custom/V8MessagePortCustom.cpp
    bindings/v8/custom/V8NamedNodeMapCustom.cpp
    bindings/v8/custom/V8NamedNodesCollection.cpp
    bindings/v8/custom/V8NavigatorCustom.cpp
    bindings/v8/custom/V8NodeCustom.cpp
    bindings/v8/custom/V8NodeListCustom.cpp
    bindings/v8/custom/V8NotificationCenterCustom.cpp
    bindings/v8/custom/V8PerformanceCustom.cpp
    bindings/v8/custom/V8PopStateEventCustom.cpp
    bindings/v8/custom/V8SQLResultSetRowListCustom.cpp
    bindings/v8/custom/V8SQLTransactionCustom.cpp
    bindings/v8/custom/V8SQLTransactionSyncCustom.cpp
    bindings/v8/custom/V8StorageCustom.cpp
    bindings/v8/custom/V8StyleSheetCustom.cpp
    bindings/v8/custom/V8StyleSheetListCustom.cpp
    bindings/v8/custom/V8Uint16ArrayCustom.cpp
    bindings/v8/custom/V8Uint32ArrayCustom.cpp
    bindings/v8/custom/V8Uint8ArrayCustom.cpp
    bindings/v8/custom/V8WebGLRenderingContextCustom.cpp
    bindings/v8/custom/V8WebKitAnimationCustom.cpp
    bindings/v8/custom/V8WebKitPointConstructor.cpp
    bindings/v8/custom/V8WebSocketCustom.cpp
    bindings/v8/custom/V8WorkerContextCustom.cpp
    bindings/v8/custom/V8WorkerCustom.cpp
    bindings/v8/custom/V8XMLHttpRequestConstructor.cpp
    bindings/v8/custom/V8XMLHttpRequestCustom.cpp
    bindings/v8/custom/V8XSLTProcessorCustom.cpp

    bindings/v8/specialization/V8BindingState.cpp
)

LIST(APPEND WebCore_SOURCES
    ${JAVASCRIPTCORE_DIR}/yarr/YarrInterpreter.cpp
    ${JAVASCRIPTCORE_DIR}/yarr/YarrJIT.cpp
    ${JAVASCRIPTCORE_DIR}/yarr/YarrPattern.cpp
    ${JAVASCRIPTCORE_DIR}/yarr/YarrSyntaxChecker.cpp
)

IF (ENABLE_JAVASCRIPT_DEBUGGER)
    LIST(APPEND WebCore_SOURCES
        bindings/v8/JavaScriptCallFrame.cpp
        bindings/v8/ScriptDebugServer.cpp
        bindings/v8/ScriptHeapSnapshot.cpp
        bindings/v8/ScriptProfile.cpp
        bindings/v8/ScriptProfileNode.cpp
        bindings/v8/ScriptProfiler.cpp

        bindings/v8/custom/V8JavaScriptCallFrameCustom.cpp
        bindings/v8/custom/V8ScriptProfileCustom.cpp
        bindings/v8/custom/V8ScriptProfileNodeCustom.cpp
    )
ENDIF ()

IF (ENABLE_NETSCAPE_PLUGIN_API)
    LIST(APPEND WebCore_SOURCES
        bindings/v8/NPV8Object.cpp
        bindings/v8/V8NPObject.cpp
        bindings/v8/V8NPUtils.cpp
    )
ENDIF()

IF (ENABLE_VIDEO)
    LIST(APPEND WebCore_SOURCES
        bindings/v8/custom/V8HTMLAudioElementConstructor.cpp
    )
ENDIF ()

IF (ENABLE_SVG)
    LIST(APPEND WebCore_SOURCES
        bindings/v8/custom/V8SVGDocumentCustom.cpp
        bindings/v8/custom/V8SVGElementCustom.cpp
        bindings/v8/custom/V8SVGLengthCustom.cpp
        bindings/v8/custom/V8SVGPathSegCustom.cpp
    )
ENDIF ()

LIST(APPEND SCRIPTS_BINDINGS
    ${WEBCORE_DIR}/bindings/scripts/CodeGenerator.pm
)

SET(IDL_INCLUDES "")
FOREACH (_include ${WebCore_IDL_INCLUDES})
    LIST(APPEND IDL_INCLUDES --include=${WEBCORE_DIR}/${_include})
ENDFOREACH ()

SET(FEATURE_DEFINES_JAVASCRIPT "LANGUAGE_JAVASCRIPT=1 V8_BINDING=1")
FOREACH (_feature ${FEATURE_DEFINES})
    SET(FEATURE_DEFINES_JAVASCRIPT "${FEATURE_DEFINES_JAVASCRIPT} ${_feature}")
ENDFOREACH ()

# Generate DebuggerScriptSource.h
ADD_CUSTOM_COMMAND(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/DebuggerScriptSource.h
    MAIN_DEPENDENCY ${WEBCORE_DIR}/bindings/v8/DebuggerScript.js
    DEPENDS ${WEBCORE_DIR}/inspector/xxd.pl
    COMMAND ${PERL_EXECUTABLE} ${WEBCORE_DIR}/inspector/xxd.pl DebuggerScriptSource_js ${WEBCORE_DIR}/bindings/v8/DebuggerScript.js ${DERIVED_SOURCES_WEBCORE_DIR}/DebuggerScriptSource.h
    VERBATIM)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/DebuggerScriptSource.h)

#GENERATOR: "RegExpJitTables.h": tables used by Yarr
ADD_CUSTOM_COMMAND(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/RegExpJitTables.h
    MAIN_DEPENDENCY ${JAVASCRIPTCORE_DIR}/create_regex_tables
    COMMAND ${PYTHON_EXECUTABLE} ${JAVASCRIPTCORE_DIR}/create_regex_tables > ${DERIVED_SOURCES_WEBCORE_DIR}/RegExpJitTables.h
    VERBATIM)
ADD_SOURCE_DEPENDENCIES(${JAVASCRIPTCORE_DIR}/yarr/YarrPattern.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/RegExpJitTables.h)

# Generate V8ArrayBufferViewCustomScript.h
ADD_CUSTOM_COMMAND(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/V8ArrayBufferViewCustomScript.h
    MAIN_DEPENDENCY ${WEBCORE_DIR}/bindings/v8/custom/V8ArrayBufferViewCustomScript.js
    DEPENDS ${WEBCORE_DIR}/inspector/xxd.pl
    COMMAND ${PERL_EXECUTABLE} ${WEBCORE_DIR}/inspector/xxd.pl V8ArrayBufferViewCustomScript_js ${WEBCORE_DIR}/bindings/v8/custom/V8ArrayBufferViewCustomScript.js ${DERIVED_SOURCES_WEBCORE_DIR}/V8ArrayBufferViewCustomScript.h
    VERBATIM)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/V8ArrayBufferViewCustomScript.h)

# Create JavaScript C++ code given an IDL input
FOREACH (_file ${WebCore_IDL_FILES})
    GET_FILENAME_COMPONENT (_name ${_file} NAME_WE)
    ADD_CUSTOM_COMMAND(
        OUTPUT  ${DERIVED_SOURCES_WEBCORE_DIR}/V8${_name}.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/V8${_name}.h
        MAIN_DEPENDENCY ${_file}
        DEPENDS ${WEBCORE_DIR}/bindings/scripts/generate-bindings.pl ${SCRIPTS_BINDINGS} ${WEBCORE_DIR}/bindings/scripts/CodeGeneratorV8.pm ${_file}
        COMMAND ${PERL_EXECUTABLE} -I${WEBCORE_DIR}/bindings/scripts ${WEBCORE_DIR}/bindings/scripts/generate-bindings.pl --defines "${FEATURE_DEFINES_JAVASCRIPT}" --generator V8 ${IDL_INCLUDES} --outputDir "${DERIVED_SOURCES_WEBCORE_DIR}" --preprocessor "${CODE_GENERATOR_PREPROCESSOR}" ${WEBCORE_DIR}/${_file}
        VERBATIM)
    LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/V8${_name}.cpp)
ENDFOREACH ()
