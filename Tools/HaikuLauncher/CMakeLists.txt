SET(HaikuLauncher_SOURCES
    ${TOOLS_DIR}/HaikuLauncher/LauncherApp.cpp
    ${TOOLS_DIR}/HaikuLauncher/LauncherWindow.cpp
)

SET(HaikuLauncher_LIBRARIES
    ${JavaScriptCore_LIBRARY_NAME}
    ${WebCore_LIBRARY_NAME}
    ${WebKit_LIBRARY_NAME}
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${SQLITE_LIBRARIES}
    ${ICU_I18N_LIBRARIES}
    ${ICU_LIBRARIES}
    be bsd network stdc++ translation
)

INCLUDE_DIRECTORIES("${WEBKIT_DIR}/haiku/API")

ADD_EXECUTABLE(bin/HaikuLauncher ${HaikuLauncher_SOURCES})
TARGET_LINK_LIBRARIES(bin/HaikuLauncher ${HaikuLauncher_LIBRARIES})
SET_TARGET_PROPERTIES(bin/HaikuLauncher PROPERTIES FOLDER "Tools")
SET_TARGET_PROPERTIES(bin/HaikuLauncher PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
