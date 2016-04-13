set(InspectorFiles
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/*.html
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Base/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Controllers/*.css
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Controllers/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Debug/*.css
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Debug/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/External/CodeMirror/*.css
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/External/CodeMirror/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/External/ESLint/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/External/Esprima/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Models/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Protocol/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Proxies/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Test/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Views/*.css
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Views/*.js
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Images/gtk/*.png
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Images/gtk/*.svg
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/Localizations/en.lproj/localizedStrings.js
)

file(GLOB InspectorFilesDependencies
    ${InspectorFiles}
)

# DerivedSources/JavaScriptCore/inspector/InspectorBackendCommands.js is
# expected in DerivedSources/WebInspectorUI/UserInterface/Protocol/.
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/UserInterface/Protocol/InspectorBackendCommands.js
    DEPENDS ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/inspector/InspectorBackendCommands.js
    COMMAND cmake -E copy ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/inspector/InspectorBackendCommands.js ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/UserInterface/Protocol/InspectorBackendCommands.js
)

get_target_property(RCC_EXECUTABLE ${Qt5Core_RCC_EXECUTABLE} IMPORTED_LOCATION)

add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/qrc_WebInspector.cpp
    DEPENDS ${InspectorFilesDependencies}
            ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/UserInterface/Protocol/InspectorBackendCommands.js
            ${TOOLS_DIR}/qt/generate-inspector-qrc.pl
    COMMAND ${PERL_EXECUTABLE} ${TOOLS_DIR}/qt/generate-inspector-qrc.pl
            --baseDir ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI
            --outDir ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}
            --prefix /webkit/inspector
            --rccExecutable ${RCC_EXECUTABLE}
            --resourceName WebInspector
            --add UserInterface/Protocol/InspectorBackendCommands.js
            ${InspectorFiles}
    VERBATIM
)
