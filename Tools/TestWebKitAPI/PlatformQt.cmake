set(TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/TestWebKitAPI")
set(TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY_WTF "${TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY}/WTF")


add_definitions(
    -DROOT_BUILD_DIR="${CMAKE_BINARY_DIR}"
    -DQT_NO_CAST_FROM_ASCII
)

#if (WIN32)
#    add_definitions(-DUSE_CONSOLE_ENTRY_POINT)
#    add_definitions(-DWEBCORE_EXPORT=)
#    add_definitions(-DSTATICALLY_LINKED_WITH_WTF)
#endif ()

set(test_main_SOURCES
    ${TESTWEBKITAPI_DIR}/qt/main.cpp
)

list(APPEND TestWTF_LIBRARIES
    Qt5::Gui
)

target_sources(TestWTF PRIVATE
    ${test_main_SOURCES}
    qt/UtilitiesQt.cpp
)

#target_link_libraries(TestWebCore ${test_webcore_LIBRARIES})
#add_test(TestWebCore ${TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY}/WebCore/TestWebCore)
#set_tests_properties(TestWebCore PROPERTIES TIMEOUT 60)
#set_target_properties(TestWebCore PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY}/WebCore)
