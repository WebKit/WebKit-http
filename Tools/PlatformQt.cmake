remove_definitions(-DQT_ASCII_CAST_WARNINGS)

if (ENABLE_WEBKIT_LEGACY)
    add_subdirectory(QtTestBrowser)
endif ()

if (ENABLE_TEST_SUPPORT)
    add_subdirectory(DumpRenderTree)
    add_subdirectory(ImageDiff)
endif ()

if (ENABLE_WEBKIT)
    add_subdirectory(MiniBrowser/qt)
endif ()
