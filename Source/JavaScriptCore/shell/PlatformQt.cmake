if (QT_STATIC_BUILD)
    list(APPEND JSC_LIBRARIES
        ${STATIC_LIB_DEPENDENCIES}
    )
endif ()

if (DEVELOPER_MODE)
    list(APPEND testapi_SOURCES
        ../API/tests/qt/main.cpp
    )
endif ()
