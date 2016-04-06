if (ENABLE_API_TESTS)
    set_target_properties(gtest PROPERTIES COMPILE_DEFINITIONS QT_NO_KEYWORDS)
endif ()
