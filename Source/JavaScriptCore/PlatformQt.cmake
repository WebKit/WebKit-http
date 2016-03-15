if (${JavaScriptCore_LIBRARY_TYPE} MATCHES STATIC)
    add_definitions(-DSTATICALLY_LINKED_WITH_WTF)
endif ()

list(APPEND JavaScriptCore_INCLUDE_DIRECTORIES
    ${WTF_DIR}
)

list(APPEND JavaScriptCore_SYSTEM_INCLUDE_DIRECTORIES
    ${Qt5Core_INCLUDE_DIRS}
)

list(APPEND JavaScriptCore_LIBRARIES
    ${Qt5Core_LIBRARIES}
)

# From PlatformWin.cmake
if (WIN32)
    list(REMOVE_ITEM JavaScriptCore_SOURCES
        inspector/JSGlobalObjectInspectorController.cpp
    )

    file(MAKE_DIRECTORY ${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore)

    set(JavaScriptCore_PRE_BUILD_COMMAND "${CMAKE_BINARY_DIR}/DerivedSources/JavaScriptCore/preBuild.cmd")
    file(REMOVE "${JavaScriptCore_PRE_BUILD_COMMAND}")
    foreach (_directory ${JavaScriptCore_FORWARDING_HEADERS_DIRECTORIES})
        file(APPEND "${JavaScriptCore_PRE_BUILD_COMMAND}" "@xcopy /y /d /f \"${JAVASCRIPTCORE_DIR}/${_directory}/*.h\" \"${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore\" >nul 2>nul\n")
    endforeach ()

    set(JavaScriptCore_POST_BUILD_COMMAND "${CMAKE_BINARY_DIR}/DerivedSources/JavaScriptCore/postBuild.cmd")
    file(WRITE "${JavaScriptCore_POST_BUILD_COMMAND}" "@xcopy /y /d /f \"${DERIVED_SOURCES_DIR}/JavaScriptCore/*.h\" \"${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore\" >nul 2>nul\n")
    file(APPEND "${JavaScriptCore_POST_BUILD_COMMAND}" "@xcopy /y /d /f \"${DERIVED_SOURCES_DIR}/JavaScriptCore/inspector/*.h\" \"${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore\" >nul 2>nul\n")
endif ()
