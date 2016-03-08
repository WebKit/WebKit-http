cmake_minimum_required(VERSION 2.8.12)

include_directories(
    "${WEBKIT_DIR}/qt/Api"
    "${WEBKIT_DIR}/qt/WidgetApi"
)

include_directories(SYSTEM
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Test_INCLUDE_DIRS}
)

set(tst_qwebframe_SOURCES qwebframe/tst_qwebframe.cpp)
qt5_add_resources(tst_qwebframe_SOURCES qwebframe/tst_qwebframe.qrc)
add_executable(tst_qwebframe ${tst_qwebframe_SOURCES})
add_test(tst_qwebframe "${CMAKE_CURRENT_BINARY_DIR}/tst_qwebframe")
target_link_libraries(tst_qwebframe
    ${Qt5Gui_LIBRARIES}
    ${Qt5Network_LIBRARIES}
    ${Qt5Test_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    WebCore # FIXME: Fix cmake to add WebKit's dependencies automatically
    WebCoreTestSupport
    WebKitWidgets
)
