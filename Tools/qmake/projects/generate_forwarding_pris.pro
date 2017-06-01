load(functions)
load(qt_build_paths)

TEMPLATE = aux

qtConfig(debug_and_release): CONFIG += debug_and_release build_all

defineTest(writeForwardingPri) {
    module = $$1
    configuration = $$2
    cmake_build_dir = $$ROOT_BUILD_DIR/$$configuration
    forwarding_pri_name = $$MODULE_QMAKE_OUTDIR/mkspecs/modules/qt_lib_$${module}.pri

    FORWARDING_PRI_CONTENTS += \
        "QT_MODULE_BIN_BASE = $$cmake_build_dir/bin" \
        "QT_MODULE_INCLUDE_BASE = $$cmake_build_dir/DerivedSources/ForwardingHeaders" \
        "QT_MODULE_LIB_BASE = $$cmake_build_dir/lib" \
        "QT_MODULE_HOST_LIB_BASE = $$cmake_build_dir/lib" \
        "include($$cmake_build_dir/Source/WebKit/qt_lib_$${module}.pri)"

    FORWARDING_PRI_CONTENTS += \
        "QT.$${module}.priority = 1" \
        "QT.$${module}.includes += $$ROOT_WEBKIT_DIR/Source"

    message("Writing $$forwarding_pri_name")
    write_file($$forwarding_pri_name, FORWARDING_PRI_CONTENTS)|error()
}


debug_and_release {
    !build_pass {
        # Use release build in case of debug_and_release
        writeForwardingPri(webkit, release)
        writeForwardingPri(webkitwidgets, release)
    }
} else {
    CONFIG(debug, debug|release) {
        configuration = debug
    } else {
        configuration = release
    }
    writeForwardingPri(webkit, $$configuration)
    writeForwardingPri(webkitwidgets, $$configuration)
}
