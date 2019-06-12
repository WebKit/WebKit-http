include_directories(
    "${CMAKE_SOURCE_DIR}/Source"
    "${FORWARDING_HEADERS_DIR}/QtWebKit"
    "${WEBKIT_DIR}/qt/Api"
)

include_directories(SYSTEM
    ${Qt5Quick_INCLUDE_DIRS}
    ${Qt5Quick_PRIVATE_INCLUDE_DIRS}
    ${Qt5QuickTest_INCLUDE_DIRS}
)

set(tst_qmltests_DEFINITIONS
    IMPORT_DIR="${CMAKE_BINARY_DIR}/imports"
    QWP_PATH="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    TESTS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

set(tst_qmltests_DesktopBehavior_DEFINITIONS
    ${tst_qmltests_DEFINITIONS}

    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qmltests/DesktopBehavior"
    DISABLE_FLICKABLE_VIEWPORT=1
)

set(tst_qmltests_WebView_DEFINITIONS
    ${tst_qmltests_DEFINITIONS}

    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qmltests/WebView"
)

set(qmltests_QML_SOURCES
    qmltests/DesktopBehavior/tst_linkHovered.qml
    qmltests/DesktopBehavior/tst_loadHtml.qml
    qmltests/DesktopBehavior/tst_navigationRequested.qml

    qmltests/WebView/tst_applicationScheme.qml
    qmltests/WebView/tst_colorChooser.qml
    qmltests/WebView/tst_doubleTapToZoom.qml
    qmltests/WebView/tst_download.qml
    qmltests/WebView/tst_evaluateJavaScript.qml
    qmltests/WebView/tst_favIconLoad.qml
    qmltests/WebView/tst_findText.qml
    qmltests/WebView/tst_fitToView.qml
    qmltests/WebView/tst_geopermission.qml
    qmltests/WebView/tst_itemSelector.qml
    qmltests/WebView/tst_javaScriptDialogs.qml
    qmltests/WebView/tst_loadFail.qml
    qmltests/WebView/tst_loadHtml.qml
    qmltests/WebView/tst_loadProgress.qml
    qmltests/WebView/tst_loadProgressSignal.qml
    qmltests/WebView/tst_loadUrl.qml
    qmltests/WebView/tst_messaging.qml
    qmltests/WebView/tst_multiFileUpload.qml
    qmltests/WebView/tst_navigationHistory.qml
    qmltests/WebView/tst_notification.qml
    qmltests/WebView/tst_origin.qml
    qmltests/WebView/tst_preferences.qml
    qmltests/WebView/tst_properties.qml
    qmltests/WebView/tst_resize.qml
    qmltests/WebView/tst_singleFileUpload.qml
    qmltests/WebView/tst_titleChanged.qml
    qmltests/WebView/tst_userScripts.qml
    qmltests/WebView/tst_webchannel.qml
    qmltests/WebView/tst_wheelEventHandling.qml

    qmltests/common/TestWebView.qml
)

set(qmltests_SOURCES
    bytearraytestdata.cpp
    util.cpp

    qmltests/tst_qmltests.cpp
    ${qmltests_QML_SOURCES}
)

qt5_add_resources(qmltests_SOURCES qmltests/resources.qrc)

set(qmltests_LIBRARIES
    WebKit
    ${Qt5Quick_LIBRARIES}
    ${Qt5QuickTest_LIBRARIES}
    ${Qt5Test_LIBRARIES}
)

if (SHARED_CORE)
    list(APPEND qmltests_LIBRARIES
        WebKit2
    )
endif ()

add_executable(tst_qmltests_WebView ${qmltests_SOURCES})
target_compile_definitions(tst_qmltests_WebView PRIVATE ${tst_qmltests_WebView_DEFINITIONS})
target_link_libraries(tst_qmltests_WebView ${qmltests_LIBRARIES})
set_target_properties(tst_qmltests_WebView PROPERTIES AUTOMOC ON)

add_executable(tst_qmltests_DesktopBehavior ${qmltests_SOURCES})
target_compile_definitions(tst_qmltests_DesktopBehavior PRIVATE ${tst_qmltests_DesktopBehavior_DEFINITIONS})
target_link_libraries(tst_qmltests_DesktopBehavior ${qmltests_LIBRARIES})
set_target_properties(tst_qmltests_DesktopBehavior PROPERTIES AUTOMOC ON)
