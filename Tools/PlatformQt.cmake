remove_definitions(-DQT_ASCII_CAST_WARNINGS)

add_subdirectory(QtTestBrowser)

if (ENABLE_TEST_SUPPORT)
    add_subdirectory(DumpRenderTree)
    add_subdirectory(ImageDiff)
endif ()

if (ENABLE_WEBKIT)
    add_subdirectory(MiniBrowser/qt)
endif ()

# FIXME: Remove when WK2 Tools patches are merged
set(ENABLE_WEBKIT 0)

if (ENABLE_API_TESTS AND NOT ENABLE_WEBKIT)
    add_subdirectory(TestWebKitAPI)
endif ()
