add_custom_target(forwarding-headersHaikuForWebKitTestRunner
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl ${WEBKIT_TESTRUNNER_DIR} ${DERIVED_SOURCES_WEBKIT2_DIR}/include haiku
)
set(ForwardingHeadersForWebKitTestRunner_NAME forwarding-headersHaikuForWebKitTestRunner)


