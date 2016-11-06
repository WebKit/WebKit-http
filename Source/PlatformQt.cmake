# GTest

if (ENABLE_API_TESTS)
    set(GTEST_DEFINITIONS QT_NO_KEYWORDS)
    if (COMPILER_IS_GCC_OR_CLANG)
        list(APPEND GTEST_DEFINITIONS "GTEST_API_=__attribute__((visibility(\"default\")))")
    endif ()
    set_target_properties(gtest PROPERTIES COMPILE_DEFINITIONS "${GTEST_DEFINITIONS}")
endif ()

# Installation

target_include_directories(WebKit INTERFACE $<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/QtWebKit>)
target_include_directories(WebKitWidgets INTERFACE $<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/QtWebKitWidgets>)

ecm_configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/Qt5WebKitConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Qt5WebKitConfig.cmake"
    INSTALL_DESTINATION "${KDE_INSTALL_CMAKEPACKAGEDIR}/Qt5WebKit"
)
ecm_configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/Qt5WebKitWidgetsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Qt5WebKitWidgetsConfig.cmake"
    INSTALL_DESTINATION "${KDE_INSTALL_CMAKEPACKAGEDIR}/Qt5WebKitWidgets"
)

write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/Qt5WebKitConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/Qt5WebKitWidgetsConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/Qt5WebKitConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/Qt5WebKitConfigVersion.cmake"
    DESTINATION "${KDE_INSTALL_CMAKEPACKAGEDIR}/Qt5WebKit"
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/Qt5WebKitWidgetsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/Qt5WebKitWidgetsConfigVersion.cmake"
    DESTINATION "${KDE_INSTALL_CMAKEPACKAGEDIR}/Qt5WebKitWidgets"
)

install(EXPORT WebKitTargets
    FILE WebKitTargets.cmake
    NAMESPACE Qt5::
    DESTINATION "${KDE_INSTALL_CMAKEPACKAGEDIR}/Qt5WebKit"
)
install(EXPORT Qt5WebKitWidgetsTargets
    FILE Qt5WebKitWidgetsTargets.cmake
    NAMESPACE Qt5::
    DESTINATION "${KDE_INSTALL_CMAKEPACKAGEDIR}/Qt5WebKitWidgets"
)

# Documentation

if (NOT TARGET Qt5::qdoc)
    add_executable(Qt5::qdoc IMPORTED)
    query_qmake(QDOC_EXECUTABLE QT_INSTALL_BINS)
    set(QDOC_EXECUTABLE "${QDOC_EXECUTABLE}/qdoc")
    set_target_properties(Qt5::qdoc PROPERTIES
        IMPORTED_LOCATION ${QDOC_EXECUTABLE}
    )
endif ()

if (NOT TARGET Qt5::qhelpgenerator)
    add_executable(Qt5::qhelpgenerator IMPORTED)
    query_qmake(QHELPGENERATOR_EXECUTABLE QT_INSTALL_BINS)
    set(QHELPGENERATOR_EXECUTABLE "${QHELPGENERATOR_EXECUTABLE}/qhelpgenerator")
    set_target_properties(Qt5::qhelpgenerator PROPERTIES
        IMPORTED_LOCATION ${QHELPGENERATOR_EXECUTABLE}
    )
endif ()

query_qmake(QT_INSTALL_DOCS QT_INSTALL_DOCS)
set(QDOC_CONFIG "${CMAKE_SOURCE_DIR}/Source/qtwebkit.qdocconf")
set(DOC_OUTPUT_DIR "${CMAKE_BINARY_DIR}/doc")
set(PROJECT_VERSION_TAG ${PROJECT_VERSION_MAJOR}${PROJECT_VERSION_MINOR}${PROJECT_VERSION_MICRO})

if (KDE_INSTALL_USE_QT_SYS_PATHS)
    set(DOC_INSTALL_DIR ${QT_INSTALL_DOCS})
else ()
    set(DOC_INSTALL_DIR "doc")
endif ()

if (WIN32)
    set(EXPORT_VAR set)
else ()
    set(EXPORT_VAR export)
endif ()

if (GENERATE_DOCUMENTATION)
    set(NEED_ALL "ALL")
else ()
    set(NEED_ALL "")
endif ()

set(EXPORT_VARS_COMMANDS
    COMMAND ${EXPORT_VAR} "QT_INSTALL_DOCS=${QT_INSTALL_DOCS}"
    COMMAND ${EXPORT_VAR} "QT_VER=${PROJECT_VERSION_STRING}"
    COMMAND ${EXPORT_VAR} "QT_VERSION=${PROJECT_VERSION_STRING}"
    COMMAND ${EXPORT_VAR} "QT_VERSION_TAG=${PROJECT_VERSION_TAG}"
)

add_custom_target(prepare_docs ${NEED_ALL}
    ${EXPORT_VARS_COMMANDS}
    COMMAND Qt5::qdoc ${QDOC_CONFIG} -prepare -outputdir "${DOC_OUTPUT_DIR}/qtwebkit" -installdir ${DOC_INSTALL_DIR} -indexdir ${QT_INSTALL_DOCS} -no-link-errors
    VERBATIM
)

add_custom_target(generate_docs ${NEED_ALL}
    ${EXPORT_VARS_COMMANDS}
    COMMAND Qt5::qdoc ${QDOC_CONFIG} -generate -outputdir "${DOC_OUTPUT_DIR}/qtwebkit" -installdir ${DOC_INSTALL_DIR} -indexdir ${QT_INSTALL_DOCS}
    VERBATIM
)
add_dependencies(generate_docs prepare_docs)

add_custom_target(html_docs)
add_dependencies(html_docs generate_docs)

add_custom_target(qch_docs ${NEED_ALL}
    COMMAND Qt5::qhelpgenerator "${DOC_OUTPUT_DIR}/qtwebkit/qtwebkit.qhp" -o "${DOC_OUTPUT_DIR}/qtwebkit.qch"
    VERBATIM
)
add_dependencies(qch_docs html_docs)

add_custom_target(docs)
add_dependencies(docs qch_docs)

if (GENERATE_DOCUMENTATION)
    install(DIRECTORY "${DOC_OUTPUT_DIR}/qtwebkit" DESTINATION ${DOC_INSTALL_DIR})
    install(FILES "${DOC_OUTPUT_DIR}/qtwebkit.qch" DESTINATION ${DOC_INSTALL_DIR})
endif ()

# Uninstall target

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)
