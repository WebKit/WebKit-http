remove_definitions(-DQT_ASCII_CAST_WARNINGS)

include_directories(
    "${CMAKE_SOURCE_DIR}/Source"
    "${FORWARDING_HEADERS_DIR}"
    "${WEBKIT_DIR}/qt/Api"
    "${WEBKIT_DIR}/qt/WidgetApi"
)

include_directories(SYSTEM
    ${ICU_INCLUDE_DIRS}
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Test_INCLUDE_DIRS}
)

add_definitions(-DTESTS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/")

if (ENABLE_TEST_SUPPORT)
    add_definitions(-DHAVE_QTTESTSUPPORT)
endif ()

set(QtWK1ApiTests_LIBRARIES
    ${Qt5Gui_LIBRARIES}
    ${Qt5Network_LIBRARIES}
    ${Qt5Test_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    WebKitWidgets
)

# Inspired by EFL WK2 tests
set(QtWK1ApiTests_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_BINARY_DIR}/tests
)

set(QtWK1ApiTests
    hybridPixmap
    qgraphicswebview
    qobjectbridge
    qwebelement
    qwebframe
    qwebhistory
    qwebhistoryinterface
    qwebinspector
    qwebpage
    qwebsecurityorigin
    qwebview
)

set(tst_hybridPixmap_SOURCES hybridPixmap/widget.cpp)
qt5_wrap_ui(tst_hybridPixmap_SOURCES hybridPixmap/widget.ui)

foreach (testName ${QtWK1ApiTests})
    list(APPEND tst_${testName}_SOURCES ${testName}/tst_${testName}.cpp)
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${testName}/tst_${testName}.qrc")
        qt5_add_resources(tst_${testName}_SOURCES ${testName}/tst_${testName}.qrc)
    endif ()

    add_executable(tst_${testName} ${tst_${testName}_SOURCES})
    target_include_directories(tst_${testName} PRIVATE ${testName})
    target_link_libraries(tst_${testName} ${QtWK1ApiTests_LIBRARIES})
    set_target_properties(tst_${testName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${QtWK1ApiTests_RUNTIME_OUTPUT_DIRECTORY} AUTOMOC ON)

    add_test(${testName} "${QtWK1ApiTests_RUNTIME_OUTPUT_DIRECTORY}/tst_${testName}")
    set_tests_properties(${testName} PROPERTIES TIMEOUT 60)
endforeach ()
