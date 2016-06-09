set(TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/TestWebKitAPI")
set(TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY_WTF "${TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY}/WTF")

include_directories(
    ${DERIVED_SOURCES_DIR}/ForwardingHeaders
    ${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore
    ${TESTWEBKITAPI_DIR}
)

include_directories(SYSTEM
    ${Qt5Gui_INCLUDE_DIRS}
)

add_definitions(
    -DROOT_BUILD_DIR="${CMAKE_BINARY_DIR}"
    -DQT_NO_CAST_FROM_ASCII
)

set(test_main_SOURCES
    ${TESTWEBKITAPI_DIR}/qt/main.cpp
)

list(APPEND test_wtf_LIBRARIES
    ${Qt5Gui_LIBRARIES}
)

set(test_webcore_LIBRARIES
    WebCore
    gtest
    ${Qt5Gui_LIBRARIES}
)

add_executable(TestWebCore
    ${test_main_SOURCES}
    ${TESTWEBKITAPI_DIR}/TestsController.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebCore/LayoutUnit.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebCore/URL.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebCore/SharedBuffer.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebCore/FileSystem.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebCore/PublicSuffix.cpp
)

target_link_libraries(TestWebCore ${test_webcore_LIBRARIES})
add_test(TestWebCore ${TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY}/WebCore/TestWebCore)
set_tests_properties(TestWebCore PROPERTIES TIMEOUT 60)
set_target_properties(TestWebCore PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY}/WebCore)
