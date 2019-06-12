add_library(qmlwebkitexperimentalplugin MODULE plugin.cpp)
target_link_libraries(qmlwebkitexperimentalplugin
    WebKit Qt5::Quick)

set(qmlwebkitexperimental_output_dir "${CMAKE_BINARY_DIR}/imports/QtWebKit/experimental")
set(qmlwebkitexperimental_install_dir "${QML_INSTALL_DIR}/QtWebKit/experimental")

set_target_properties(qmlwebkitexperimentalplugin PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${qmlwebkitexperimental_output_dir}
    LIBRARY_OUTPUT_DIRECTORY ${qmlwebkitexperimental_output_dir}
    RUNTIME_OUTPUT_DIRECTORY ${qmlwebkitexperimental_output_dir}
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${qmlwebkitexperimental_output_dir}
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${qmlwebkitexperimental_output_dir}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${qmlwebkitexperimental_output_dir}
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${qmlwebkitexperimental_output_dir}
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${qmlwebkitexperimental_output_dir}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${qmlwebkitexperimental_output_dir}

    AUTOMOC ON
)

add_custom_target(
    qmlwebkitexperimentalplugin-files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different qmldir "${CMAKE_BINARY_DIR}/imports/QtWebKit/experimental/qmldir"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS qmlwebkitexperimentalplugin
        DESTINATION ${qmlwebkitexperimental_install_dir})
install(FILES qmldir
        DESTINATION ${qmlwebkitexperimental_install_dir})
