set(ForwardingHeadersForWebKitTestRunner_NAME WebKitTestRunner-forwarding-headers)

list(APPEND WebKitTestRunner_SOURCES
    ${WEBKIT_TESTRUNNER_DIR}/wpe/EventSenderProxyWPE.cpp
    ${WEBKIT_TESTRUNNER_DIR}/wpe/HeadlessViewBackend.cpp
    ${WEBKIT_TESTRUNNER_DIR}/wpe/PlatformWebViewWPE.cpp
    ${WEBKIT_TESTRUNNER_DIR}/wpe/TestControllerWPE.cpp
    ${WEBKIT_TESTRUNNER_DIR}/wpe/TestInvocationWPE.cpp
    ${WEBKIT_TESTRUNNER_DIR}/wpe/main.cpp
)

list(APPEND WebKitTestRunner_INCLUDE_DIRECTORIES
    ${CMAKE_SOURCE_DIR}/Source/ThirdParty/WPE/include
    ${CMAKE_SOURCE_DIR}/Source/ThirdParty/WPE-mesa/include
    ${FORWARDING_HEADERS_DIR}
    ${CAIRO_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
)

list(APPEND WebKitTestRunner_LIBRARIES
    WPE-mesa
    ${CAIRO_LIBRARIES}
    ${GLIB_LIBRARIES}
)

list(APPEND WebKitTestRunnerInjectedBundle_SOURCES
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/wpe/ActivateFontsWPE.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/wpe/InjectedBundleWPE.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/wpe/TestRunnerWPE.cpp
)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/WebKitTestRunner-forwarding-headers.stamp
    DEPENDS ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl
            ${WebKitTestRunner_SOURCES}
            ${WebKitTestRunner_HEADERS}
            ${WebKitTestRunnerInjectedBundle_SOURCES}
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl --include-path ${WEBKIT_TESTRUNNER_DIR} --output ${FORWARDING_HEADERS_DIR} --platform wpe
    COMMAND touch ${CMAKE_BINARY_DIR}/WebKitTestRunner-forwarding-headers.stamp
)
add_custom_target(WebKitTestRunner-forwarding-headers
    DEPENDS ${CMAKE_BINARY_DIR}/WebKitTestRunner-forwarding-headers.stamp
)
