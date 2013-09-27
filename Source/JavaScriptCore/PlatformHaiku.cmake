LIST(APPEND JavaScriptCore_SOURCES
    jit/ExecutableAllocatorFixedVMPool.cpp
    jit/ExecutableAllocator.cpp
)

LIST(APPEND JavaScriptCore_LIBRARIES
    ${ICU_I18N_LIBRARIES}
)

LIST(APPEND JavaScriptCore_INCLUDE_DIRECTORIES
    ${ICU_INCLUDE_DIRS}
)

LIST(APPEND JavaScriptCore_LINK_FLAGS
    be
)
