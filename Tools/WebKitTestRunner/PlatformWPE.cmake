set(ForwardingHeadersForWebKitTestRunner_NAME WebKitTestRunner-forwarding-headers)

list(APPEND WebKitTestRunner_SOURCES
    ${WEBKIT_TESTRUNNER_DIR}/wpe/EventSenderProxyWPE.cpp
    ${WEBKIT_TESTRUNNER_DIR}/wpe/PlatformWebViewWPE.cpp
    ${WEBKIT_TESTRUNNER_DIR}/wpe/TestControllerWPE.cpp
    ${WEBKIT_TESTRUNNER_DIR}/wpe/TestInvocationWPE.cpp
    ${WEBKIT_TESTRUNNER_DIR}/wpe/Module.cpp
)

list(APPEND WebKitTestRunner_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${GLIB_INCLUDE_DIRS}
)

list(APPEND WebKitTestRunner_LIBRARIES
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

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/bin/WebKitTestRunner
    DEPENDS ${WEBKIT_TESTRUNNER_DIR}/wpe/WebKitTestRunner.in
    COMMAND cp ${WEBKIT_TESTRUNNER_DIR}/wpe/WebKitTestRunner.in ${CMAKE_BINARY_DIR}/bin/WebKitTestRunner
    COMMAND chmod +x ${CMAKE_BINARY_DIR}/bin/WebKitTestRunner
)
add_custom_target(WebKitTestRunner-shell-script
    DEPENDS ${CMAKE_BINARY_DIR}/bin/WebKitTestRunner
)
