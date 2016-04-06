cmake_minimum_required(VERSION 2.8.12)

include_directories(
    "${DERIVED_SOURCES_WEBKIT_DIR}"
    "${WEBKIT_DIR}/qt/Api"
    "${WEBKIT_DIR}/qt/WidgetApi"
)

include_directories(SYSTEM
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Test_INCLUDE_DIRS}
)

set(QtWK1ApiTests_LIBRARIES
    ${Qt5Gui_LIBRARIES}
    ${Qt5Network_LIBRARIES}
    ${Qt5Test_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    WebCoreTestSupport
    WebKitWidgets
)

# Inspired by EFL WK2 tests
set(QtWK1ApiTests_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_BINARY_DIR}/tests
)

set(QtWK1ApiTests
    qwebframe
)

if (ENABLE_API_TESTS)
    foreach (testName ${QtWK1ApiTests})
        set(tst_${testName}_SOURCES ${testName}/tst_${testName}.cpp)
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${testName}/tst_${testName}.qrc")
            qt5_add_resources(tst_${testName}_SOURCES ${testName}/tst_${testName}.qrc)
        endif ()

        add_executable(tst_${testName} ${tst_${testName}_SOURCES})
        target_link_libraries(tst_${testName} ${QtWK1ApiTests_LIBRARIES})
        set_target_properties(tst_${testName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${QtWK1ApiTests_RUNTIME_OUTPUT_DIRECTORY})

        add_test(${testName} "${QtWK1ApiTests_RUNTIME_OUTPUT_DIRECTORY}/tst_${testName}")
        set_tests_properties(${testName} PROPERTIES TIMEOUT 60)
    endforeach ()
endif ()
