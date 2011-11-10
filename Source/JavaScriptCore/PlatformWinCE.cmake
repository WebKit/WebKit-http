LIST(APPEND JavaScriptCore_SOURCES
    jit/ExecutableAllocator.cpp
)

IF (ENABLE_JIT AND WTF_CPU_ARM)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${DERIVED_SOURCES_DIR}/GeneratedJITStubs.asm
        MAIN_DEPENDENCY ${JAVASCRIPTCORE_DIR}/create_jit_stubs
        DEPENDS ${JAVASCRIPTCORE_DIR}/jit/JITStubs.cpp
        COMMAND ${PERL_EXECUTABLE} ${JAVASCRIPTCORE_DIR}/create_jit_stubs --prefix=MSVC ${JAVASCRIPTCORE_DIR}/jit/JITStubs.cpp > ${DERIVED_SOURCES_DIR}/GeneratedJITStubs.asm
        VERBATIM)

    ADD_CUSTOM_COMMAND(
        OUTPUT ${DERIVED_SOURCES_DIR}/GeneratedJITStubs.obj
        MAIN_DEPENDENCY ${DERIVED_SOURCES_DIR}/GeneratedJITStubs.asm
        COMMAND armasm -nologo ${DERIVED_SOURCES_DIR}/GeneratedJITStubs.asm ${DERIVED_SOURCES_DIR}/GeneratedJITStubs.obj
        VERBATIM)

    LIST (APPEND JavaScriptCore_SOURCES ${DERIVED_SOURCES_DIR}/GeneratedJITStubs.obj)
ENDIF ()
