load(functions)

ROOT_QT_BUILD_DIR = $$ROOT_BUILD_DIR/..

TEMPLATE = aux

CONFIG(debug, debug|release) {
    configuration = Debug
} else {
    configuration = Release
}

cmake_build_dir = $$system_quote($$system_path($$ROOT_BUILD_DIR/$$lower($$configuration)))
toolchain_file = $$system_quote($$system_path($$ROOT_BUILD_DIR/qmake_toolchain.cmake))

build_pass|!debug_and_release {
    CMAKE_CONFIG += \
        PORT=Qt \
        CMAKE_BUILD_TYPE=$$configuration \
        CMAKE_TOOLCHAIN_FILE=$$toolchain_file \
        CMAKE_PREFIX_PATH=\"$$[QT_INSTALL_PREFIX];$$ROOT_QT_BUILD_DIR/qtbase;$$ROOT_QT_BUILD_DIR/qtlocation;$$ROOT_QT_BUILD_DIR/qtsensors\" \
        CMAKE_INSTALL_PREFIX=\"$$[QT_INSTALL_PREFIX]\" \
        KDE_INSTALL_LIBDIR=\"$$relative_path($$[QT_INSTALL_LIBS], $$[QT_INSTALL_PREFIX])\" \
        USE_LIBHYPHEN=OFF

    QT_FOR_CONFIG += gui-private
    !qtConfig(system-jpeg):exists($$QTBASE_DIR) {
        CMAKE_CONFIG += \
            QT_BUNDLED_JPEG=1 \
            JPEG_INCLUDE_DIR=$$system_path($$QTBASE_DIR/src/3rdparty/libjpeg) \
            JPEG_LIBRARIES=$$system_path($$ROOT_BUILD_DIR/lib/libqtjpeg.$$QMAKE_EXTENSION_STATICLIB)
    }

    !qtConfig(system-png):qtConfig(png):exists($$QTBASE_DIR) {
        CMAKE_CONFIG += \
            QT_BUNDLED_PNG=1 \
            PNG_INCLUDE_DIRS=$$system_path($$QTBASE_DIR/src/3rdparty/libpng) \
            PNG_LIBRARIES=$$system_path($$ROOT_BUILD_DIR/lib/libqtpng.$$QMAKE_EXTENSION_STATICLIB)
    }

    !qtConfig(system-zlib):exists($$QTBASE_DIR) {
        CMAKE_CONFIG += \
            QT_BUNDLED_ZLIB=1 \
            ZLIB_INCLUDE_DIRS=$$system_path($$QTBASE_DIR/src/3rdparty/zlib)
    }

    equals(QMAKE_HOST.os, Windows): cmake_args += "-G \"NMake Makefiles JOM\""

    !silent: make_args += "VERBOSE=1"

    # Append additional platform options defined in CMAKE_CONFIG
    for (config, CMAKE_CONFIG): cmake_args += "-D$$config"

    cmake_cmd_base = "$$QMAKE_MKDIR $$cmake_build_dir && cd $$cmake_build_dir &&"

    log("$${EOL}Running $$cmake_env cmake $$ROOT_WEBKIT_DIR $$cmake_args $${EOL}$${EOL}")
    !system("$$cmake_cmd_base $$cmake_env cmake $$ROOT_WEBKIT_DIR $$cmake_args"): error("Running cmake failed")

    log("$${EOL}WebKit is now configured for building. Just run 'make'.$${EOL}$${EOL}")


    default_target.target = first
    default_target.commands = cd $$cmake_build_dir && $(MAKE) $$make_args
    QMAKE_EXTRA_TARGETS += default_target

    install_target.target = install
    install_target.commands = cd $$cmake_build_dir && $(MAKE) install $$make_args DESTDIR=$(INSTALL_ROOT)
    QMAKE_EXTRA_TARGETS += install_target
}
