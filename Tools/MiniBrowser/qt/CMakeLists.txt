set(MiniBrowser_INCLUDE_DIRECTORIES
    ${CMAKE_BINARY_DIR}
    "${CMAKE_SOURCE_DIR}/Source"
    "${FORWARDING_HEADERS_DIR}/QtWebKit"
    "${WEBKIT_DIR}/qt/Api"
    "${WEBKIT_DIR}/qt/WebCoreSupport"
    "${WEBKIT2_DIR}/UIProcess/API/qt"
    ${WTF_DIR}
)

set(MiniBrowser_QML_SOURCES
    qml/AlertDialog.qml
    qml/AuthenticationDialog.qml
    qml/BrowserWindow.qml
    qml/CheckBox.qml
    qml/ColorChooser.qml
    qml/ConfirmDialog.qml
    qml/Dialog.qml
    qml/DialogButton.qml
    qml/DialogLineInput.qml
    qml/FilePicker.qml
    qml/ItemSelector.qml
    qml/MockTouchPoint.qml
    qml/PromptDialog.qml
    qml/ProxyAuthenticationDialog.qml
    qml/ScrollIndicator.qml
    qml/ViewportInfoItem.qml
)

set(QML_IMPORT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/qml ${CMAKE_BINARY_DIR}/imports
    CACHE STRING "" FORCE)

set(MiniBrowser_SOURCES
    BrowserWindow.cpp
    MiniBrowserApplication.cpp
    UrlLoader.cpp
    main.cpp
    utils.cpp
    ${MiniBrowser_QML_SOURCES}
)

qt5_add_resources(MiniBrowser_SOURCES
    MiniBrowser.qrc
)

set(MiniBrowser_SYSTEM_INCLUDE_DIRECTORIES
    ${Qt5Quick_INCLUDE_DIRS}
    ${Qt5Quick_PRIVATE_INCLUDE_DIRS}
)

set(MiniBrowser_LIBRARIES
    WebKit
    ${Qt5Quick_LIBRARIES}
)

if (SHARED_CORE)
    list(APPEND MiniBrowser_LIBRARIES
        WebKit2
    )
endif ()

if (ENABLE_TEST_SUPPORT)
    add_definitions(-DHAVE_QTTESTSUPPORT)
endif ()

include_directories(${MiniBrowser_INCLUDE_DIRECTORIES})
include_directories(SYSTEM ${MiniBrowser_SYSTEM_INCLUDE_DIRECTORIES})
add_executable(MiniBrowser ${MiniBrowser_SOURCES})
target_link_libraries(MiniBrowser ${MiniBrowser_LIBRARIES})
set_target_properties(MiniBrowser PROPERTIES FOLDER "Tools" AUTOMOC ON)
