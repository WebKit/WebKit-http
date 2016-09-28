if (QT_STATIC_BUILD)
    list(APPEND JSC_LIBRARIES
        ${DEPEND_STATIC_LIBS}
    )
endif ()
